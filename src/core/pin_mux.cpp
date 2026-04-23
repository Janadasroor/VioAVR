#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/logger.hpp"
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

void PinMux::update_pin(u8 port_idx, u8 bit_idx, PinOwner requester, bool is_output, bool level, bool pullup) noexcept
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

    reevaluate_ownership(port_idx, bit_idx);
}

void PinMux::update_pin_by_address(u16 pin_address, u8 bit_index, PinOwner requester, bool is_output, bool level, bool pullup) noexcept
{
    auto it = addr_to_port_.find(pin_address);
    if (it != addr_to_port_.end()) {
        update_pin(it->second, bit_index, requester, is_output, level, pullup);
    }
}

void PinMux::update_pullup_suppressed(bool suppressed) noexcept
{
    if (pullup_suppressed_ == suppressed) return;
    pullup_suppressed_ = suppressed;

    // re-evaluate all pins
    for (u8 p = 0; p < static_cast<u8>(ports_.size()); ++p) {
        for (u8 b = 0; b < 8; ++b) {
            // This is tricky because PinMux doesn't store the "requested" pullup state,
            // it only stores the "effective" one in PinState.
            // In a better design, PinEntry should store requested state per owner.
            // For now, let's just trigger a callback if something might have changed?
            // Actually, we'll need to store requested_pullup in PinEntry for this to work perfectly.
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
            entry.drive_levels = 0xFFFFFFFFU;
            entry.output_mask = 0;
            entry.pullup_mask = 0;
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

    // Composite drive level (AND of all active drivers)
    bool composite_drive = true;
    u32 mask = entry.active_claims;
    if ((entry.drive_levels & mask) != mask) {
        composite_drive = false;
    }
    entry.state.drive_level = composite_drive;
    Logger::debug("PinMux: Port" + std::to_string(port_idx) + " Pin" + std::to_string(bit_idx) + 
                  " Composite Drive: " + std::to_string(composite_drive) + 
                  " Claims: 0x" + Logger::hex(static_cast<u32>(entry.active_claims)));

    // Composite output state (OR of all active output requests)
    entry.state.is_output = (entry.output_mask & entry.active_claims) != 0;

    // Composite pullup state
    entry.state.pullup_enabled = ((entry.pullup_mask & entry.active_claims) != 0) && !pullup_suppressed_;

    if (callback_) callback_(port_idx, bit_idx, entry.state);
}

} // namespace vioavr::core
