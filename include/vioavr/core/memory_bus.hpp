#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/pin_map.hpp"
#include "vioavr/core/tracing.hpp"
#include "vioavr/core/types.hpp"

#include "vioavr/core/event_scheduler.hpp"
#include <array>
#include <span>
#include <string_view>
#include <vector>
#include <string>
#include <memory>

// Include peripheral headers for inline implementations
#include "vioavr/core/xmem.hpp"
#include "vioavr/core/nvm_ctrl.hpp"
#include "vioavr/core/eeprom.hpp"
#include "vioavr/core/cpu_int.hpp"
#include "vioavr/core/logger.hpp"

namespace vioavr::core {

class MemoryBus {
public:
    static constexpr u16 kLowIoBase = 0x20U;
    static constexpr u16 kLowIoSize = 0x40U;

    explicit MemoryBus(const DeviceDescriptor& device);

    void reset() noexcept;
    void set_trace_hook(ITraceHook* trace_hook) noexcept;
    void set_pin_map(PinMap* pin_map) noexcept;
    void set_pin_mux(class PinMux* pin_mux) noexcept { pin_mux_ = pin_mux; }
    [[nodiscard]] class PinMux* pin_mux() const noexcept { return pin_mux_; }
    void attach_peripheral(IoPeripheral& peripheral);
    void register_ticking_peripheral(IoPeripheral& peripheral) noexcept;
    void set_peripheral_active(IoPeripheral* peripheral, bool active) noexcept;
    void load_flash(std::span<const u16> words);
    void load_image(const HexImage& image);

    void execute_spm(u8 command, u32 address, u16 data, u32 pc_word) noexcept;

    [[nodiscard]] constexpr std::span<const u16> flash_words() const noexcept
    {
        return flash_;
    }

    [[nodiscard]] constexpr std::span<u8> data_space() noexcept
    {
        return data_;
    }

    [[nodiscard]] inline u32 flash_size_words() const noexcept { return static_cast<u32>(flash_.size()); }
    [[nodiscard]] inline const u16* flash_ptr() const noexcept { return flash_.data(); }

    [[nodiscard]] constexpr std::span<const u8> data_space() const noexcept
    {
        return data_;
    }

    [[nodiscard]] inline u16 read_program_word(u32 word_address, u32 pc_word = 0xFFFFFFFFU) const noexcept
    {
        (void)pc_word;
        if (flash_wait_states_ > 0U) request_cpu_stall(flash_wait_states_);
        if (flash_rww_busy_ && word_address <= device_.flash_rww_end_word) {
            return 0xFFFFU;
        }
        if (spm_busy_cycles_left_ > 0U && word_address > device_.flash_rww_end_word) {
            request_cpu_stall(spm_busy_cycles_left_);
        }
        if (word_address >= flash_.size()) return 0xFFFFU;
        return flash_[word_address];
    }
    [[nodiscard]] inline u8 read_program_byte(u32 byte_address, u32 pc_word = 0xFFFFFFFFU) const noexcept
    {
        if (flash_wait_states_ > 0U) request_cpu_stall(flash_wait_states_);

        // Lockbit Enforcement for LPM (Classic Mega)
        if (pc_word != 0xFFFFFFFFU && device_.boot_start_address != 0U) {
            const u32 pc_bytes = pc_word << 1U;
            const u8 lb = lockbit_ & 0x3FU;
            const u32 boot_start_bytes = device_.boot_start_address << 1U;
            const bool boot_pc = (pc_bytes >= boot_start_bytes);
            const bool app_target = (byte_address < boot_start_bytes);

            if (boot_pc && app_target) { // LPM from Boot to App
                if ((lb & 0x04U) == 0U) return 0xFFU;
            } else if (!boot_pc && !app_target) { // LPM from App to Boot
                if ((lb & 0x10U) == 0U) return 0xFFU;
            }
        }

        if (lpm_special_mode_timeout_ > 0U) {
            if (device_.sigrd_mask != 0 && (last_spmcsr_val_ & device_.sigrd_mask) != 0U) {
                const u32 offset = byte_address >> 1U;
                if (offset < device_.signature.size()) return device_.signature[offset];
            }
            if (device_.blbset_mask != 0 && (last_spmcsr_val_ & device_.blbset_mask) != 0U) {
                if (byte_address == 0x0000) return fuses_[0];
                if (byte_address == 0x0001) return lockbit_;
                if (byte_address == 0x0002) return fuses_[2];
                if (byte_address == 0x0003) return fuses_[1];
                return 0xFFU;
            }
        }

        const u32 word_address = byte_address >> 1U;
        if (flash_rww_busy_ && word_address <= device_.flash_rww_end_word) return 0xFFU;
        if (spm_busy_cycles_left_ > 0U && word_address > device_.flash_rww_end_word) {
            request_cpu_stall(spm_busy_cycles_left_);
        }
        
        if (word_address >= flash_.size()) return 0U;
        return static_cast<u8>(flash_[word_address] >> ((byte_address & 1U) ? 8U : 0U));
    }

