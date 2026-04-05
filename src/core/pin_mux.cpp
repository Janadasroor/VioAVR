#include "vioavr/core/pin_mux.hpp"
#include <stdexcept>

namespace vioavr::core {

PinMux::PinMux(u8 num_ports) noexcept
{
    ports_.resize(num_ports);
    for (auto& port : ports_) {
        port.resize(8); // Standard AVR ports have 8 pins
    }
}

void PinMux::register_port(u16 pin_address, u8 port_idx) noexcept
{
    addr_to_port_[pin_address] = port_idx;
}

bool PinMux::claim_pin(u8 port_idx, u8 bit_idx, PinOwner owner) noexcept
{
    if (port_idx >= ports_.size() || bit_idx >= 8) return false;

    auto& pin = ports_[port_idx][bit_idx].state;
    
    // If already owned by someone else (not the default GPIO), we can't claim it
    if (pin.owner != PinOwner::gpio && pin.owner != owner) {
        return false;
    }

    if (pin.owner != owner) {
        pin.owner = owner;
        if (callback_) callback_(port_idx, bit_idx, pin);
    }
    
    return true;
}

bool PinMux::claim_pin_by_address(u16 pin_address, u8 bit_index, PinOwner owner) noexcept
{
    auto it = addr_to_port_.find(pin_address);
    if (it == addr_to_port_.end()) return false;
    return claim_pin(it->second, bit_index, owner);
}

void PinMux::release_pin(u8 port_idx, u8 bit_idx, PinOwner owner) noexcept
{
    if (port_idx >= ports_.size() || bit_idx >= 8) return;

    auto& pin = ports_[port_idx][bit_idx].state;
    if (pin.owner == owner) {
        pin.owner = PinOwner::gpio;
        if (callback_) callback_(port_idx, bit_idx, pin);
    }
}

void PinMux::release_pin_by_address(u16 pin_address, u8 bit_index, PinOwner owner) noexcept
{
    auto it = addr_to_port_.find(pin_address);
    if (it != addr_to_port_.end()) {
        release_pin(it->second, bit_index, owner);
    }
}

void PinMux::update_pin(u8 port_idx, u8 bit_idx, PinOwner requester, bool is_output, bool level) noexcept
{
    if (port_idx >= ports_.size() || bit_idx >= 8) return;

    auto& pin = ports_[port_idx][bit_idx].state;
    
    // Only the current owner can update the pin state
    if (pin.owner != requester) return;

    if (pin.is_output != is_output || pin.drive_level != level) {
        pin.is_output = is_output;
        pin.drive_level = level;
        if (callback_) callback_(port_idx, bit_idx, pin);
    }
}

void PinMux::update_pin_by_address(u16 pin_address, u8 bit_index, PinOwner requester, bool is_output, bool level) noexcept
{
    auto it = addr_to_port_.find(pin_address);
    if (it != addr_to_port_.end()) {
        update_pin(it->second, bit_index, requester, is_output, level);
    }
}

PinState PinMux::get_state(u8 port_idx, u8 bit_idx) const noexcept
{
    if (port_idx >= ports_.size() || bit_idx >= 8) return {};
    return ports_[port_idx][bit_idx].state;
}

PinState PinMux::get_state_by_address(u16 pin_address, u8 bit_index) const noexcept
{
    auto it = addr_to_port_.find(pin_address);
    if (it == addr_to_port_.end()) return {};
    return get_state(it->second, bit_index);
}

void PinMux::set_callback(PinChangeCallback callback) noexcept
{
    callback_ = callback;
}

} // namespace vioavr::core
