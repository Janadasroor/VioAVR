#include "vioavr/core/memory_bus.hpp"
#include <cstdio>
#include "vioavr/core/io_multiplexer.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/xmem.hpp"
#include "vioavr/core/ebi.hpp"
#include "vioavr/core/nvm_ctrl.hpp"
#include "vioavr/core/eeprom.hpp"
#include "vioavr/core/cpu_int.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>

namespace vioavr::core {

MemoryBus::MemoryBus(const DeviceDescriptor& device) : device_(device)
{
    // Erased flash reads as 0xFF, not 0x00: the flash ships unprogrammed
    // and reads 0xFFFF (datasheet).
    flash_.resize(device_.flash_words, 0xFFFFU);
    data_.resize(static_cast<std::size_t>(device_.data_end_address()) + 1U, 0U);
    dispatch_table_.fill(nullptr);
    flash_page_buffer_.resize(device_.flash_page_size, 0xFFFFU);
    
    if (device_.mapped_user_signatures.size > 0) {
        user_row_.resize(device_.mapped_user_signatures.size, 0xFFU);
    }
    if (device_.nvm_ctrl_count > 0) {
        eeprom_page_buffer_.resize(device_.eeprom_bytes > 0 ? device_.eeprom_bytes : 32, 0xFFU);
        eeprom_buffer_dirty_.assign(eeprom_page_buffer_.size(), false);
    }
    // Devices without an EEPOM peripheral (AVR-Dx/Ex) keep the EEPROM array
    // in the bus so that NVM erase/write commands have observable effects.
    if (device_.mapped_eeprom.size > 0) {
        const u32 size = device_.eeprom_bytes > device_.mapped_eeprom.size
            ? device_.eeprom_bytes : device_.mapped_eeprom.size;
        eeprom_storage_.assign(size, 0xFFU);
    }
    
    reset();
}

void MemoryBus::reset() noexcept
{
    active_ticking_peripherals_ = ticking_peripherals_;
    // Only clear SRAM to preserve EEPROM/User Row if they are mapped to data space
    const auto sram = device_.sram_range();
    if (sram.begin < data_.size()) {
        const u16 end = std::min<u16>(sram.end, static_cast<u16>(data_.size() - 1));
        std::fill(data_.begin() + sram.begin, data_.begin() + end + 1, 0U);
    }
    
    // Also clear IO register range to be safe (usually registers have their own reset, but mirrors in data_ should be cleared)
    if (device_.register_file_range.end < data_.size()) {
        std::fill(data_.begin(), data_.begin() + device_.register_file_range.end + 1, 0U);
    }
    if (device_.io_range.begin < data_.size()) {
        const u16 io_end = std::min<u16>(device_.extended_io_range.end, static_cast<u16>(data_.size() - 1));
        std::fill(data_.begin() + device_.io_range.begin, data_.begin() + io_end + 1, 0U);
    }

    fuses_ = device_.fuses;
    lockbit_ = device_.lockbit_reset;
    spm_busy_cycles_left_ = 0U;
    flash_rww_busy_ = false;
    spm_blocks_cpu_ = true;
    scheduler_.reset();
    domain_gated_cycles_.fill(0U);
    cpu_cycles_ = 0U;
    current_active_domains_ = 0xFFU;
    for (IoPeripheral* peripheral : peripherals_) {
        if (peripheral != nullptr) {
            peripheral->reset();
            peripheral->last_update_cycle_ = 0U;
        }
    }
    interrupts_dirty_ = true;
    last_catch_up_cycle_ = 0U;
}

void MemoryBus::set_trace_hook(ITraceHook* trace_hook) noexcept
{
    trace_hook_ = trace_hook;
}

void MemoryBus::set_pin_map(PinMap* pin_map) noexcept
{
    pin_map_ = pin_map;
}

void MemoryBus::attach_peripheral(IoPeripheral& peripheral)
{
    Logger::debug("Attaching peripheral to memory bus: " + std::string(peripheral.name()) + " this=" + Logger::hex((uintptr_t)&peripheral));
    peripherals_.push_back(&peripheral);
    peripheral.bus_id_ = static_cast<u8>(peripherals_.size() - 1);
    if (peripheral.supports_interrupt_mask()) {
        mask_support_bits_ |= (1ULL << peripheral.bus_id_);
    } else {
        all_peripherals_support_mask_ = false;
    }

    if (peripheral.wants_tick()) {
        ticking_peripherals_.push_back(&peripheral);
        active_ticking_peripherals_.push_back(&peripheral);
    }
    Logger::debug("MemoryBus: peripherals size is now " + std::to_string(peripherals_.size()) + " (ticking: " + std::to_string(ticking_peripherals_.size()) + ")");
    peripheral.bus_ = this;
    peripheral.set_memory_bus(this);

    // Auto-identify special peripherals
    if (peripheral.name() == "NVMCTRL") {
        nvm_ctrl_ = static_cast<NvmCtrl*>(&peripheral);
    } else if (peripheral.name() == "EEPROM") {
        eeprom_ = static_cast<Eeprom*>(&peripheral);
    } else if (peripheral.name() == "CPUINT") {
        cpu_int_ = static_cast<CpuInt*>(&peripheral);
    }

    for (const auto& range : peripheral.mapped_ranges()) {
        char buf[128];
        snprintf(buf, sizeof(buf), "[0x%04X, 0x%04X]", range.begin, range.end);
        Logger::debug("Mapping peripheral '" + std::string(peripheral.name()) + "' to range " + buf);
        for (u32 addr = range.begin; addr <= range.end; ++addr) {
            if (dispatch_table_[addr] != nullptr && dispatch_table_[addr] != &peripheral) {
                // Address already mapped! Create or update a multiplexer.
                auto* existing = dispatch_table_[addr];
                if (existing->name().starts_with("MUX_")) {
                    static_cast<IoMultiplexer*>(existing)->add(&peripheral);
                } else {
                    auto mux = std::make_unique<IoMultiplexer>("MUX_" + std::to_string(addr));
                    mux->add(existing);
                    mux->add(&peripheral);
                    dispatch_table_[addr] = mux.get();
                    managed_muxes_.push_back(std::move(mux));
                }
            } else {
                dispatch_table_[addr] = &peripheral;
            }
        }
    }
}

void MemoryBus::register_ticking_peripheral(IoPeripheral& peripheral) noexcept
{
    if (std::find(ticking_peripherals_.begin(), ticking_peripherals_.end(), &peripheral) == ticking_peripherals_.end()) {
        ticking_peripherals_.push_back(&peripheral);
    }
    if (std::find(active_ticking_peripherals_.begin(), active_ticking_peripherals_.end(), &peripheral) == active_ticking_peripherals_.end()) {
        active_ticking_peripherals_.push_back(&peripheral);
    }
}

void MemoryBus::set_peripheral_active(IoPeripheral* peripheral, bool active) noexcept
{
    if (!peripheral->wants_tick()) return;

    auto it = std::find(active_ticking_peripherals_.begin(), active_ticking_peripherals_.end(), peripheral);
    bool currently_active = (it != active_ticking_peripherals_.end());

    if (active == currently_active) return;

    u64 current = cpu_cycles();
    u64 start = peripheral->last_update_cycle_;

    if (active) {
        // Transitioning from INACTIVE to ACTIVE:
        // Do NOT tick for the inactive period, but update the last update cycle to current.
        peripheral->last_update_cycle_ = current;
        active_ticking_peripherals_.push_back(peripheral);
    } else {
        // Transitioning from ACTIVE to INACTIVE:
        // Tick up to the current cycle before turning off!
        if (start < current) {
            peripheral->last_update_cycle_ = current;
            const u8 domain_bit = static_cast<u8>(peripheral->clock_domain());
            if (domain_bit == 0U || (current_active_domains_ & domain_bit) != 0U) {
                peripheral->tick(current - start);
            }
        }
        active_ticking_peripherals_.erase(it);
    }
}

void MemoryBus::load_flash(std::span<const u16> words)
{
    const std::size_t count = std::min(words.size(), flash_.size());
    std::copy_n(words.begin(), count, flash_.begin());
    loaded_program_words_ = static_cast<u32>(count);
    Logger::debug("MemoryBus: loaded " + std::to_string(count) + " words");
}

void MemoryBus::load_image(const HexImage& image)
{
    load_flash(image.flash_words);
    reset_word_address_ = image.entry_word;
}



void MemoryBus::catch_up_all_peripherals(u64 target_cycle) const noexcept {
    if (target_cycle <= last_catch_up_cycle_) return;
    last_catch_up_cycle_ = target_cycle;
    for (auto* peripheral : active_ticking_peripherals_) {
        u64 start = peripheral->last_update_cycle_;
        if (start < target_cycle) {
            peripheral->last_update_cycle_ = target_cycle;
            const u8 domain_bit = static_cast<u8>(peripheral->clock_domain());
            if (domain_bit == 0U || (current_active_domains_ & domain_bit) != 0U) {
                peripheral->tick(target_cycle - start);
            }
        }
    }
}

void MemoryBus::tick_peripherals(u64 elapsed_cycles, u8 active_domains) noexcept {
    if (elapsed_cycles == 0) return;

    if (__builtin_expect(active_domains != 0xFF, 0)) {
        u8 gated = static_cast<u8>(~active_domains);
        do {
            int bit = __builtin_ctz(static_cast<unsigned>(gated));
            u8 mask = static_cast<u8>(1U << bit);
            domain_gated_cycles_[mask] += elapsed_cycles;
            gated = static_cast<u8>(gated & static_cast<u8>(gated - 1U));
        } while (gated != 0U);
    }

    if (io_stall_cycles_ > 0U) {
        if (elapsed_cycles >= io_stall_cycles_) io_stall_cycles_ = 0U;
        else io_stall_cycles_ -= static_cast<u32>(elapsed_cycles);
    }

    if (lpm_special_mode_timeout_ > 0U) {
        if (elapsed_cycles >= lpm_special_mode_timeout_) {
            lpm_special_mode_timeout_ = 0U;
            if (device_.spmcsr_address < data_.size())
                data_[device_.spmcsr_address] &= ~(device_.sigrd_mask | device_.blbset_mask);
        } else {
            lpm_special_mode_timeout_ -= static_cast<u8>(elapsed_cycles);
        }
    }

    current_active_domains_ = active_domains;
    cpu_cycles_ += elapsed_cycles;
    const u64 target = scheduler_.current_cycle() + elapsed_cycles;

    for (auto* peripheral : active_ticking_peripherals_) {
        const u64 start = peripheral->last_update_cycle_;
        if (start < target) {
            peripheral->last_update_cycle_ = target;
            const u8 domain_bit = static_cast<u8>(peripheral->clock_domain());
            if (domain_bit == 0U || (active_domains & domain_bit) != 0U) {
                peripheral->tick(target - start);
            }
        }
    }

    scheduler_.advance_to(target, [this](u64 event_cycle) {
        for (auto* peripheral : active_ticking_peripherals_) {
            const u64 start = peripheral->last_update_cycle_;
            if (start < event_cycle) {
                peripheral->last_update_cycle_ = event_cycle;
                const u8 domain_bit = static_cast<u8>(peripheral->clock_domain());
                if (domain_bit == 0U || (current_active_domains_ & domain_bit) != 0U) {
                    peripheral->tick(event_cycle - start);
                }
            }
        }
    });

    if (spm_busy_cycles_left_ > 0U) {
        if (elapsed_cycles >= spm_busy_cycles_left_) {
            spm_busy_cycles_left_ = 0U;
            perform_deferred_nvm_operation();
            if (nvm_ctrl_) { nvm_ctrl_->set_busy(false, false); nvm_ctrl_->clear_command(); nvm_ctrl_->set_done(); }
        } else {
            spm_busy_cycles_left_ -= static_cast<u32>(elapsed_cycles);
        }
    }
}

bool MemoryBus::consume_pin_change(PinStateChange& change) noexcept
{
    if (!has_pending_pin_changes_) return false;

    for (IoPeripheral* peripheral : peripherals_) {
        if (peripheral != nullptr && peripheral->consume_pin_change(change)) {
            return true;
        }
    }
    has_pending_pin_changes_ = false;
    return false;
}

void MemoryBus::propagate_external_pin_change(u16 pin_address, u8 bit_index, PinLevel level) noexcept
{
    catch_up_all_peripherals(cpu_cycles());
    for (IoPeripheral* peripheral : peripherals_) {
        if (peripheral != nullptr) {
            peripheral->on_external_pin_change(pin_address, bit_index, level);
        }
    }
}

void MemoryBus::propagate_analog_pin_change(u16 pin_address, u8 bit_index, double voltage) noexcept
{
    catch_up_all_peripherals(cpu_cycles());
    for (IoPeripheral* peripheral : peripherals_) {
        if (peripheral != nullptr) {
            peripheral->on_analog_pin_change(pin_address, bit_index, voltage);
        }
    }
}

bool MemoryBus::pending_interrupt_request(InterruptRequest& request, const u8 active_domains) const noexcept
{
    // Super Fast Path: if all peripherals support the mask and no interrupts are pending, return false immediately!
    if (all_peripherals_support_mask_ && pending_interrupt_mask_ == 0ULL) {
        return false;
    }

    bool found = false;
    InterruptRequest best_request {};

    for (const IoPeripheral* peripheral : peripherals_) {
        if (peripheral == nullptr) continue;

        const u8 bus_id = peripheral->bus_id_;
        // If it supports the mask, and its bit is not set, we can skip it entirely!
        if ((mask_support_bits_ & (1ULL << bus_id)) != 0) {
            if ((pending_interrupt_mask_ & (1ULL << bus_id)) == 0) {
                continue;
            }
        }

        const u8 domain_mask = static_cast<u8>(peripheral->clock_domain());
        if ((domain_mask & active_domains) == 0 && domain_mask != 0) {
            continue;
        }

        InterruptRequest candidate {};
        if (peripheral->pending_interrupt_request(candidate)) {
            bool replace = false;
            if (!found) {
                replace = true;
            } else if (cpu_int_) {
                // CPUINT Priority (AVR8X)
                bool candidate_is_lvl1 = cpu_int_->is_lvl1_vector(candidate.vector_index);
                bool best_is_lvl1 = cpu_int_->is_lvl1_vector(best_request.vector_index);

                if (candidate_is_lvl1 && !best_is_lvl1) {
                    replace = true;
                } else if (!candidate_is_lvl1 && best_is_lvl1) {
                    replace = false;
                } else {
                    // Same level (either both Level 1 or both Level 0)
                    u8 base = cpu_int_->highest_priority_vector();
                    u32 n = static_cast<u32>(device_.interrupt_vector_count);
                    
                    if (n > 0) {
                        u32 p_candidate = (static_cast<u32>(candidate.vector_index) + n - base) % n;
                        u32 p_best = (static_cast<u32>(best_request.vector_index) + n - base) % n;

                        if (p_candidate < p_best) {
                            replace = true;
                        }
                    } else {
                        // Fallback to legacy
                        if (candidate.vector_index < best_request.vector_index) {
                            replace = true;
                        }
                    }
                }
            } else {
                // Legacy Priority (Lowest index wins)
                if (candidate.vector_index < best_request.vector_index ||
                    (candidate.vector_index == best_request.vector_index && candidate.source_id < best_request.source_id)) {
                    replace = true;
                }
            }

            if (replace) {
                best_request = candidate;
                found = true;
            }
        }
    }

    if (found) {
        request = best_request;
    }
    return found;
}

void MemoryBus::notify_interrupt_state_change(IoPeripheral* peripheral, bool pending) noexcept {
    if (peripheral->bus_id_ >= 64) return; // Only first 64 peripherals supported for mask
    if (pending) {
        pending_interrupt_mask_ |= (1ULL << peripheral->bus_id_);
    } else {
        pending_interrupt_mask_ &= ~(1ULL << peripheral->bus_id_);
    }
    interrupts_dirty_ = true;
}

bool MemoryBus::consume_interrupt_request(InterruptRequest& request, const u8 active_domains) noexcept
{
    if (all_peripherals_support_mask_ && pending_interrupt_mask_ == 0ULL) {
        return false;
    }

    bool found = false;
    InterruptRequest best_request {};
    IoPeripheral* best_peripheral = nullptr;

    for (IoPeripheral* peripheral : peripherals_) {
        if (peripheral == nullptr) continue;

        const u8 bus_id = peripheral->bus_id_;
        if ((mask_support_bits_ & (1ULL << bus_id)) != 0) {
            if ((pending_interrupt_mask_ & (1ULL << bus_id)) == 0) {
                continue;
            }
        }

        const u8 domain_mask = static_cast<u8>(peripheral->clock_domain());
        if ((domain_mask & active_domains) == 0 && domain_mask != 0) {
            continue;
        }

        InterruptRequest candidate {};
        if (peripheral->pending_interrupt_request(candidate)) {
            bool replace = false;
            if (!found) {
                replace = true;
            } else if (cpu_int_) {
                bool candidate_is_lvl1 = cpu_int_->is_lvl1_vector(candidate.vector_index);
                bool best_is_lvl1 = cpu_int_->is_lvl1_vector(best_request.vector_index);

                if (candidate_is_lvl1 && !best_is_lvl1) {
                    replace = true;
                } else if (!candidate_is_lvl1 && best_is_lvl1) {
                    replace = false;
                } else {
                    u8 base = cpu_int_->highest_priority_vector();
                    u32 n = static_cast<u32>(device_.interrupt_vector_count);

                    if (n > 0) {
                        u32 p_candidate = (static_cast<u32>(candidate.vector_index) + n - base) % n;
                        u32 p_best = (static_cast<u32>(best_request.vector_index) + n - base) % n;

                        if (p_candidate < p_best) {
                            replace = true;
                        }
                    } else {
                        if (candidate.vector_index < best_request.vector_index) {
                            replace = true;
                        }
                    }
                }
            } else {
                if (candidate.vector_index < best_request.vector_index ||
                    (candidate.vector_index == best_request.vector_index && candidate.source_id < best_request.source_id)) {
                    replace = true;
                }
            }

            if (replace) {
                best_request = candidate;
                best_peripheral = peripheral;
                found = true;
            }
        }
    }

    if (!found) {
        return false;
    }

    request = best_request;
    bool consumed = best_peripheral->consume_interrupt_request(request);
    if (consumed && cpu_int_) {
        cpu_int_->on_interrupt_acknowledged(request.vector_index);
    }
    return consumed;
}







void MemoryBus::request_cpu_stall(u32 cycles) const noexcept {
    io_stall_cycles_ = std::max(io_stall_cycles_, cycles);
}

void MemoryBus::consume_stall_cycle() const noexcept {
    if (io_stall_cycles_ > 0U) {
        io_stall_cycles_--;
    }
}

void MemoryBus::execute_spm(const u8 command, const u32 address, const u16 data, u32 pc_word) noexcept {
    (void)pc_word;
    // Access control is purely BLB/lockbit-based (datasheet §Self-Programming):
    // SPM may execute from any section when the lockbits permit it. A CPU
    // fetching from the targeted section stalls via should_stall_cpu().
    if (device_.boot_start_address != 0U) {
        const u32 boot_start_bytes = device_.boot_start_address << 1U;
        const u8 lb = lockbit_ & 0x3FU;
        if ((lb & 0x03U) != 0x03U) {
            Logger::warning("MemoryBus: SPM inhibited by lockbits! Lock=0x" + std::to_string(lockbit_));
            return;
        }
        if (address < boot_start_bytes) {
            if ((lb & 0x08U) == 0U) {
                Logger::warning("MemoryBus: SPM to Application Section inhibited by BLB0!");
                return;
            }
        } else {
            if ((lb & 0x20U) == 0U) {
                Logger::warning("MemoryBus: SPM to Bootloader Section inhibited by BLB1!");
                return;
            }
        }
    }

    if (spm_busy_cycles_left_ > 0U) {
        return;
    }

    if ((command & 0x06U) != 0U) { // Page Erase or Page Write
        spm_command_ = command;
        spm_address_ = address;
        spm_data_ = data;
        spm_busy_cycles_left_ = 64000U;
        spm_blocks_cpu_ = true;
        // RWWSB: an RWW-targeted operation blocks the RWW section (reads
        // return 0xFF) until RWWSRE re-enables it. Surfaces via read_data.
        if (device_.flash_rww_end_word != 0U &&
            (address >> 1U) < device_.flash_rww_end_word) {
            flash_rww_busy_ = true;
        }
        Logger::debug("MemoryBus: SPM timed operation started at 0x" + std::to_string(address));
        return;
    }

    if ((command & 0x10U) != 0U) { // RWWSRE
        flash_rww_busy_ = false;
        if (device_.spmcsr_address != 0U && device_.spmcsr_address < data_.size()) {
            data_[device_.spmcsr_address] &= ~0x1FU;
        }
        return;
    }

    if ((command & 0x01U) != 0U) { // Fill Page Buffer
        const u32 word_addr = address >> 1;
        const u32 offset = word_addr % device_.flash_page_size;
        if (offset < flash_page_buffer_.size()) {
            flash_page_buffer_[offset] = data;
        }
        return;
    }
}

void MemoryBus::set_nvm_ctrl(class NvmCtrl* nvm_ctrl) noexcept { 
    nvm_ctrl_ = nvm_ctrl; 
    if (nvm_ctrl) nvm_ctrl->set_bus(*this);
}

NvmFamily MemoryBus::nvm_family() const noexcept {
    if (nvm_ctrl_ != nullptr) return nvm_ctrl_->family();
    return nvm_family_for_name(device_.name);
}

void MemoryBus::execute_nvm_command(u8 command, u32 address, u16 data) noexcept {
    if (spm_busy_cycles_left_ > 0U) {
        return;
    }

    spm_command_ = command;
    spm_address_ = address;
    spm_data_ = data;
    
    // NVMCTRL commands are family-specific (see NvmFamily in device.hpp);
    // the family is generated into the descriptor from the ATDF command set.
    u32 cycles = 64000U; // ~4ms at 16MHz default
    bool flash_busy = true;
    bool ee_busy = false;
    const NvmFamily family = nvm_family();

    auto clear_flash_buf = [&] {
        Logger::debug("MemoryBus: Clearing Flash page buffer (PBC)");
        std::ranges::fill(flash_page_buffer_, 0xFFFFU);
    };
    auto clear_ee_buf = [&] {
        Logger::debug("MemoryBus: Clearing EEPROM page buffer (EEPBC)");
        std::ranges::fill(eeprom_page_buffer_, 0xFFU);
    };

    if (family == NvmFamily::Dx) {
        if (command == 0x01) { // NOOP
            cycles = 1U;
            flash_busy = false;
        } else if (command >= 0x09U && command <= 0x0DU) { // FLMPERn
            cycles = 64000U * (1U << (command - 0x09U + 1U));
        } else if (command == 0x12 || command == 0x13 || command == 0x18
                || (command >= 0x19U && command <= 0x1DU)) {
            flash_busy = false;
            ee_busy = true;
        } else if (command == 0x20) { // CHER
            cycles = 1600000U;
            ee_busy = true;
        } else if (command == 0x30) { // EECHER
            cycles = 1600000U;
            flash_busy = false;
            ee_busy = true;
        }
    } else if (family == NvmFamily::Ex) {
        if (command == 0x01 || command == 0x0FU || command == 0x1FU) { // NOOP/PBCLR
            cycles = 1U;
            if (command == 0x0FU) clear_flash_buf();
            if (command == 0x1FU) clear_ee_buf();
            flash_busy = false;
        } else if (command >= 0x09U && command <= 0x0DU) { // FLMPERn
            cycles = 64000U * (1U << (command - 0x09U + 1U));
        } else if (command == 0x14 || command == 0x15 || command == 0x17) {
            flash_busy = false;
            ee_busy = true;
        } else if (command == 0x20) { // CHER
            cycles = 1600000U;
            ee_busy = true;
        } else if (command == 0x30) { // EECHER
            cycles = 1600000U;
            flash_busy = false;
            ee_busy = true;
        }
    } else {
        // Legacy megaAVR-0/tiny mapping (unchanged behavior).
        if (command == 0x05) { // CHER (Chip Erase)
            cycles = 1600000U; // ~100ms at 16MHz
        } else if (command == 0x04 || command == 0x09) { // PBC / EEPBC
            cycles = 1; // Immediate
            if (command == 0x09) {
                clear_ee_buf();
            } else {
                clear_flash_buf();
            }
        }
        flash_busy = (command >= 1 && command <= 5) || command == 0x10;
        ee_busy = (command >= 6 && command <= 8);
    }

    char log_buf[256];
    snprintf(log_buf, sizeof(log_buf), "MemoryBus [%p]: NVM command 0x%02X (cycles: %u) started at 0x%04X", (void*)this, command, cycles, address);
    Logger::debug(log_buf);

    spm_busy_cycles_left_ = cycles;
    // Flash operations halt the CPU; pure EEPROM operations only raise
    // EEBUSY and never stall (datasheet §NVMCTRL).
    spm_blocks_cpu_ = flash_busy;

    if (nvm_ctrl_) {
        nvm_ctrl_->set_busy(flash_busy, ee_busy);
    }

    Logger::debug("MemoryBus: NVM command " + std::to_string(command) + " (cycles: " + std::to_string(cycles) + ") started at 0x" + std::to_string(address));
}


void MemoryBus::on_reti() noexcept {
    for (auto* peripheral : peripherals_) {
        peripheral->on_reti();
    }
}

bool MemoryBus::twi_broadcast(u8 address) noexcept {
    // Notify every matching slave (multi-slave bus); ACK if any slave ACKs
    // (wired-AND: a single ACK pulls SDA low).
    bool matched = false;
    bool acked = false;
    for (auto* peripheral : peripherals_) {
        if (peripheral != nullptr && peripheral->check_twi_address(address)) {
            peripheral->on_twi_match(address);
            matched = true;
            acked = acked || peripheral->twi_match_acked();
        }
    }
    return matched && acked;
}

bool MemoryBus::twi_data_send(u8 data) noexcept {
    bool acked = false;
    for (auto* peripheral : peripherals_) {
        if (peripheral != nullptr && peripheral->on_twi_data(data)) {
            acked = true;
        }
    }
    return acked;
}

void MemoryBus::twi_stop_broadcast() noexcept {
    for (auto* peripheral : peripherals_) {
        if (peripheral != nullptr) peripheral->on_twi_stop();
    }
}

IoPeripheral* MemoryBus::get_peripheral_by_name(std::string_view name) noexcept {
    for (auto* p : peripherals_) {
        // Some peripherals might have instance names like USART0
        if (p->name() == name) return p;
    }
    return nullptr;
}

void MemoryBus::perform_deferred_nvm_dx_ex(u8 command) noexcept {
    // AVR-Dx / AVR-Ex NVM commands. EEPROM erase granularity follows the
    // datasheets: Dx EEBER/EEMBERn erase 1/2/4/8/16/32 addressed bytes;
    // Ex EEPER erases only the bytes loaded in the EEPROM page buffer
    // within the 8-byte page (ATDF EEPROM pagesize 0x8 on EA/EB).
    if (device_.flash_page_size == 0U) return;
    const u32 word_addr = spm_address_ >> 1;
    const u32 page_mask = ~(static_cast<u32>(device_.flash_page_size) - 1U);
    const u32 page_start = word_addr & page_mask;
    auto erase_flash_pages = [&](u32 first_page, u32 count) {
        for (u32 p = 0; p < count; ++p) {
            const u32 start = first_page + p * device_.flash_page_size;
            for (u32 i = 0; i < device_.flash_page_size && (start + i) < flash_.size(); ++i) {
                flash_[start + i] = 0xFFFFU;
            }
        }
    };
    auto write_flash_page = [&] {
        if (page_start < flash_.size()) {
            for (u32 i = 0; i < device_.flash_page_size && (page_start + i) < flash_.size(); ++i) {
                flash_[page_start + i] = flash_page_buffer_[i];
            }
        }
    };
    auto commit_eeprom = [&](u32 offset, u32 count) {
        const u32 buf_size = static_cast<u32>(eeprom_page_buffer_.size());
        if (eeprom_) {
            eeprom_->commit_page(offset, std::span<const u8>(
                eeprom_page_buffer_.data() + std::min(offset, buf_size),
                std::min(count, buf_size > std::min(offset, buf_size)
                                 ? buf_size - std::min(offset, buf_size) : 0U)));
        } else {
            for (u32 i = 0; i < count; ++i) {
                const u32 src = offset + i;
                const u32 dst = offset + i;
                if (src < eeprom_page_buffer_.size() && dst < eeprom_storage_.size()) {
                    eeprom_storage_[dst] = eeprom_page_buffer_[src];
                }
            }
        }
    };
    auto erase_eeprom_all = [&] {
        if (eeprom_) {
            eeprom_->erase_all();
        } else {
            std::ranges::fill(eeprom_storage_, 0xFFU);
        }
    };
    // EEPROM offsets are absolute from the mapped EEPROM start, matching
    // the page-buffer indexing used by mapped writes.
    auto ee_base_offset = [&] {
        u32 off = spm_address_;
        if (off >= device_.mapped_eeprom.data_start) {
            off -= device_.mapped_eeprom.data_start;
        }
        return off;
    };
    auto erase_eeprom_bytes = [&](u32 start, u32 count) {
        const u32 buf_size = static_cast<u32>(eeprom_page_buffer_.size());
        if (start >= buf_size) return;
        if (count > buf_size - start) count = buf_size - start;
        if (eeprom_) {
            std::array<u8, 32> ff{};
            ff.fill(0xFFU);
            eeprom_->commit_page(start, std::span<const u8>(ff.data(), count));
        } else {
            for (u32 i = 0; i < count; ++i) {
                if (start + i < eeprom_storage_.size()) eeprom_storage_[start + i] = 0xFFU;
            }
        }
    };
    // "The page buffer used will automatically be cleared after finishing the
    // operation." Clearing also resets the dirty flags that tell which bytes
    // were loaded for an erase.
    auto clear_eeprom_buffer = [&] {
        std::ranges::fill(eeprom_page_buffer_, 0xFFU);
        for (u8& dirty : eeprom_buffer_dirty_) dirty = 0U;
    };

    switch (command) {
        case 0x02: // Dx FLWR: flash page write
        case 0x04: // Ex FLPW: flash page write
            write_flash_page();
            break;
        case 0x05: // Ex FLPERW: erase + write flash page
            erase_flash_pages(page_start, 1U);
            write_flash_page();
            break;
        case 0x08: // FLPER: erase flash page
            erase_flash_pages(page_start, 1U);
            break;
        case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: { // FLMPER2..32
            const u32 pages = 1U << (command - 0x09U + 1U);
            erase_flash_pages(page_start, pages);
            break;
        }
        case 0x0F: // Ex FLPBCLR
            std::ranges::fill(flash_page_buffer_, 0xFFFFU);
            break;
        case 0x12: // Dx EEWR: single byte at ADDR
            commit_eeprom(ee_base_offset(), 1U);
            clear_eeprom_buffer();
            break;
        case 0x13: // Dx EEERWR: erase + write the byte at ADDR
            erase_eeprom_bytes(ee_base_offset(), 1U);
            commit_eeprom(ee_base_offset(), 1U);
            clear_eeprom_buffer();
            break;
        case 0x14: // Ex EEPW: page buffer to the EEPROM page of ADDR
        case 0x15: // Ex EEPERW
            if (command == 0x15U) {
                erase_eeprom_bytes(ee_base_offset() & ~0x07U, 8U);
            }
            commit_eeprom(ee_base_offset() & ~0x07U, 8U); // one EEPROM page
            clear_eeprom_buffer();
            break;
        case 0x17: { // Ex EEPER: erase the page-buffer bytes loaded for this page
            const u32 page = ee_base_offset() & ~0x07U; // 8-byte EEPROM page
            const u32 buf_size = static_cast<u32>(eeprom_page_buffer_.size());
            for (u32 i = 0U; i < 8U; ++i) {
                const u32 idx = page + i;
                if (idx < buf_size && idx < eeprom_buffer_dirty_.size()
                    && eeprom_buffer_dirty_[idx]) {
                    erase_eeprom_bytes(idx, 1U);
                }
            }
            clear_eeprom_buffer();
            break;
        }
        case 0x18: // Dx EEBER: erase the addressed byte
            erase_eeprom_bytes(ee_base_offset(), 1U);
            clear_eeprom_buffer();
            break;
        case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: { // Dx EEMBERn
            const u32 count = 1U << (command - 0x19U + 1U); // 2..32 bytes
            erase_eeprom_bytes(ee_base_offset(), count);
            clear_eeprom_buffer();
            break;
        }
        case 0x30: // EECHER
            erase_eeprom_all();
            clear_eeprom_buffer();
            break;
        case 0x1F: // Ex EEPBCLR
            std::ranges::fill(eeprom_page_buffer_, 0xFFU);
            break;
        case 0x20: // CHER: erase flash + EEPROM (+ user row like mega path)
            erase_flash_pages(0U, static_cast<u32>(flash_.size()) / device_.flash_page_size + 1U);
            erase_eeprom_all();
            break;
        case 0x01: // NOOP
        case 0x00: // NONE/NOCMD
        default:
            break;
    }
}

void MemoryBus::perform_deferred_nvm_operation() noexcept {
    const u32 word_addr = spm_address_ >> 1;
    const u32 page_mask = ~(static_cast<u32>(device_.flash_page_size) - 1U);
    const u32 page_start = word_addr & page_mask;

    if (nvm_ctrl_ != nullptr) {
        // AVR8X. Command numbers are family-specific (see NvmFamily);
        // megaAVR-0/tiny mapping is the legacy table below.
        if (nvm_family() != NvmFamily::MegaTiny) {
            perform_deferred_nvm_dx_ex(spm_command_);
            return;
        }
        switch (spm_command_) {
            case 0x01: // PAGEWRITE
            case 0x03: // PAGEERASEWRITE
                if (page_start < flash_.size()) {
                    for (u32 i = 0; i < device_.flash_page_size && (page_start + i) < flash_.size(); ++i) {
                        flash_[page_start + i] = flash_page_buffer_[i];
                    }
                }
                break;
            case 0x02: // PAGEERASE
                if (page_start < flash_.size()) {
                    for (u32 i = 0; i < device_.flash_page_size && (page_start + i) < flash_.size(); ++i) {
                        flash_[page_start + i] = 0xFFFFU;
                    }
                }
                break;
            case 0x04: // PBC (Page Buffer Clear)
                std::ranges::fill(flash_page_buffer_, 0xFFFFU);
                break;
            case 0x05: // CHER (Chip Erase)
                std::ranges::fill(flash_, 0xFFFFU);
                if (device_.mapped_user_signatures.size > 0) {
                    std::ranges::fill(user_row_, 0xFFU);
                }
                if (eeprom_) {
                    eeprom_->erase_all();
                }
                break;
            case 0x06: // EEER (EEPROM Erase)
            case 0x07: // EEWP (EEPROM Write)
            case 0x08: // EEERWP (EEPROM Erase-Write)
                if (eeprom_) {
                    u32 ee_offset = spm_address_;
                    if (ee_offset >= device_.mapped_eeprom.data_start) {
                        ee_offset -= device_.mapped_eeprom.data_start;
                    }
                    eeprom_->commit_page(ee_offset, eeprom_page_buffer_);
                }
                break;
            case 0x10: // URWP (User Row Write)
                // User row is already updated via mapped access in write_data,
                // which updates the shadow user_row_ buffer.
                break;
            case 0x09: // EEPBC
                std::ranges::fill(eeprom_page_buffer_, 0xFFU);
                break;
            default:
                break;
        }
    } else {
        // Classic Mega — check lockbits before committing flash changes
        if ((spm_command_ & 0x06U) != 0U) {
            const u8 lb = lockbit_ & 0x3FU;
            if ((lb & 0x03U) != 0x03U) {
                Logger::warning("MemoryBus: Deferred NVM blocked by lockbits! Lock=0x" + std::to_string(lockbit_));
                if (device_.spmcsr_address < data_.size()) {
                    data_[device_.spmcsr_address] &= ~0x1FU;
                }
                return;
            }
        }
        if ((spm_command_ & 0x02U) != 0U) { // PGERS
            if (page_start < flash_.size()) {
                for (u32 i = 0; i < device_.flash_page_size && (page_start + i) < flash_.size(); ++i) {
                    flash_[page_start + i] = 0xFFFFU;
                }
            }
            flash_rww_busy_ = true;
        } else if ((spm_command_ & 0x04U) != 0U) { // PGWRT
            if (page_start < flash_.size()) {
                for (u32 i = 0; i < device_.flash_page_size && (page_start + i) < flash_.size(); ++i) {
                    flash_[page_start + i] = flash_page_buffer_[i];
                }
            }
            flash_rww_busy_ = true;
        }
    }

    // Clear SPMCSR bits (Legacy)
    if (device_.spmcsr_address < data_.size()) {
        data_[device_.spmcsr_address] &= ~0x1FU;
    }
}

void MemoryBus::write_program_word(const u32 word_address, const u16 value) noexcept
{
    if (word_address < flash_.size()) {
        flash_[word_address] = value;
        if (word_address + 1U > loaded_program_words_) {
            loaded_program_words_ = word_address + 1U;
        }
    }
}

} // namespace vioavr::core
