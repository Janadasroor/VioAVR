#include "vioavr/core/xmegaac.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include <algorithm>

namespace vioavr::core {

XmegaAc::XmegaAc(const XmegaAcDescriptor& desc) noexcept : desc_(desc) {
    u16 start = desc_.ac0ctrl_address;
    u16 end = desc_.status_address;
    if (start != 0 && end >= start) {
        ranges_[0] = {start, end};
    }
}

void XmegaAc::reset() noexcept {
    comps_[0] = {};
    comps_[1] = {};
    ctrla_ = 0;
    winctrl_ = 0;
    status_ = 0;
    prev_comp_out_[0] = false;
    prev_comp_out_[1] = false;
    prev_wstate_ = 0;
    pending_int_ = false;
}

double XmegaAc::comp_input_positive(const CompRegs& regs) const noexcept {
    if (!signal_bank_) return 0.0;
    u8 muxpos = (regs.mux >> 3) & 0x07U;
    return signal_bank_->voltage(muxpos);
}

double XmegaAc::comp_input_negative(const CompRegs& regs) const noexcept {
    if (!signal_bank_) return 0.0;
    u8 muxneg = regs.mux & 0x07U;
    return signal_bank_->voltage(8 + muxneg);
}

void XmegaAc::evaluate(u8 comp_index) noexcept {
    CompRegs& regs = comps_[comp_index];
    if (!(regs.ctrl & 0x01U)) return;
    if (!(ctrla_ & 0x01U)) return;

    bool old_out = prev_comp_out_[comp_index];
    double vp = comp_input_positive(regs);
    double vn = comp_input_negative(regs);
    u8 hys = (regs.ctrl >> 5) & 0x03U;
    double hyst_v = 0.0;
    if (hys == 1) hyst_v = 0.010;
    else if (hys == 2) hyst_v = 0.050;

    bool new_out;
    if (hyst_v > 0.0) {
        if (old_out)
            new_out = (vp > vn - hyst_v);
        else
            new_out = (vp > vn + hyst_v);
    } else {
        new_out = (vp > vn);
    }

    if (new_out)
        status_ |= (0x01U << comp_index);
    else
        status_ &= ~(0x01U << comp_index);

    prev_comp_out_[comp_index] = new_out;

    if (new_out == old_out) return;

    u8 intmode = (regs.ctrl >> 1) & 0x03U;
    bool trigger = false;
    switch (intmode) {
        case 0: trigger = true; break;
        case 1: trigger = new_out; break;
        case 2: trigger = !new_out; break;
        case 3: trigger = true; break;
    }

    u8 intsel = (regs.ctrl >> 3) & 0x03U;
    if (trigger && intsel != 0) {
        pending_int_ = true;
    }
}

void XmegaAc::update_window() noexcept {
    if (!(winctrl_ & 0x01U)) return;

    bool ac0_out = prev_comp_out_[0];
    bool ac1_out = prev_comp_out_[1];
    u8 wstate = (ac1_out ? 2U : 0U) | (ac0_out ? 1U : 0U);

    if (wstate >= 2)
        status_ |= 0x10U;
    else
        status_ &= ~0x10U;

    if (wstate == 0)
        status_ |= 0x20U;
    else
        status_ &= ~0x20U;

    if (wstate == prev_wstate_) return;
    prev_wstate_ = wstate;

    u8 wintmode = (winctrl_ >> 4) & 0x03U;
    if (wintmode == 0) return;

    bool trigger = false;
    switch (wintmode) {
        case 1: trigger = (wstate >= 2); break;
        case 2: trigger = (wstate == 0); break;
        case 3: trigger = true; break;
    }

    if (trigger) {
        pending_int_ = true;
    }
}

void XmegaAc::tick(u64) noexcept {
    if (!(ctrla_ & 0x01U)) return;

    evaluate(0);
    evaluate(1);
    update_window();
    update_interrupt_state();
}

u8 XmegaAc::read(u16 address) noexcept {
    if (address == desc_.ac0ctrl_address) return comps_[0].ctrl;
    if (address == desc_.ac1ctrl_address) return comps_[1].ctrl;
    if (address == desc_.ac0mux_address) return comps_[0].mux;
    if (address == desc_.ac1mux_address) return comps_[1].mux;
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.ctrlb_address) return 0;
    if (address == desc_.winctrl_address) return winctrl_;
    if (address == desc_.status_address) return status_;
    return 0;
}

void XmegaAc::write(u16 address, u8 value) noexcept {
    if (address == desc_.ac0ctrl_address) {
        comps_[0].ctrl = value;
    } else if (address == desc_.ac1ctrl_address) {
        comps_[1].ctrl = value;
    } else if (address == desc_.ac0mux_address) {
        comps_[0].mux = value;
    } else if (address == desc_.ac1mux_address) {
        comps_[1].mux = value;
    } else if (address == desc_.ctrla_address) {
        ctrla_ = value & 0x03U;
    } else if (address == desc_.winctrl_address) {
        winctrl_ = value & 0x33U;
    }
}

bool XmegaAc::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (pending_int_) {
        request.vector_index = desc_.vector_index;
        return true;
    }
    return false;
}

bool XmegaAc::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (pending_int_) {
        request.vector_index = desc_.vector_index;
        pending_int_ = false;
        update_interrupt_state();
        return true;
    }
    return false;
}

void XmegaAc::update_interrupt_state() noexcept {
    set_interrupt_pending(pending_int_);
}

} // namespace vioavr::core
