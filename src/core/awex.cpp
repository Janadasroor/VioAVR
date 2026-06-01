#include "vioavr/core/awex.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/port_mux.hpp"
#include "vioavr/core/pin_mux.hpp"

namespace vioavr::core {

Awex::Awex(std::string name, const AwexDescriptor& desc) noexcept
    : name_(std::move(name)), desc_(desc) {
    u8 ri = 0;
    auto add_range = [&](u16 addr) {
        if (addr == 0) return;
        for (u8 j = 0; j < ri; ++j) {
            if (addr >= ranges_[j].begin && addr <= ranges_[j].end) return;
        }
        bool merged = false;
        for (u8 j = 0; j < ri; ++j) {
            if (addr == ranges_[j].end + 1) { ranges_[j].end = addr; merged = true; break; }
            if (addr == ranges_[j].begin - 1) { ranges_[j].begin = addr; merged = true; break; }
        }
        if (!merged && ri < ranges_.size()) { ranges_[ri++] = {addr, addr}; }
    };
    add_range(desc_.ctrl_address);
    add_range(desc_.fdemask_address);
    add_range(desc_.fdctrl_address);
    add_range(desc_.status_address);
    add_range(desc_.dtboth_address);
    add_range(desc_.dtbothbuf_address);
    add_range(desc_.dtls_address);
    add_range(desc_.dths_address);
    add_range(desc_.dtlsbuf_address);
    add_range(desc_.dthsbuf_address);
    add_range(desc_.outoven_address);
    for (; ri < ranges_.size(); ++ri) ranges_[ri] = {0, 0};
}

std::span<const AddressRange> Awex::mapped_ranges() const noexcept {
    if (desc_.ctrl_address == 0) return {};
    size_t count = 0;
    for (auto& r : ranges_) { if (r.begin == 0 && r.end == 0) break; ++count; }
    return {ranges_.data(), count};
}

void Awex::reset() noexcept {
    ctrl_ = 0;
    fdemask_ = 0;
    fdctrl_ = 0;
    status_ = 0;
    dtboth_ = 0;
    dtbothbuf_ = 0;
    dtls_ = 0;
    dths_ = 0;
    dtlsbuf_ = 0;
    dthsbuf_ = 0;
    outoven_ = 0;
    wo_levels_ = 0;
    for (auto& p : dt_pending_) p = false;
    for (auto& c : dt_counters_) c = 0;
    drive_outputs();
}

void Awex::tick(u64 elapsed_cycles) noexcept {
    if (elapsed_cycles == 0) return;
    bool changed = false;
    for (u8 ch = 0; ch < 4; ++ch) {
        if (dt_pending_[ch]) {
            if (dt_counters_[ch] <= elapsed_cycles) {
                dt_counters_[ch] = 0;
                dt_pending_[ch] = false;
                changed = true;
            } else {
                dt_counters_[ch] -= static_cast<u16>(elapsed_cycles);
            }
        }
    }
    if (changed) drive_outputs();
}

void Awex::set_wo_level(u8 channel, bool level) noexcept {
    if (channel >= 4) return;
    bool old = (wo_levels_ >> channel) & 0x01;
    if (old == level) {
        drive_outputs();
        return;
    }

    if (level) {
        wo_levels_ |= (1 << channel);
    } else {
        wo_levels_ &= ~(1 << channel);
    }

    if (dt_enabled_for_channel(channel) && dtboth_ > 0) {
        dt_pending_[channel] = true;
        dt_counters_[channel] = dtboth_;
    }

    drive_outputs();
}

bool Awex::dt_enabled_for_channel(u8 ch) const noexcept {
    return (ctrl_ & (1 << ch)) != 0;
}

bool Awex::get_dh_level(u8 ch) const noexcept {
    if (outoven_ & (1 << ch)) {
        return (outoven_ >> (ch + 4)) & 0x01;
    }
    if (dt_pending_[ch]) {
        return (wo_levels_ >> ch) & 0x01;
    }
    return (wo_levels_ >> ch) & 0x01;
}

bool Awex::get_dl_level(u8 ch) const noexcept {
    if (dt_pending_[ch]) {
        // During dead-time: DL = !new_WO (new WO already in wo_levels_)
        return !((wo_levels_ >> ch) & 0x01);
    }
    return !get_dh_level(ch);
}

u8 Awex::read(u16 address) noexcept {
    if (address == desc_.ctrl_address) return ctrl_;
    if (address == desc_.fdemask_address) return fdemask_;
    if (address == desc_.fdctrl_address) return fdctrl_;
    if (address == desc_.status_address) return status_;
    if (address == desc_.dtboth_address) return dtboth_;
    if (address == desc_.dtbothbuf_address) return dtbothbuf_;
    if (address == desc_.dtls_address) return dtls_;
    if (address == desc_.dths_address) return dths_;
    if (address == desc_.dtlsbuf_address) return dtlsbuf_;
    if (address == desc_.dthsbuf_address) return dthsbuf_;
    if (address == desc_.outoven_address) return outoven_;
    return 0;
}

void Awex::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrl_address) {
        ctrl_ = value & 0x3F;
        drive_outputs();
    }
    else if (address == desc_.fdemask_address) fdemask_ = value;
    else if (address == desc_.fdctrl_address) fdctrl_ = value & 0x17;
    else if (address == desc_.status_address) status_ &= ~value;
    else if (address == desc_.dtboth_address) {
        dtboth_ = value;
        drive_outputs();
    }
    else if (address == desc_.dtbothbuf_address) dtbothbuf_ = value;
    else if (address == desc_.dtls_address) dtls_ = value;
    else if (address == desc_.dths_address) dths_ = value;
    else if (address == desc_.dtlsbuf_address) dtlsbuf_ = value;
    else if (address == desc_.dthsbuf_address) dthsbuf_ = value;
    else if (address == desc_.outoven_address) {
        outoven_ = value & 0xFF;
        drive_outputs();
    }
}

