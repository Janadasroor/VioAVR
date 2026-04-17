#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/xmem.hpp"
#include "vioavr/core/nvm_ctrl.hpp"

#include <algorithm>
#include <stdexcept>

namespace vioavr::core {

MemoryBus::MemoryBus(const DeviceDescriptor& device) : device_(device)
{
    flash_.resize(device_.flash_words, 0U);
    data_.resize(static_cast<std::size_t>(device_.data_end_address()) + 1U, 0U);
    dispatch_table_.resize(data_.size(), nullptr);
    flash_page_buffer_.resize(device_.flash_page_size, 0xFFFFU);
    reset();
}

void MemoryBus::reset() noexcept
{
    std::ranges::fill(data_, 0U);
    fuses_ = device_.fuses;
    spm_busy_cycles_left_ = 0U;
    flash_rww_busy_ = false;
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
    Logger::debug("Attaching peripheral to memory bus: " + std::string(peripheral.name()));
    peripherals_.push_back(&peripheral);
    Logger::debug("MemoryBus: peripherals size is now " + std::to_string(peripherals_.size()));
    for (const auto& range : peripheral.mapped_ranges()) {
        char buf[128];
        snprintf(buf, sizeof(buf), "[0x%04X, 0x%04X]", range.begin, range.end);
        Logger::debug("Mapping peripheral '" + std::string(peripheral.name()) + "' to range " + buf);
        for (u32 addr = range.begin; addr <= range.end && addr < dispatch_table_.size(); ++addr) {
            dispatch_table_[addr] = &peripheral;
        }
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

u8 MemoryBus::read_data(const u16 address) noexcept
{
    u8 value = 0U;

    // 1. Check for Unified Memory Map aliases
    if (device_.mapped_flash.size > 0 && 
        address >= device_.mapped_flash.data_start && 
        address < device_.mapped_flash.data_start + device_.mapped_flash.size) {
        const u32 offset = address - device_.mapped_flash.data_start;
        const u32 word_addr = offset >> 1;
        if (word_addr < flash_.size()) {
            const u16 word = flash_[word_addr];
            return (offset & 1) ? static_cast<u8>(word >> 8) : static_cast<u8>(word & 0xFF);
        }
    }

    if (device_.mapped_fuses.size > 0 &&
        address >= device_.mapped_fuses.data_start &&
        address < device_.mapped_fuses.data_start + device_.mapped_fuses.size) {
        const u32 offset = address - device_.mapped_fuses.data_start;
        return (offset < fuses_.size()) ? fuses_[offset] : 0xFFU;
    }

    if (device_.mapped_signatures.size > 0 &&
        address >= device_.mapped_signatures.data_start &&
        address < device_.mapped_signatures.data_start + device_.mapped_signatures.size) {
        const u32 offset = address - device_.mapped_signatures.data_start;
        return (offset < device_.signature.size()) ? device_.signature[offset] : 0xFFU;
    }

    // 2. Standard Peripheral/Memory dispatch
    if (IoPeripheral* peripheral = find_peripheral(address); peripheral != nullptr) {
        value = peripheral->read(address);
    } else if (address < data_.size()) {
        value = data_[address];
        // Dynamic bits for SPMCSR
        if (address == device_.spmcsr_address) {
            if (spm_busy_cycles_left_ > 0) {
                value |= 0x01U; // SPMEN
            }
            if (flash_rww_busy_) {
                value |= 0x40U; // RWWSB
            }
        }
    } else if (xmem_ != nullptr) {
        value = xmem_->read_external(address);
    }

    if (trace_hook_ != nullptr) {
        trace_hook_->on_memory_read(address, value);
    }

    return value;
}

void MemoryBus::write_data(const u16 address, const u8 value) noexcept
{
    if (trace_hook_ != nullptr) {
        trace_hook_->on_memory_write(address, value);
    }

    // 1. Check for Unified Memory Map aliases (Page Buffer writes)
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
            return;
        }
    }

    if (IoPeripheral* peripheral = find_peripheral(address); peripheral != nullptr) {
        peripheral->write(address, value);
        return;
    }

    if (address == device_.spmcsr_address) {
        // Check for RWWSRE (bit 4)
        if ((value & 0x10U) != 0U) {
            flash_rww_busy_ = false;
        }
    }

    if (address < data_.size()) {
        data_[address] = value;
    } else if (xmem_ != nullptr) {
        xmem_->write_external(address, value);
    }
}

void MemoryBus::tick_peripherals(const u64 elapsed_cycles, const u8 active_domains) noexcept
{
    for (IoPeripheral* peripheral : peripherals_) {
        if (peripheral != nullptr) {
            const u8 domain_mask = static_cast<u8>(peripheral->clock_domain());
            if ((domain_mask & active_domains) != 0 || domain_mask == 0) {
                peripheral->tick(elapsed_cycles);
            }
        }
    }

    if (spm_busy_cycles_left_ > 0U) {
        if (elapsed_cycles >= spm_busy_cycles_left_) {
            spm_busy_cycles_left_ = 0U;
            
            // Finalize SPM operation
            const u8 command = spm_command_;
            const u32 address = spm_address_;
            (void)spm_data_; // Reserved for future use (e.g. word write)

            const u32 word_addr = address >> 1;
            const u32 page_mask = ~(static_cast<u32>(device_.flash_page_size) - 1U);
            const u32 page_start = word_addr & page_mask;

            const bool is_nvm = (nvm_ctrl_ != nullptr);
            const bool is_erase = is_nvm ? (command == 2 || command == 3) : (command & 0x02U);
            const bool is_write = is_nvm ? (command == 1 || command == 3) : (command & 0x04U);

            if (is_erase) { // Page Erase
                for (u32 i = 0; i < device_.flash_page_size; ++i) {
                    if (page_start + i < flash_.size()) {
                        flash_[page_start + i] = 0xFFFFU;
                    }
                }
                flash_rww_busy_ = true;
            }
            
            if (is_write) { // Page Write
                 for (u32 i = 0; i < device_.flash_page_size; ++i) {
                    if (page_start + i < flash_.size()) {
                        flash_[page_start + i] = flash_page_buffer_[i];
                    }
                }
                flash_rww_busy_ = true;
            }

            // Clear SPMEN and command bits
            if (device_.spmcsr_address < data_.size()) {
                data_[device_.spmcsr_address] &= ~0x1FU;
            }

            Logger::debug("MemoryBus: SPM/NVM finished at 0x" + std::to_string(address));
            
            if (nvm_ctrl_) {
                nvm_ctrl_->set_busy(false, false);
                nvm_ctrl_->clear_command();
            }
        } else {
            spm_busy_cycles_left_ -= static_cast<u32>(elapsed_cycles);
        }
    }
}

bool MemoryBus::consume_pin_change(PinStateChange& change) noexcept
{
    for (IoPeripheral* peripheral : peripherals_) {
        if (peripheral != nullptr && peripheral->consume_pin_change(change)) {
            return true;
        }
    }
    return false;
}

void MemoryBus::propagate_external_pin_change(const u32 external_id, const PinLevel level) noexcept
{
    for (IoPeripheral* peripheral : peripherals_) {
        if (peripheral != nullptr) {
            peripheral->on_external_pin_change(external_id, level);
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
            if (!found || candidate.vector_index < best_request.vector_index ||
                (candidate.vector_index == best_request.vector_index && candidate.source_id < best_request.source_id)) {
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
        return false;
    }

    request = selected_request;
    return selected_peripheral->consume_interrupt_request(request);
}

IoPeripheral* MemoryBus::find_peripheral(const u16 address) noexcept
{
    return address < dispatch_table_.size() ? dispatch_table_[address] : nullptr;
}

const IoPeripheral* MemoryBus::find_peripheral(const u16 address) const noexcept
{
    return address < dispatch_table_.size() ? dispatch_table_[address] : nullptr;
}

void MemoryBus::write_program_word(const u32 word_address, const u16 value) noexcept
{
    if (word_address < flash_.size()) {
        flash_[word_address] = value;
    }
}

u8 MemoryBus::get_wait_states(const u16 address) const noexcept
{
    return (xmem_ != nullptr) ? xmem_->get_wait_states(address) : 0U;
}

bool MemoryBus::should_stall_cpu(u32 pc_word) const noexcept {
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

void MemoryBus::execute_spm(const u8 command, const u32 address, const u16 data) noexcept {
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

void MemoryBus::execute_nvm_command(u8 command, u32 address, u16 data) noexcept {
    if (spm_busy_cycles_left_ > 0U) {
        return;
    }

    spm_command_ = command;
    spm_address_ = address;
    spm_data_ = data;
    
    // NVMCTRL commands (AVR8X)
    // 1: PAGEWRITE, 2: PAGEERASE, 3: PAGEERASEWRITE, 5: CHIPERASE, etc.
    spm_busy_cycles_left_ = 64000U; // ~4ms at 16MHz

    if (nvm_ctrl_) {
        bool flash_busy = (command >= 1 && command <= 5);
        bool ee_busy = (command == 6);
        nvm_ctrl_->set_busy(flash_busy, ee_busy);
    }

    Logger::debug("MemoryBus: NVM command " + std::to_string(command) + " started at 0x" + std::to_string(address));
}

}  // namespace vioavr::core
