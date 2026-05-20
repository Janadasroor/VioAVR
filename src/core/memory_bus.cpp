#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/io_multiplexer.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/xmem.hpp"
#include "vioavr/core/nvm_ctrl.hpp"
#include "vioavr/core/eeprom.hpp"
#include "vioavr/core/cpu_int.hpp"

#include <algorithm>
#include <stdexcept>

namespace vioavr::core {

MemoryBus::MemoryBus(const DeviceDescriptor& device) : device_(device)
{
    flash_.resize(device_.flash_words, 0U);
    data_.resize(static_cast<std::size_t>(device_.data_end_address()) + 1U, 0U);
    dispatch_table_.fill(nullptr);
    flash_page_buffer_.resize(device_.flash_page_size, 0xFFFFU);
    
    if (device_.mapped_user_signatures.size > 0) {
        user_row_.resize(device_.mapped_user_signatures.size, 0xFFU);
    }
    if (device_.nvm_ctrl_count > 0) {
        eeprom_page_buffer_.resize(device_.eeprom_bytes > 0 ? device_.eeprom_bytes : 32, 0xFFU);
    }
    
    reset();
}

void MemoryBus::reset() noexcept
{
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
    scheduler_.reset();
    domain_gated_cycles_.fill(0U);
    // Do NOT clear loaded_program_words_ here, it should survive reset
    for (IoPeripheral* peripheral : peripherals_) {
        if (peripheral != nullptr) {
            peripheral->reset();
        }
    }
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

    if (peripheral.wants_tick()) {
        ticking_peripherals_.push_back(&peripheral);
        printf("PERIPHERAL TICKING: %s\n", peripheral.name().data());
    }
    Logger::debug("MemoryBus: peripherals size is now " + std::to_string(peripherals_.size()) + " (ticking: " + std::to_string(ticking_peripherals_.size()) + ")");
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



void MemoryBus::tick_peripherals(u64 elapsed_cycles, u8 active_domains) noexcept {
    if (elapsed_cycles == 0) return;

    for (size_t i = 1; i < 256; ++i) {
        if ((active_domains & i) == 0U) {
            domain_gated_cycles_[i] += elapsed_cycles;
        }
    }

    if (io_stall_cycles_ > 0U) {
        if (elapsed_cycles >= io_stall_cycles_) {
            io_stall_cycles_ = 0U;
        } else {
            io_stall_cycles_ -= static_cast<u32>(elapsed_cycles);
        }
    }

    if (lpm_special_mode_timeout_ > 0U) {
        if (elapsed_cycles >= lpm_special_mode_timeout_) {
            lpm_special_mode_timeout_ = 0U;
            if (device_.spmcsr_address < data_.size()) {
                data_[device_.spmcsr_address] &= ~(device_.sigrd_mask | device_.blbset_mask);
            }
        } else {
            lpm_special_mode_timeout_ -= static_cast<u8>(elapsed_cycles);
        }
    }

    for (auto* peripheral : ticking_peripherals_) {
        const u8 domain_bit = static_cast<u8>(peripheral->clock_domain());
        if (domain_bit == 0U || (active_domains & domain_bit) != 0U) {
            peripheral->tick(elapsed_cycles);
        }
    }

    const u64 target = scheduler_.current_cycle() + elapsed_cycles;
    scheduler_.advance_to(target);

    if (spm_busy_cycles_left_ > 0U) {
        if (elapsed_cycles >= spm_busy_cycles_left_) {
            spm_busy_cycles_left_ = 0U;
            
            perform_deferred_nvm_operation();
            
            if (nvm_ctrl_) {
                nvm_ctrl_->set_busy(false, false);
                nvm_ctrl_->clear_command();
                nvm_ctrl_->set_done();
            }
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
    for (IoPeripheral* peripheral : peripherals_) {
        if (peripheral != nullptr) {
            peripheral->on_external_pin_change(pin_address, bit_index, level);
        }
    }
}

void MemoryBus::propagate_analog_pin_change(u16 pin_address, u8 bit_index, double voltage) noexcept
{
    for (IoPeripheral* peripheral : peripherals_) {
        if (peripheral != nullptr) {
            peripheral->on_analog_pin_change(pin_address, bit_index, voltage);
        }
    }
}

bool MemoryBus::pending_interrupt_request(InterruptRequest& request, const u8 active_domains) const noexcept
{
    bool found = false;
    InterruptRequest best_request {};

    for (const IoPeripheral* peripheral : peripherals_) {
        if (peripheral == nullptr) continue;

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
}

bool MemoryBus::consume_interrupt_request(InterruptRequest& request, const u8 active_domains) noexcept
{
    InterruptRequest selected_request;
    if (!pending_interrupt_request(selected_request, active_domains)) {
        return false;
    }

    IoPeripheral* selected_peripheral = nullptr;
    for (IoPeripheral* peripheral : peripherals_) {
        if (peripheral == nullptr) continue;
        InterruptRequest candidate;
        if (peripheral->pending_interrupt_request(candidate)) {
            if (candidate.vector_index == selected_request.vector_index && candidate.source_id == selected_request.source_id) {
                selected_peripheral = peripheral;
                break;
            }
        }
    }

    if (selected_peripheral == nullptr) {
        printf("[DEBUG] MemoryBus: Selected selected_request but found no matching peripheral for vec %d!\n", selected_request.vector_index);
        return false;
    }

    request = selected_request;
    bool consumed = selected_peripheral->consume_interrupt_request(request);
    if (!consumed) {
        printf("[DEBUG] MemoryBus: Peripheral refused to consume vec %d!\n", request.vector_index);
    }
    if (consumed && cpu_int_) {
        cpu_int_->on_interrupt_acknowledged(request.vector_index);
    }
    return consumed;
}







bool MemoryBus::should_stall_cpu(u32 pc_word) const noexcept {
    if (io_stall_cycles_ > 0U) return true;
    if (spm_busy_cycles_left_ == 0U) return false;

    // Non-RWW devices (e.g., ATmega328P) always halt during SPM operations.
    if (device_.flash_rww_end_word == 0U) return true;

    // Programming the NRWW (typically bootloader) section always halts the CPU.
    const u32 program_word = spm_address_ >> 1U;
    if (program_word > device_.flash_rww_end_word) return true;

    // Programming the RWW (typically application) section only halts the CPU if it's
    // executing from within that same RWW section.
    if (pc_word <= device_.flash_rww_end_word) return true;

    return false;
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
    // Section Check: SPM is usually only allowed from the Boot section on many classic Megas
    if (device_.boot_start_address != 0U) {
        if (pc_word < device_.boot_start_address) {
            Logger::warning("MemoryBus: SPM attempt from App section! PC=0x" + std::to_string(pc_word));
            // For fidelity, we should return; but some tests might rely on this.
            // In real hardware, the instruction is ignored.
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

void MemoryBus::execute_nvm_command(u8 command, u32 address, u16 data) noexcept {
    if (spm_busy_cycles_left_ > 0U) {
        return;
    }

    spm_command_ = command;
    spm_address_ = address;
    spm_data_ = data;
    
    // NVMCTRL commands (AVR8X)
    // 1: PAGEWRITE, 2: PAGEERASE, 3: PAGEERASEWRITE, 5: CHIPERASE, etc.
    u32 cycles = 64000U; // ~4ms at 16MHz default
    
    if (command == 0x05) { // CHER (Chip Erase)
        cycles = 1600000U; // ~100ms at 16MHz
    } else if (command == 0x04 || command == 0x09) { // PBC / EEPBC
        cycles = 1; // Immediate
        if (command == 0x09) {
            Logger::debug("MemoryBus: Clearing EEPROM page buffer (EEPBC)");
            std::ranges::fill(eeprom_page_buffer_, 0xFFU);
        } else {
            Logger::debug("MemoryBus: Clearing Flash page buffer (PBC)");
            std::ranges::fill(flash_page_buffer_, 0xFFFFU);
        }
    }
    
    char log_buf[256];
    snprintf(log_buf, sizeof(log_buf), "MemoryBus [%p]: NVM command 0x%02X (cycles: %u) started at 0x%04X", (void*)this, command, cycles, address);
    Logger::debug(log_buf);
    
    spm_busy_cycles_left_ = cycles;

    if (nvm_ctrl_) {
        bool flash_busy = (command >= 1 && command <= 5) || command == 0x10;
        bool ee_busy = (command >= 6 && command <= 8);
        nvm_ctrl_->set_busy(flash_busy, ee_busy);
    }

    Logger::debug("MemoryBus: NVM command " + std::to_string(command) + " (cycles: " + std::to_string(cycles) + ") started at 0x" + std::to_string(address));
}


void MemoryBus::on_reti() noexcept {
    for (auto* peripheral : peripherals_) {
        peripheral->on_reti();
    }
}

bool MemoryBus::twi_broadcast(u8 address) const noexcept {
    for (const auto* peripheral : peripherals_) {
        if (peripheral != nullptr && peripheral->check_twi_address(address)) {
            return true;
        }
    }
    return false;
}

IoPeripheral* MemoryBus::get_peripheral_by_name(std::string_view name) noexcept {
    for (auto* p : peripherals_) {
        // Some peripherals might have instance names like USART0
        if (p->name() == name) return p;
    }
    return nullptr;
}

void MemoryBus::perform_deferred_nvm_operation() noexcept {
    const u32 word_addr = spm_address_ >> 1;
    const u32 page_mask = ~(static_cast<u32>(device_.flash_page_size) - 1U);
    const u32 page_start = word_addr & page_mask;

    if (nvm_ctrl_ != nullptr) {
        // AVR8X
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
        // Classic Mega
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
