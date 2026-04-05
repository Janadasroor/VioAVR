#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/logger.hpp"

#include <algorithm>
#include <stdexcept>

namespace vioavr::core {

MemoryBus::MemoryBus(const DeviceDescriptor& device) : device_(device)
{
    flash_.resize(device_.flash_words, 0U);
    data_.resize(static_cast<std::size_t>(device_.data_end_address()) + 1U, 0U);
    dispatch_table_.resize(data_.size(), nullptr);
    reset();
}

void MemoryBus::reset() noexcept
{
    std::ranges::fill(data_, 0U);
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
    Logger::info("Attaching peripheral to memory bus: " + std::string(peripheral.name()));
    peripherals_.push_back(&peripheral);
    Logger::debug("MemoryBus: peripherals size is now " + std::to_string(peripherals_.size()));

    for (const auto& range : peripheral.mapped_ranges()) {
        Logger::debug("Mapping peripheral '" + std::string(peripheral.name()) + "' to range [0x" + 
                      std::to_string(range.begin) + ", 0x" + std::to_string(range.end) + "]");
        for (u32 addr = range.begin; addr <= range.end && addr < dispatch_table_.size(); ++addr) {
            dispatch_table_[addr] = &peripheral;
        }
    }
}

void MemoryBus::load_flash(const std::span<const u16> words)
{
    if (words.size() > flash_.size()) {
        throw std::out_of_range("flash image exceeds configured device capacity");
    }

    const auto copy_size = std::min(words.size(), flash_.size());
    std::ranges::copy(words.first(copy_size), flash_.begin());
    if (copy_size < flash_.size()) {
        std::ranges::fill(flash_.begin() + static_cast<std::ptrdiff_t>(copy_size), flash_.end(), 0U);
    }

    loaded_program_words_ = static_cast<u32>(copy_size);
    Logger::debug("MemoryBus: loaded " + std::to_string(loaded_program_words_) + " words");
    reset_word_address_ = 0U;
}

void MemoryBus::load_image(const HexImage& image)
{
    if (image.entry_word >= flash_.size()) {
        throw std::out_of_range("reset entry exceeds configured flash capacity");
    }

    load_flash(image.flash_words);
    reset_word_address_ = image.entry_word;
}

u8 MemoryBus::read_data(const u16 address) noexcept
{
    u8 value = 0U;
    if (IoPeripheral* peripheral = find_peripheral(address); peripheral != nullptr) {
        value = peripheral->read(address);
    } else if (address < data_.size()) {
        value = data_[address];
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

    if (IoPeripheral* peripheral = find_peripheral(address); peripheral != nullptr) {
        peripheral->write(address, value);
        return;
    }

    if (address < data_.size()) {
        data_[address] = value;
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

void MemoryBus::propagate_external_pin_change(u32 external_id, PinLevel level) noexcept
{
    if (!pin_map_) return;

    auto mapping = pin_map_->get_mapping_by_external(external_id);
    if (!mapping) return;

    for (IoPeripheral* peripheral : peripherals_) {
        if (peripheral != nullptr) {
            // Peripherals should check if they are interested in this port/bit
            // or we could filter here. For now, broadcast to allow multiple 
            // observers (e.g. GPIO and EXINT).
            if (peripheral->name() == mapping->port_name) {
                peripheral->on_external_pin_change(mapping->bit_index, level);
            } else if (peripheral->name() == "EXINT") {
                // EXINT is a special case that often needs to observe specific pins
                // For a generic implementation, we'd need a way for EXINT to 
                // declare interest in "PORTD:2".
                // As a shortcut for the POC:
                peripheral->on_external_pin_change(mapping->bit_index, level);
            }
        }
    }
}

bool MemoryBus::pending_interrupt_request(InterruptRequest& request) const noexcept
{
    bool found = false;
    InterruptRequest best_request {};

    for (const IoPeripheral* peripheral : peripherals_) {
        InterruptRequest candidate {};
        if (peripheral != nullptr && peripheral->pending_interrupt_request(candidate)) {
            Logger::debug("MemoryBus: " + std::string(peripheral->name()) + " has pending interrupt vector=" + std::to_string(candidate.vector_index));
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

bool MemoryBus::consume_interrupt_request(InterruptRequest& request) noexcept
{
    IoPeripheral* selected_peripheral = nullptr;
    InterruptRequest selected_request {};

    for (IoPeripheral* peripheral : peripherals_) {
        InterruptRequest candidate {};
        if (peripheral != nullptr && peripheral->pending_interrupt_request(candidate)) {
            if (selected_peripheral == nullptr || candidate.vector_index < selected_request.vector_index ||
                (candidate.vector_index == selected_request.vector_index &&
                 candidate.source_id < selected_request.source_id)) {
                selected_peripheral = peripheral;
                selected_request = candidate;
            }
        }
    }

    if (selected_peripheral == nullptr) {
        return false;
    }

    request = selected_request;
    Logger::debug("MemoryBus: consuming request vector=" + std::to_string(request.vector_index));
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

}  // namespace vioavr::core