    void write_program_word(u32 word_address, u16 value) noexcept;

    [[nodiscard]] constexpr u32 loaded_program_words() const noexcept
    {
        return loaded_program_words_;
    }

    [[nodiscard]] constexpr u32 reset_word_address() const noexcept
    {
        return reset_word_address_;
    }

    [[nodiscard]] static constexpr u16 low_io_address(u8 io_offset) noexcept
    {
        return static_cast<u16>(kLowIoBase + io_offset);
    }

    [[nodiscard]] std::span<IoPeripheral* const> peripherals() const noexcept
    {
        return peripherals_;
    }

    [[nodiscard]] inline u8 read_data(u16 address) noexcept;
    inline void write_data(u16 address, u8 value) noexcept;
    void tick_peripherals(u64 elapsed_cycles, u8 active_domains = 0xFFU) noexcept;
    
    [[nodiscard]] inline u64 cpu_cycles() const noexcept { return scheduler_.current_cycle(); }
    [[nodiscard]] inline u64 domain_cycles(ClockDomain domain) const noexcept
    {
        const u8 d = static_cast<u8>(domain);
        const u64 current = scheduler_.current_cycle();
        const u64 gated = domain_gated_cycles_[d];
        if (current <= gated) return 0U;
        return current - gated;
    }
    [[nodiscard]] inline u32 io_stall_cycles() const noexcept { return io_stall_cycles_; }
    [[nodiscard]] EventScheduler& scheduler() noexcept { return scheduler_; }
    [[nodiscard]] inline bool has_pending_pin_changes() const noexcept { return has_pending_pin_changes_; }
    void mark_pin_change_pending() noexcept { has_pending_pin_changes_ = true; }
    [[nodiscard]] bool consume_pin_change(PinStateChange& change) noexcept;
    void propagate_external_pin_change(u16 pin_address, u8 bit_index, PinLevel level) noexcept;
    void propagate_analog_pin_change(u16 pin_address, u8 bit_index, double voltage) noexcept;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request, u8 active_domains = 0xFFU) const noexcept;
    void notify_interrupt_state_change(IoPeripheral* peripheral, bool pending) noexcept;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request, u8 active_domains = 0xFFU) noexcept;
    void on_reti() noexcept;
    [[nodiscard]] bool twi_broadcast(u8 address) const noexcept;
    void set_flash_rww_busy(bool busy) noexcept { flash_rww_busy_ = busy; }
    [[nodiscard]] bool flash_rww_busy() const noexcept { return flash_rww_busy_; }
    [[nodiscard]] bool should_stall_cpu(u32 pc_word) const noexcept;
    void request_cpu_stall(u32 cycles) const noexcept;
    void consume_stall_cycle() const noexcept;

    void set_xmem(class Xmem* xmem) noexcept { xmem_ = xmem; }
    [[nodiscard]] class Xmem* xmem() const noexcept { return xmem_; }
    void set_nvm_ctrl(class NvmCtrl* nvm_ctrl) noexcept;
    [[nodiscard]] class NvmCtrl* nvm_ctrl() const noexcept { return nvm_ctrl_; }
    void set_eeprom(class Eeprom* eeprom) noexcept { eeprom_ = eeprom; }
    [[nodiscard]] class Eeprom* eeprom() const noexcept { return eeprom_; }
    void set_cpu_int(class CpuInt* cpu_int) noexcept { cpu_int_ = cpu_int; }
    [[nodiscard]] class CpuInt* cpu_int() const noexcept { return cpu_int_; }

    void set_flash_wait_states(u8 states) noexcept { flash_wait_states_ = states; }
    [[nodiscard]] u8 flash_wait_states() const noexcept { return flash_wait_states_; }

    void set_zcd(class Zcd* zcd) noexcept { zcd_ = zcd; }
    [[nodiscard]] class Zcd* zcd() const noexcept { return zcd_; }
    
    [[nodiscard]] const DeviceDescriptor& device() const noexcept { return device_; }
    [[nodiscard]] DeviceDescriptor& device() noexcept { return device_; }
    
    [[nodiscard]] IoPeripheral* get_peripheral_by_name(std::string_view name) noexcept;
    
    void set_lockbit(u8 value) noexcept { lockbit_ = value; }
    [[nodiscard]] u8 lockbit() const noexcept { return lockbit_; }
    
    void execute_nvm_command(u8 command, u32 address, u16 data) noexcept;
    void request_analysis_freeze() noexcept { analysis_freeze_requested_ = true; }
    [[nodiscard]] bool analysis_freeze_requested() const noexcept { return analysis_freeze_requested_; }
    void clear_analysis_freeze_request() noexcept { analysis_freeze_requested_ = false; }

    [[nodiscard]] u8 get_wait_states(u16 address) const noexcept;

    [[nodiscard]] bool in_rmw() const noexcept { return in_rmw_; }
    void set_in_rmw(bool value) noexcept { in_rmw_ = value; }

    [[nodiscard]] inline bool interrupts_dirty() const noexcept { return interrupts_dirty_; }
    inline void clear_interrupts_dirty() noexcept { interrupts_dirty_ = false; }
    inline void set_interrupts_dirty() noexcept { interrupts_dirty_ = true; }

