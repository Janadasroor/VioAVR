#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/pin_map.hpp"
#include "vioavr/core/tracing.hpp"
#include "vioavr/core/types.hpp"

#include <array>
#include <span>
#include <string_view>
#include <vector>

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

    [[nodiscard]] constexpr u32 flash_size_words() const noexcept
    {
        return static_cast<u32>(flash_.size());
    }

    [[nodiscard]] constexpr std::span<const u8> data_space() const noexcept
    {
        return data_;
    }

    [[nodiscard]] u16 read_program_word(u32 word_address, u32 pc_word = 0xFFFFFFFFU) const noexcept;
    [[nodiscard]] u8 read_program_byte(u32 byte_address, u32 pc_word = 0xFFFFFFFFU) const noexcept;

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

    [[nodiscard]] u8 read_data(u16 address) noexcept;
    void write_data(u16 address, u8 value) noexcept;
    void tick_peripherals(u64 elapsed_cycles, u8 active_domains = 0xFFU) noexcept;
    [[nodiscard]] u64 cpu_cycles() const noexcept { return cpu_cycles_; }
    [[nodiscard]] bool consume_pin_change(PinStateChange& change) noexcept;
    void propagate_external_pin_change(u16 pin_address, u8 bit_index, PinLevel level) noexcept;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request, u8 active_domains = 0xFFU) const noexcept;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request, u8 active_domains = 0xFFU) noexcept;
    void on_reti() noexcept;
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

private:
    [[nodiscard]] IoPeripheral* find_peripheral(u16 address) noexcept;
    [[nodiscard]] const IoPeripheral* find_peripheral(u16 address) const noexcept;

    DeviceDescriptor device_;
    ITraceHook* trace_hook_ {};
    PinMap* pin_map_ {};
    class PinMux* pin_mux_ {};
    std::vector<u16> flash_;
    std::vector<u8> data_;
    bool flash_rww_busy_ {false};
    Xmem* xmem_ {nullptr};
    class NvmCtrl* nvm_ctrl_ {nullptr};
    class Eeprom* eeprom_ {nullptr};
    class CpuInt* cpu_int_ {nullptr};
    class Zcd* zcd_ {nullptr};
    std::vector<IoPeripheral*> peripherals_ {};
    std::vector<IoPeripheral*> dispatch_table_ {};
    u32 loaded_program_words_ {};
    u32 reset_word_address_ {};
    std::vector<u16> flash_page_buffer_;
    std::vector<u8> eeprom_page_buffer_;
    std::vector<u8> user_row_;
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
    bool analysis_freeze_requested_ {false};
};
}  // namespace vioavr::core
