#include "vioavr/core/pin_mux.hpp"
#include <stdexcept>
#include <cstdio>

namespace vioavr::core {

PinMux::PinMux(u8 num_ports) noexcept
{
    ports_.resize(num_ports);
    for (auto& port : ports_) {
        port.resize(8); // Standard AVR ports have 8 pins
        for (auto& pin : port) {
            pin.active_claims = (1U << static_cast<u8>(PinOwner::gpio));
        }
    }
}

void PinMux::register_port(u16 pin_address, u8 port_idx) noexcept
{
    addr_to_port_[pin_address] = port_idx;
}

bool PinMux::claim_pin(u8 port_idx, u8 bit_idx, PinOwner owner) noexcept
{
    if (port_idx >= ports_.size() || bit_idx >= 8) return false;

    auto& entry = ports_[port_idx][bit_idx];
    const u32 claim_bit = (1U << static_cast<u8>(owner));

    if (!(entry.active_claims & claim_bit)) {
        entry.active_claims |= claim_bit;
        reevaluate_ownership(port_idx, bit_idx);
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
    if (owner == PinOwner::gpio) return; // Cannot release GPIO

    auto& entry = ports_[port_idx][bit_idx];
    const u32 claim_bit = (1U << static_cast<u8>(owner));

    if (entry.active_claims & claim_bit) {
        entry.active_claims &= ~claim_bit;
        reevaluate_ownership(port_idx, bit_idx);
    }
}

void PinMux::release_pin_by_address(u16 pin_address, u8 bit_index, PinOwner owner) noexcept
{
    auto it = addr_to_port_.find(pin_address);
    if (it != addr_to_port_.end()) {
        release_pin(it->second, bit_index, owner);
    }
}

void PinMux::update_pin(u8 port_idx, u8 bit_idx, PinOwner requester, bool is_output, bool level, bool pullup, bool wired_and) noexcept
{
    if (port_idx >= ports_.size() || bit_idx >= 8) return;

    PinEntry& entry = ports_[port_idx][bit_idx];
    u32 owner_bit = 1U << static_cast<u32>(requester);
    
    if (level) entry.drive_levels |= owner_bit;
    else entry.drive_levels &= ~owner_bit;
    
    if (is_output) entry.output_mask |= owner_bit;
    else entry.output_mask &= ~owner_bit;
    
    if (pullup) entry.pullup_mask |= owner_bit;
    else entry.pullup_mask &= ~owner_bit;

    if (wired_and) entry.wired_and_mask |= owner_bit;
    else entry.wired_and_mask &= ~owner_bit;

    reevaluate_ownership(port_idx, bit_idx);
}

void PinMux::update_pin_by_address(u16 pin_address, u8 bit_index, PinOwner requester, bool is_output, bool level, bool pullup) noexcept
{
    auto it = addr_to_port_.find(pin_address);
    if (it != addr_to_port_.end()) {
        update_pin(it->second, bit_index, requester, is_output, level, pullup);
    }
}

void PinMux::update_analog_pin_by_address(u16 pin_address, u8 bit_index, PinOwner requester, double voltage) noexcept
{
    auto it = addr_to_port_.find(pin_address);
    if (it == addr_to_port_.end()) return;
    u8 port_idx = it->second;
    if (port_idx >= ports_.size() || bit_index >= 8) return;

    PinEntry& entry = ports_[port_idx][bit_index];
    if (entry.state.owner == requester) {
        entry.state.voltage = voltage;
        if (callback_) callback_(port_idx, bit_index, entry.state);
    }
}

void PinMux::update_pullup_suppressed(bool suppressed) noexcept
{
    if (pullup_suppressed_ == suppressed) return;
    pullup_suppressed_ = suppressed;

    // re-evaluate all pins
    for (u8 p = 0; p < static_cast<u8>(ports_.size()); ++p) {
        for (u8 b = 0; b < 8; ++b) {
            reevaluate_ownership(p, b);
        }
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

void PinMux::reset() noexcept {
    for (u8 p = 0; p < ports_.size(); ++p) {
        for (u8 b = 0; b < ports_[p].size(); ++b) {
            PinEntry& entry = ports_[p][b];
            entry.drive_levels = 0;
            entry.output_mask = 0;
            entry.pullup_mask = 0;
            entry.wired_and_mask = 0;
            reevaluate_ownership(p, b);
        }
    }
}

void PinMux::reevaluate_ownership(u8 port_idx, u8 bit_idx) noexcept
{
    auto& entry = ports_[port_idx][bit_idx];
    PinOwner highest_owner = PinOwner::gpio;
    u8 highest_prio = 0;

    // Find the claimant with the highest priority
    for (u8 i = 0; i < 32; ++i) {
        if (entry.active_claims & (1U << i)) {
            PinOwner owner = static_cast<PinOwner>(i);
            u8 prio = get_pin_priority(owner);
            if (prio >= highest_prio) {
                highest_prio = prio;
                highest_owner = owner;
            }
        }
    }

    entry.state.owner = highest_owner;
    
    // Check if any owner wants Wired-AND
    bool is_wired_and = (entry.wired_and_mask & entry.active_claims) != 0;
    entry.state.is_wired_and = is_wired_and;

    if (is_wired_and) {
        // In Wired-AND mode, the level is LOW if ANY output drives LOW.
        // I.e., it is HIGH only if ALL active outputs drive HIGH.
        u32 active_outputs = entry.active_claims & entry.output_mask;
        if (active_outputs == 0) {
            entry.state.drive_level = true; // Weak HIGH (floating or pullup)
        } else {
            entry.state.drive_level = (entry.drive_levels & active_outputs) == active_outputs;
        }
    } else {
        // Drive level follows the highest priority owner
        u32 owner_bit = (1U << static_cast<u32>(highest_owner));
        entry.state.drive_level = (entry.drive_levels & owner_bit) != 0;
    }

    // Composite output state
    u32 highest_owner_bit = (1U << static_cast<u32>(highest_owner));
    entry.state.is_output = (entry.output_mask & highest_owner_bit) != 0;

    // Composite pullup state
    entry.state.pullup_enabled = ((entry.pullup_mask & highest_owner_bit) != 0) && !pullup_suppressed_;
 
    if (callback_) callback_(port_idx, bit_idx, entry.state);
}

} // namespace vioavr::core