private:
    [[nodiscard]] inline IoPeripheral* find_peripheral(u16 address) noexcept
    {
        return dispatch_table_[address];
    }
    [[nodiscard]] inline const IoPeripheral* find_peripheral(u16 address) const noexcept
    {
        return dispatch_table_[address];
    }

    DeviceDescriptor device_;
    ITraceHook* trace_hook_ {};
    PinMap* pin_map_ {};
    class PinMux* pin_mux_ {};
    std::vector<u16> flash_;
    std::vector<u8> data_;
    bool flash_rww_busy_ {false};
    bool has_pending_pin_changes_ {false};
    Xmem* xmem_ {nullptr};
    class NvmCtrl* nvm_ctrl_ {nullptr};
    class Eeprom* eeprom_ {nullptr};
    class CpuInt* cpu_int_ {nullptr};
    class Zcd* zcd_ {nullptr};
    uint64_t pending_interrupt_mask_ {0U};
    bool all_peripherals_support_mask_ {true};
    uint64_t mask_support_bits_ {0ULL};
    std::vector<IoPeripheral*> peripherals_ {};
    std::vector<IoPeripheral*> ticking_peripherals_ {};
    std::vector<IoPeripheral*> active_ticking_peripherals_ {};
    std::array<IoPeripheral*, 65536> dispatch_table_ {};
    u32 loaded_program_words_ {};
    u32 reset_word_address_ {};
    std::vector<u16> flash_page_buffer_;
    std::vector<u8> eeprom_page_buffer_;
    std::vector<u8> user_row_;
    std::vector<std::unique_ptr<IoPeripheral>> managed_muxes_;
    std::array<u8, 16> fuses_ {};
    void perform_deferred_nvm_operation() noexcept;
    u8 lockbit_ {0xFFU};

    // SPM timing state
    u32 spm_busy_cycles_left_ {0U};
    mutable u32 io_stall_cycles_ {0U};
    u8 spm_command_ {0U};
    u32 spm_address_ {0U};
    u16 spm_data_ {0U};
    u8 lpm_special_mode_timeout_ {0U};
    u8 last_spmcsr_val_ {0U};
    u8 flash_wait_states_ {0U};
    u64 cpu_cycles_ {0U};
    std::array<u64, 256> domain_gated_cycles_ {};
    bool analysis_freeze_requested_ {false};
    bool in_rmw_ {false};
    bool interrupts_dirty_ {true};
    u8 current_active_domains_ {0xFFU};
    void catch_up_all_peripherals(u64 target_cycle) const noexcept;
    EventScheduler scheduler_;
};
}  // namespace vioavr::core