static void drive_dh_dl_pins(PinMux* pm, u8 port_idx, u8 channel, bool dh, bool dl, bool enabled) noexcept {
    if (!pm || channel >= 4) return;
    if (port_idx >= 6) return;
    static constexpr u8 DH_BITS[4] = {4, 6, 2, 0};
    static constexpr u8 DL_BITS[4] = {5, 7, 3, 1};
    u8 dh_bit = DH_BITS[channel];
    u8 dl_bit = DL_BITS[channel];
    if (enabled) {
        pm->claim_pin(port_idx, dh_bit, PinOwner::timer);
        pm->update_pin(port_idx, dh_bit, PinOwner::timer, true, dh, false);
        pm->claim_pin(port_idx, dl_bit, PinOwner::timer);
        pm->update_pin(port_idx, dl_bit, PinOwner::timer, true, dl, false);
    } else {
        pm->release_pin(port_idx, dh_bit, PinOwner::timer);
        pm->release_pin(port_idx, dl_bit, PinOwner::timer);
    }
}

void Awex::drive_outputs(u8 port_override) noexcept {
    if (!port_mux_ && !pin_mux_) return;
    u8 port_idx = (port_override != 0xFF) ? port_override : port_index_;
    bool awex_enabled = (ctrl_ & 0x01) != 0;
    for (u8 ch = 0; ch < 4; ++ch) {
        bool enabled = awex_enabled;
        if (port_mux_ && port_mux_->has_tc_routing()) {
            port_mux_->drive_awex_dh_dl(ch, enabled ? get_dh_level(ch) : false,
                                        enabled ? get_dl_level(ch) : false, enabled, port_idx);
        } else if (pin_mux_) {
            drive_dh_dl_pins(pin_mux_, port_idx, ch, get_dh_level(ch), get_dl_level(ch), enabled);
        }
    }
}

} // namespace vioavr::core
