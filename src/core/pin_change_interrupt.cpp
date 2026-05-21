#include "vioavr/core/pin_change_interrupt.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/memory_bus.hpp"
#include <algorithm>
#include <vector>

namespace vioavr::core {

PinChangeInterrupt::PinChangeInterrupt(const std::string_view name,
                                       const PinChangeInterruptDescriptor& desc,
                                       GpioPort& port) noexcept
    : name_(name),
      desc_(desc),
      port_(port),
      shared_state_ptr_(&shared_state_),
      map_shared_registers_(true)
{
    std::vector<u16> addrs = {desc.pcmsk_address, desc.pcicr_address, desc.pcifr_address};
    std::sort(addrs.begin(), addrs.end());

    size_t ri = 0;
    ranges_.fill({0, 0});
    for (size_t i = 0; i < addrs.size(); ++i) {
        if (addrs[i] == 0) continue;
        if (ri > 0 && addrs[i] == ranges_[ri-1].end + 1) {
             ranges_[ri-1].end = addrs[i];
        } else {
             ranges_[ri++] = AddressRange{addrs[i], addrs[i]};
        }
    }
}

PinChangeInterrupt::PinChangeInterrupt(const std::string_view name,
                                       const PinChangeInterruptDescriptor& desc,
                                       GpioPort& port,
                                       PinChangeInterruptSharedState& shared_state,
                                       const bool map_shared_registers) noexcept
    : name_(name),
      desc_(desc),
      port_(port),
      shared_state_ptr_(&shared_state),
      map_shared_registers_(map_shared_registers)
{
    std::vector<u16> addrs = {desc.pcmsk_address};
    if (map_shared_registers_) {
        addrs.push_back(desc.pcicr_address);
        addrs.push_back(desc.pcifr_address);
    }
    std::sort(addrs.begin(), addrs.end());
    
    size_t ri = 0;
    ranges_.fill({0, 0});
    for (size_t i = 0; i < addrs.size(); ++i) {
        if (addrs[i] == 0) continue;
        if (ri > 0 && addrs[i] == ranges_[ri-1].end + 1) {
             ranges_[ri-1].end = addrs[i];
        } else {
             ranges_[ri++] = AddressRange{addrs[i], addrs[i]};
        }
    }
}

std::string_view PinChangeInterrupt::name() const noexcept
{
    return name_;
}

ClockDomain PinChangeInterrupt::clock_domain() const noexcept
{
    return ClockDomain::none;
}

std::span<const AddressRange> PinChangeInterrupt::mapped_ranges() const noexcept
{
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

void PinChangeInterrupt::reset() noexcept
{
    shared_state_ptr_->pcicr = 0U;
    shared_state_ptr_->pcifr = 0U;
    pcmsk_ = 0U;
    pcint_pending_ = false;
    last_pin_state_ = port_.read(port_.pin_address());
    was_active_ = true;
    update_active_state();
}

void PinChangeInterrupt::tick(const u64 elapsed_cycles) noexcept
{
    (void)elapsed_cycles;
    const u8 current_pin_state = port_.read(port_.pin_address());
    if (current_pin_state != last_pin_state_) {
        const u8 changed_bits = static_cast<u8>(current_pin_state ^ last_pin_state_);
        notify_pin_change(changed_bits);
        last_pin_state_ = current_pin_state;
    }
}

u8 PinChangeInterrupt::read(const u16 address) noexcept
{
    if (address == desc_.pcicr_address) return shared_state_ptr_->pcicr;
    if (address == desc_.pcifr_address) return shared_state_ptr_->pcifr;
    if (address == desc_.pcmsk_address) return pcmsk_;
    return 0U;
}

void PinChangeInterrupt::write(const u16 address, const u8 value) noexcept
{
    if (address == desc_.pcicr_address) {
        shared_state_ptr_->pcicr = value;
    } else if (address == desc_.pcifr_address) {
        shared_state_ptr_->pcifr &= static_cast<u8>(~value);
        if ((shared_state_ptr_->pcifr & desc_.pcifr_flag_mask) == 0U) {
            pcint_pending_ = false;
        }
    } else if (address == desc_.pcmsk_address) {
        pcmsk_ = value;
    }
    update_interrupt_pending();
    update_active_state();
}

bool PinChangeInterrupt::pending_interrupt_request(InterruptRequest& request) const noexcept
{
    bool enabled = (shared_state_ptr_->pcicr & desc_.pcicr_enable_mask) != 0U;
    if (pcint_pending_ && enabled) {
        request = {.vector_index = desc_.vector_index, .source_id = 0U};
        Logger::debug("PinChangeInterrupt '" + name_ + "' IS REQUESTING interrupt vector " + std::to_string(request.vector_index));
        return true;
    }
    return false;
}

bool PinChangeInterrupt::consume_interrupt_request(InterruptRequest& request) noexcept
{
    if (!pending_interrupt_request(request)) {
        return false;
    }

    pcint_pending_ = false;
    shared_state_ptr_->pcifr &= static_cast<u8>(~desc_.pcifr_flag_mask);
    update_interrupt_pending();
    return true;
}

void PinChangeInterrupt::notify_pin_change(const u8 mask) noexcept
{
    if ((mask & pcmsk_) != 0U) {
        Logger::debug("PinChangeInterrupt '" + name_ + "' triggered by mask 0x" + Logger::hex(mask));
        shared_state_ptr_->pcifr |= desc_.pcifr_flag_mask;
        pcint_pending_ = true;
        update_interrupt_pending();
    }
}

void PinChangeInterrupt::update_interrupt_pending() noexcept {
    InterruptRequest req;
    set_interrupt_pending(pending_interrupt_request(req));
}

void PinChangeInterrupt::update_active_state() noexcept
{
    if (bus_) {
        bool active = is_enabled();
        if (active && !was_active_) {
            last_pin_state_ = port_.read(port_.pin_address());
        }
        was_active_ = active;
        bus_->set_peripheral_active(this, active);
    }
}

}  // namespace vioavr::core