namespace vioavr::core {

inline u8 MemoryBus::read_data(const u16 address) noexcept
{
    catch_up_all_peripherals(cpu_cycles());
    // 1. Unified Memory Map aliases (AVR8X)
    if (device_.mapped_flash.size > 0 && 
        address >= device_.mapped_flash.data_start && 
        address < device_.mapped_flash.data_start + device_.mapped_flash.size) {
        const u32 offset = address - device_.mapped_flash.data_start;
        const u32 word_addr = offset >> 1;
        if (word_addr < flash_.size()) {
            const u16 word = flash_[word_addr];
            return (offset & 1) ? static_cast<u8>(word >> 8) : static_cast<u8>(word & 0xFF);
        }
    } else if (device_.mapped_fuses.size > 0 &&
        address >= device_.mapped_fuses.data_start &&
        address < device_.mapped_fuses.data_start + device_.mapped_fuses.size) {
        const u32 offset = address - device_.mapped_fuses.data_start;
        return (offset < fuses_.size()) ? fuses_[offset] : 0xFFU;
    } else if (device_.mapped_eeprom.size > 0 &&
        address >= device_.mapped_eeprom.data_start &&
        address < device_.mapped_eeprom.data_start + device_.mapped_eeprom.size) {
        if (eeprom_) {
            return eeprom_->read(address);
        }
        const u32 offset = address - device_.mapped_eeprom.data_start;
        return (offset < eeprom_page_buffer_.size()) ? eeprom_page_buffer_[offset] : 0xFFU;
    } else if (device_.mapped_signatures.size > 0 &&
        address >= device_.mapped_signatures.data_start &&
        address < device_.mapped_signatures.data_start + device_.mapped_signatures.size) {
        const u32 offset = address - device_.mapped_signatures.data_start;
        return (offset < device_.signature.size()) ? device_.signature[offset] : 0xFFU;
    } else if (device_.mapped_user_signatures.size > 0 &&
        address >= device_.mapped_user_signatures.data_start &&
        address < device_.mapped_user_signatures.data_start + device_.mapped_user_signatures.size) {
        const u32 offset = address - device_.mapped_user_signatures.data_start;
        return (offset < user_row_.size()) ? user_row_[offset] : 0xFFU;
    }

    // 2. High-performance path for Registers/Peripherals
    if (IoPeripheral* peripheral = dispatch_table_[address]; peripheral != nullptr) {
        return peripheral->read(address);
    }

    // 3. SRAM / General Data Space
    if (address < data_.size()) {
        u8 value = data_[address];
        // Dynamic bits for SPMCSR (Classic Mega only)
        if (device_.spmcsr_address != 0U && address == device_.spmcsr_address) {
            if (lpm_special_mode_timeout_ > 0U) {
                value = last_spmcsr_val_;
            }
            if (spm_busy_cycles_left_ > 0U) {
                value |= device_.spmen_mask != 0 ? device_.spmen_mask : 0x01U;
            }
            if (flash_rww_busy_) {
                value |= 0x40U; // RWWSB
            }
        }
        return value;
    }

    if (xmem_ != nullptr) {
        return xmem_->read_external(address);
    }

    return 0xFFU;
}

inline void MemoryBus::write_data(const u16 address, const u8 value) noexcept
{
    catch_up_all_peripherals(cpu_cycles());
    // 1. Update Unified Memory Map aliases (Shadow Buffers for NVM commands)
    if (device_.mapped_flash.size > 0 && 
        address >= device_.mapped_flash.data_start && 
        address < device_.mapped_flash.data_start + device_.mapped_flash.size) {
        const u32 offset = address - device_.mapped_flash.data_start;
        const u32 word_addr = offset >> 1;
        const u32 page_offset = word_addr % device_.flash_page_size;
        
        if (page_offset < flash_page_buffer_.size()) {
            u16 word = flash_page_buffer_[page_offset];
            if (offset & 1) {
                word = (word & 0x00FFU) | (static_cast<u16>(value) << 8U);
            } else {
                word = (word & 0xFF00U) | static_cast<u16>(value);
            }
            flash_page_buffer_[page_offset] = word;
            if (nvm_ctrl_) nvm_ctrl_->set_address(address);
            return;
        }
    }

    if (device_.mapped_eeprom.size > 0 &&
        address >= device_.mapped_eeprom.data_start &&
        address < device_.mapped_eeprom.data_start + device_.mapped_eeprom.size) {
        request_cpu_stall(54400U);
        const u32 offset = address - device_.mapped_eeprom.data_start;
        if (offset < eeprom_page_buffer_.size()) {
            eeprom_page_buffer_[offset] = value;
            if (nvm_ctrl_) nvm_ctrl_->set_address(address);
        }
        return;
    }

    if (device_.mapped_user_signatures.size > 0 &&
        address >= device_.mapped_user_signatures.data_start &&
        address < device_.mapped_user_signatures.data_start + device_.mapped_user_signatures.size) {
        const u32 offset = address - device_.mapped_user_signatures.data_start;
        if (offset < user_row_.size()) {
            user_row_[offset] = value;
            if (nvm_ctrl_) nvm_ctrl_->set_address(address);
            return;
        }
    }
    const bool is_prr_write = ((device_.prr_address != 0U && address == device_.prr_address) ||
                               (device_.prr0_address != 0U && address == device_.prr0_address) ||
                               (device_.prr1_address != 0U && address == device_.prr1_address));
    if (is_prr_write) {
        for (IoPeripheral* p : peripherals_) {
            if (p != nullptr) {
                p->sync();
            }
        }
    }

    // 2. High-performance path for Registers/Peripherals
    if (IoPeripheral* peripheral = dispatch_table_[address]; peripheral != nullptr) {
        peripheral->write(address, value);
        if (is_prr_write) {
            for (IoPeripheral* p : peripherals_) {
                if (p != nullptr) {
                    p->on_power_state_change();
                }
            }
        }
        return;
    }

    // 3. SRAM / General Data Space
    if (address < data_.size()) {
        data_[address] = value;
        // Handle SPMCSR (Classic Mega) - Shadow state update
        if (device_.spmcsr_address != 0U && address == device_.spmcsr_address) {
            if ((value & 0x10U) != 0U) {
                flash_rww_busy_ = false;
            }
            const u8 special_mask = device_.sigrd_mask | device_.blbset_mask;
            if (special_mask != 0 && (value & special_mask) != 0U) {
                lpm_special_mode_timeout_ = 5U;
                last_spmcsr_val_ = value;
            }
        }
        return;
    }

    if (xmem_ != nullptr) {
        xmem_->write_external(address, value);
    }
}

inline u8 MemoryBus::get_wait_states(const u16 address) const noexcept
{
    // Fast path: internal memory has 0 wait states
    if (address < data_.size()) return 0U;
    return (xmem_ != nullptr) ? xmem_->get_wait_states(address) : 0U;
}

} // namespace vioavr::core
