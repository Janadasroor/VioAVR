#include "vioavr/core/tc.hpp"
#include "vioavr/core/awex.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/evsys.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/port_mux.hpp"
namespace vioavr::core {

Tc::Tc(std::string name, const TcDescriptor& desc) noexcept
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
    add_range(desc_.ctrla_address);
    add_range(desc_.ctrlb_address);
    add_range(desc_.ctrlc_address);
    add_range(desc_.ctrld_address);
    add_range(desc_.ctrle_address);
    add_range(desc_.intctrla_address);
    add_range(desc_.intctrlb_address);
    add_range(desc_.ctrlfclr_address);
    add_range(desc_.ctrlfset_address);
    add_range(desc_.ctrlgclr_address);
    add_range(desc_.ctrlgset_address);
    add_range(desc_.intflags_address);
    add_range(desc_.temp_address);
    add_range(desc_.cnt_address);
    add_range(desc_.cnt_address + 1);
    add_range(desc_.period_address);
    add_range(desc_.period_address + 1);
    add_range(desc_.cca_address);
    add_range(desc_.cca_address + 1);
    add_range(desc_.ccb_address);
    add_range(desc_.ccb_address + 1);
    if (desc_.ccc_address) {
        add_range(desc_.ccc_address);
        add_range(desc_.ccc_address + 1);
    }
    if (desc_.ccd_address) {
        add_range(desc_.ccd_address);
        add_range(desc_.ccd_address + 1);
    }
    add_range(desc_.perbuf_address);
    add_range(desc_.perbuf_address + 1);
    add_range(desc_.ccabuf_address);
    add_range(desc_.ccabuf_address + 1);
    add_range(desc_.ccbbuf_address);
    add_range(desc_.ccbbuf_address + 1);
    if (desc_.cccbuf_address) {
        add_range(desc_.cccbuf_address);
        add_range(desc_.cccbuf_address + 1);
    }
    if (desc_.ccdbuf_address) {
        add_range(desc_.ccdbuf_address);
        add_range(desc_.ccdbuf_address + 1);
    }
    for (; ri < ranges_.size(); ++ri) ranges_[ri] = {0, 0};
}

std::span<const AddressRange> Tc::mapped_ranges() const noexcept {
    if (desc_.ctrla_address == 0) return {};
    size_t count = 0;
    for (auto& r : ranges_) { if (r.begin == 0 && r.end == 0) break; ++count; }
    return {ranges_.data(), count};
}

void Tc::set_event_system(EventSystem* evsys) noexcept {
    evsys_ = evsys;
}

void Tc::reset() noexcept {
    ctrla_ = 0;
    ctrlb_ = 0;
    ctrlc_ = 0;
    ctrld_ = 0;
    ctrle_ = 0;
    intctrla_ = 0;
    intctrlb_ = 0;
    ctrlf_ = 0;
    ctrlg_ = 0;
    intflags_ = 0;
    temp_ = 0;
    cnt_ = 0;
    per_ = 0xFFFF;
    cca_ = 0;
    ccb_ = 0;
    ccc_ = 0;
    ccd_ = 0;
    per_buf_ = 0;
    cca_buf_ = 0;
    ccb_buf_ = 0;
    ccc_buf_ = 0;
    ccd_buf_ = 0;
    per_buf_valid_ = false;
    cca_buf_valid_ = false;
    ccb_buf_valid_ = false;
    ccc_buf_valid_ = false;
    ccd_buf_valid_ = false;
    prescaler_counter_ = 0;
    prescaler_limit_ = 1;
    counting_up_ = true;
    last_event_level_ = false;
    just_hit_top_ = false;
    just_hit_bottom_ = false;
    wo_states_[0] = false;
    wo_states_[1] = false;
    wo_states_[2] = false;
    wo_states_[3] = false;
    update_interrupt_state();
    update_active_state();
}

bool Tc::is_enabled() const noexcept {
    return (ctrla_ & 0x01) != 0;
}

u8 Tc::get_clk_sel() const noexcept {
    return (ctrla_ >> 1) & 0x07;
}

u8 Tc::get_wg_mode() const noexcept {
    return wg_mode_in_ctrld_ ? (ctrld_ & 0x07) : (ctrlb_ & 0x07);
}

bool Tc::is_single_slope_pwm() const noexcept {
    u8 wg = get_wg_mode();
    return wg_mode_in_ctrld_ ? (wg == 3) : (wg == 1);
}

void Tc::tick(u64 elapsed_cycles) noexcept {
    if (elapsed_cycles == 0 || !is_enabled() || prescaler_limit_ == 0) return;

    bool any_tick = false;
    for (u64 i = 0; i < elapsed_cycles; ++i) {
        ++prescaler_counter_;
        if (prescaler_counter_ >= prescaler_limit_) {
            prescaler_counter_ = 0;
            perform_tick();
            any_tick = true;
        }
    }
    if (any_tick) update_interrupt_state();
}

void Tc::perform_tick() noexcept {
    u8 wg_mode = get_wg_mode();
    just_hit_top_ = false;
    just_hit_bottom_ = false;

    u16 prev_cnt = cnt_;

    if (wg_mode >= 4 && wg_mode <= 7) {
        if (counting_up_) {
            if (cnt_ < per_) ++cnt_;
            if (cnt_ >= per_) { cnt_ = per_; just_hit_top_ = true; counting_up_ = false; }
        } else {
            if (cnt_ > 0) --cnt_;
            if (cnt_ == 0) { just_hit_bottom_ = true; counting_up_ = true; intflags_ |= 0x01; }
        }
        // Dual-slope: check matches after updating count but before updating outputs
        handle_matches(prev_cnt);
    } else {
        // Single-slope: increment but don't wrap yet — wrap after matches so CMP==PER is seen
        if (cnt_ < per_) ++cnt_;
        handle_matches(prev_cnt);
        if (cnt_ >= per_) { cnt_ = 0; just_hit_top_ = true; intflags_ |= 0x01; }
    }

    // Single-slope PWM: reset WO states at TOP
    if (just_hit_top_ && is_single_slope_pwm()) {
        wo_states_[0] = (ctrlc_ & 0x10) != 0;
        wo_states_[1] = (ctrlc_ & 0x20) != 0;
        wo_states_[2] = (ctrlc_ & 0x40) != 0;
        wo_states_[3] = (ctrlc_ & 0x80) != 0;
    }

    // Commit double-buffers at TOP (single-slope) or BOTTOM (dual-slope)
    if ((wg_mode < 4 && just_hit_top_) || (wg_mode >= 4 && just_hit_bottom_)) {
        commit_buffers();
    }

    update_outputs();
}

void Tc::handle_matches(u16 prev_cnt) noexcept {
    auto check_match = [&](u16 cmp, u8 flag, u8 wo_idx, u8 pol_bit) {
        if (cmp == 0) return;
        if (cnt_ == cmp || (just_hit_top_ && prev_cnt == cmp)) {
            intflags_ |= flag;
            if (is_single_slope_pwm()) wo_states_[wo_idx] = !(ctrlc_ & pol_bit);
        }
    };

    check_match(cca_, 0x10, 0, 0x10);
    check_match(ccb_, 0x20, 1, 0x20);
    if (desc_.ccc_address) check_match(ccc_, 0x40, 2, 0x40);
    if (desc_.ccd_address) check_match(ccd_, 0x80, 3, 0x80);
}

void Tc::commit_buffers() noexcept {
    // Called when just_hit_top_ or just_hit_bottom_ is true (set in perform_tick)
    if (per_buf_valid_) { per_ = per_buf_; per_buf_valid_ = false; ctrlf_ &= ~0x10; }
    if (cca_buf_valid_) { cca_ = cca_buf_; cca_buf_valid_ = false; ctrlf_ &= ~0x01; }
    if (ccb_buf_valid_) { ccb_ = ccb_buf_; ccb_buf_valid_ = false; ctrlf_ &= ~0x02; }
    if (ccc_buf_valid_) { ccc_ = ccc_buf_; ccc_buf_valid_ = false; ctrlf_ &= ~0x04; }
    if (ccd_buf_valid_) { ccd_ = ccd_buf_; ccd_buf_valid_ = false; ctrlf_ &= ~0x08; }
}

void Tc::handle_cmd(u8 cmd) noexcept {
    switch (cmd) {
        case 1: { // UPDATE
            if (!(ctrle_ & 0x10)) {
                if (per_buf_valid_) { per_ = per_buf_; per_buf_valid_ = false; ctrlf_ &= ~0x10; }
                if (cca_buf_valid_) { cca_ = cca_buf_; cca_buf_valid_ = false; ctrlf_ &= ~0x01; }
                if (ccb_buf_valid_) { ccb_ = ccb_buf_; ccb_buf_valid_ = false; ctrlf_ &= ~0x02; }
                if (ccc_buf_valid_) { ccc_ = ccc_buf_; ccc_buf_valid_ = false; ctrlf_ &= ~0x04; }
                if (ccd_buf_valid_) { ccd_ = ccd_buf_; ccd_buf_valid_ = false; ctrlf_ &= ~0x08; }
            }
            break;
        }
        case 2: // RESTART
            cnt_ = 0;
            counting_up_ = true;
            update_outputs();
            break;
        case 3: // RESET
            reset();
            break;
        default: break;
    }
}

void Tc::update_prescaler() noexcept {
    static const u64 presc_table[] = {1, 2, 4, 8, 64, 256, 1024};
    u8 clksel = get_clk_sel();
    prescaler_limit_ = (clksel == 0) ? 0 : presc_table[clksel - 1];
}

static void drive_wo_pins_direct(PinMux* pm, u8 port_idx, u8 wo_index, bool level, bool enabled) noexcept {
    if (!pm || wo_index >= 4 || port_idx >= 6) return;
    static constexpr u8 WO_PIN_BITS[4] = {4, 5, 6, 7};
    u8 pin_bit = WO_PIN_BITS[wo_index];
    if (enabled) {
        pm->claim_pin(port_idx, pin_bit, PinOwner::timer);
        pm->update_pin(port_idx, pin_bit, PinOwner::timer, true, level, false);
    } else {
        pm->release_pin(port_idx, pin_bit, PinOwner::timer);
    }
}

void Tc::update_outputs() noexcept {
    if (!port_mux_ && !pin_mux_) return;
    bool wo0_en = (ctrlb_ & 0x10) != 0;
    bool wo1_en = (ctrlb_ & 0x20) != 0;
    bool wo2_en = (ctrlb_ & 0x40) != 0;
    bool wo3_en = (ctrlb_ & 0x80) != 0;

    auto w0 = get_wo_level(0);
    auto w1 = get_wo_level(1);
    auto w2 = get_wo_level(2);
    auto w3 = get_wo_level(3);

    if (awex_companion_) {
        awex_companion_->set_wo_level(0, w0);
        awex_companion_->set_wo_level(1, w1);
        awex_companion_->set_wo_level(2, w2);
        awex_companion_->set_wo_level(3, w3);
    }

    if (port_mux_ && port_mux_->has_tc_routing()) {
        port_mux_->drive_tca0_wo(0, w0, wo0_en, port_index_);
        port_mux_->drive_tca0_wo(1, w1, wo1_en, port_index_);
        port_mux_->drive_tca0_wo(2, w2, wo2_en, port_index_);
        if (desc_.ccd_address) {
            port_mux_->drive_tca0_wo(3, w3, wo3_en, port_index_);
        } else {
            port_mux_->drive_tca0_wo(3, false, false, port_index_);
        }
    } else if (pin_mux_) {
        drive_wo_pins_direct(pin_mux_, port_index_, 0, w0, wo0_en);
        drive_wo_pins_direct(pin_mux_, port_index_, 1, w1, wo1_en);
        drive_wo_pins_direct(pin_mux_, port_index_, 2, w2, wo2_en);
        drive_wo_pins_direct(pin_mux_, port_index_, 3, w3, desc_.ccd_address ? wo3_en : false);
    }
}

bool Tc::get_wo_level(u8 index) const noexcept {
    if (index >= 4) return false;
    u16 cmp_val;
    u8 cmp_ov, cmp_pol;
    switch (index) {
        case 0: cmp_val = cca_; cmp_ov = 0x01; cmp_pol = 0x10; break;
        case 1: cmp_val = ccb_; cmp_ov = 0x02; cmp_pol = 0x20; break;
        case 2: cmp_val = ccc_; cmp_ov = 0x04; cmp_pol = 0x40; break;
        case 3: cmp_val = ccd_; cmp_ov = 0x08; cmp_pol = 0x80; break;
        default: return false;
    }
    u8 wg_mode = get_wg_mode();

    if (ctrlc_ & cmp_ov) return (ctrlc_ & cmp_pol) != 0;

    if (is_single_slope_pwm()) {
        return wo_states_[index];
    }

    bool single = wg_mode < 4;
    if (single) {
        return cnt_ >= cmp_val;
    } else {
        return counting_up_ ? cnt_ < cmp_val : cnt_ >= cmp_val;
    }
}

bool Tc::pending_interrupt_request(InterruptRequest& request) const noexcept {
    // Check if any enabled interrupt is pending
    u8 ovf_en = intctrla_ & 0x03;
    u8 err_en = (intctrla_ >> 2) & 0x03;
    u8 cca_en = intctrlb_ & 0x03;
    u8 ccb_en = (intctrlb_ >> 2) & 0x03;
    u8 ccc_en = (intctrlb_ >> 4) & 0x03;
    u8 ccd_en = (intctrlb_ >> 6) & 0x03;

    if ((intflags_ & 0x01) && ovf_en) {
        request.vector_index = desc_.ovf_vector_index;
        request.source_id = 0; return true;
    }
    if ((intflags_ & 0x02) && err_en && desc_.err_vector_index) {
        request.vector_index = desc_.err_vector_index;
        request.source_id = 0; return true;
    }
    if ((intflags_ & 0x10) && cca_en) {
        request.vector_index = desc_.cca_vector_index;
        request.source_id = 0; return true;
    }
    if ((intflags_ & 0x20) && ccb_en) {
        request.vector_index = desc_.ccb_vector_index;
        request.source_id = 0; return true;
    }
    if ((intflags_ & 0x40) && ccc_en && desc_.ccc_vector_index) {
        request.vector_index = desc_.ccc_vector_index;
        request.source_id = 0; return true;
    }
    if ((intflags_ & 0x80) && ccd_en && desc_.ccd_vector_index) {
        request.vector_index = desc_.ccd_vector_index;
        request.source_id = 0; return true;
    }
    return false;
}

bool Tc::consume_interrupt_request(InterruptRequest& request) noexcept {
    u8 ovf_en = intctrla_ & 0x03;
    u8 err_en = (intctrla_ >> 2) & 0x03;
    u8 cca_en = intctrlb_ & 0x03;
    u8 ccb_en = (intctrlb_ >> 2) & 0x03;
    u8 ccc_en = (intctrlb_ >> 4) & 0x03;
    u8 ccd_en = (intctrlb_ >> 6) & 0x03;

    if ((intflags_ & 0x01) && ovf_en) {
        request.vector_index = desc_.ovf_vector_index;
        request.source_id = 0;
        intflags_ &= ~0x01; return true;
    }
    if ((intflags_ & 0x02) && err_en && desc_.err_vector_index) {
        request.vector_index = desc_.err_vector_index;
        request.source_id = 0;
        intflags_ &= ~0x02; return true;
    }
    if ((intflags_ & 0x10) && cca_en) {
        request.vector_index = desc_.cca_vector_index;
        request.source_id = 0;
        intflags_ &= ~0x10; return true;
    }
    if ((intflags_ & 0x20) && ccb_en) {
        request.vector_index = desc_.ccb_vector_index;
        request.source_id = 0;
        intflags_ &= ~0x20; return true;
    }
    if ((intflags_ & 0x40) && ccc_en && desc_.ccc_vector_index) {
        request.vector_index = desc_.ccc_vector_index;
        request.source_id = 0;
        intflags_ &= ~0x40; return true;
    }
    if ((intflags_ & 0x80) && ccd_en && desc_.ccd_vector_index) {
        request.vector_index = desc_.ccd_vector_index;
        request.source_id = 0;
        intflags_ &= ~0x80; return true;
    }
    return false;
}

void Tc::on_event(bool level) noexcept {
    if (level == last_event_level_) return;
    last_event_level_ = level;
    if (!level) return;

    u8 evact = ctrld_ & 0x03;

    switch (evact) {
        case 0:
            perform_tick();
            break;
        case 1:
            break;
        case 2:
            if (counting_up_) {
                if (cnt_ < per_) ++cnt_;
            } else {
                if (cnt_ > 0) --cnt_;
            }
            break;
        case 3:
            cnt_ = 0;
            break;
        default: break;
    }
}

void Tc::update_interrupt_state() noexcept {
    u8 ovf_en = intctrla_ & 0x03;
    u8 cca_en = intctrlb_ & 0x03;
    bool pending = false;
    if ((intflags_ & 0x01) && ovf_en) pending = true;
    if ((intflags_ & 0x10) && cca_en) pending = true;
    if (pending) bus_->set_interrupts_dirty();
}

void Tc::update_active_state() noexcept {
    if (bus_) bus_->set_peripheral_active(this, is_enabled());
}

void Tc::on_routing_changed() noexcept {
    update_outputs();
}

u8 Tc::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.ctrlb_address) return ctrlb_;
    if (address == desc_.ctrlc_address) return ctrlc_;
    if (address == desc_.ctrld_address) return ctrld_;
    if (address == desc_.ctrle_address) return ctrle_ & 0x18;
    if (address == desc_.intctrla_address) return intctrla_;
    if (address == desc_.intctrlb_address) return intctrlb_;
    if (address == desc_.ctrlfclr_address) return ctrlf_;
    if (address == desc_.ctrlfset_address) return ctrlf_;
    if (address == desc_.ctrlgclr_address) return ctrlg_;
    if (address == desc_.ctrlgset_address) return ctrlg_;
    if (address == desc_.intflags_address) return intflags_;

    if (address == desc_.temp_address) return temp_;

    if (address == desc_.cnt_address) { temp_ = static_cast<u8>(cnt_ >> 8); return cnt_ & 0xFF; }
    if (address == desc_.cnt_address + 1) { u8 v = temp_; return v; }
    if (address == desc_.period_address) { temp_ = static_cast<u8>(per_ >> 8); return per_ & 0xFF; }
    if (address == desc_.period_address + 1) { u8 v = temp_; return v; }
    if (address == desc_.cca_address) { temp_ = static_cast<u8>(cca_ >> 8); return cca_ & 0xFF; }
    if (address == desc_.cca_address + 1) { u8 v = temp_; return v; }
    if (address == desc_.ccb_address) { temp_ = static_cast<u8>(ccb_ >> 8); return ccb_ & 0xFF; }
    if (address == desc_.ccb_address + 1) { u8 v = temp_; return v; }
    if (desc_.ccc_address) {
        if (address == desc_.ccc_address) { temp_ = static_cast<u8>(ccc_ >> 8); return ccc_ & 0xFF; }
        if (address == desc_.ccc_address + 1) { u8 v = temp_; return v; }
    }
    if (desc_.ccd_address) {
        if (address == desc_.ccd_address) { temp_ = static_cast<u8>(ccd_ >> 8); return ccd_ & 0xFF; }
        if (address == desc_.ccd_address + 1) { u8 v = temp_; return v; }
    }
    if (address == desc_.perbuf_address) { temp_ = static_cast<u8>(per_buf_ >> 8); return per_buf_ & 0xFF; }
    if (address == desc_.perbuf_address + 1) { u8 v = temp_; return v; }
    if (address == desc_.ccabuf_address) { temp_ = static_cast<u8>(cca_buf_ >> 8); return cca_buf_ & 0xFF; }
    if (address == desc_.ccabuf_address + 1) { u8 v = temp_; return v; }
    if (address == desc_.ccbbuf_address) { temp_ = static_cast<u8>(ccb_buf_ >> 8); return ccb_buf_ & 0xFF; }
    if (address == desc_.ccbbuf_address + 1) { u8 v = temp_; return v; }
    if (desc_.cccbuf_address) {
        if (address == desc_.cccbuf_address) { temp_ = static_cast<u8>(ccc_buf_ >> 8); return ccc_buf_ & 0xFF; }
        if (address == desc_.cccbuf_address + 1) { u8 v = temp_; return v; }
    }
    if (desc_.ccdbuf_address) {
        if (address == desc_.ccdbuf_address) { temp_ = static_cast<u8>(ccd_buf_ >> 8); return ccd_buf_ & 0xFF; }
        if (address == desc_.ccdbuf_address + 1) { u8 v = temp_; return v; }
    }
    return 0;
}

void Tc::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value & 0x8F;
        update_prescaler();
        update_active_state();
        update_interrupt_state();
    }
    else if (address == desc_.ctrlb_address) {
        u8 old_ctrlb = ctrlb_;
        ctrlb_ = value & 0xF7;
        if (!wg_mode_in_ctrld_) {
            u8 new_wg = ctrlb_ & 0x07;
            u8 old_wg = old_ctrlb & 0x07;
            if (new_wg == 1 && old_wg != 1) {
                wo_states_[0] = (ctrlc_ & 0x10) != 0;
                wo_states_[1] = (ctrlc_ & 0x20) != 0;
                wo_states_[2] = (ctrlc_ & 0x40) != 0;
                wo_states_[3] = (ctrlc_ & 0x80) != 0;
            }
        } else {
            if ((ctrlb_ & 0x10) && !(old_ctrlb & 0x10) && is_single_slope_pwm()) {
                wo_states_[0] = (ctrlc_ & 0x10) != 0;
                wo_states_[1] = (ctrlc_ & 0x20) != 0;
                wo_states_[2] = (ctrlc_ & 0x40) != 0;
                wo_states_[3] = (ctrlc_ & 0x80) != 0;
            }
        }
        update_outputs();
    }
    else if (address == desc_.ctrlc_address) {
        ctrlc_ = value;
        if (is_single_slope_pwm()) {
            wo_states_[0] = (ctrlc_ & 0x10) != 0;
            wo_states_[1] = (ctrlc_ & 0x20) != 0;
            wo_states_[2] = (ctrlc_ & 0x40) != 0;
            wo_states_[3] = (ctrlc_ & 0x80) != 0;
        }
    }
    else if (address == desc_.ctrld_address) {
        u8 old_ctrld = ctrld_;
        ctrld_ = value & 0x1F;
        if (wg_mode_in_ctrld_) {
            u8 new_wg = ctrld_ & 0x07;
            u8 old_wg = old_ctrld & 0x07;
            if (new_wg == 3 && old_wg != 3) {
                wo_states_[0] = (ctrlc_ & 0x10) != 0;
                wo_states_[1] = (ctrlc_ & 0x20) != 0;
                wo_states_[2] = (ctrlc_ & 0x40) != 0;
                wo_states_[3] = (ctrlc_ & 0x80) != 0;
            }
            update_outputs();
        }
    }
    else if (address == desc_.ctrle_address) {
        ctrle_ = value & 0x18;
        handle_cmd(value & 0x07);
    }
    else if (address == desc_.intctrla_address) intctrla_ = value & 0x0F;
    else if (address == desc_.intctrlb_address) intctrlb_ = value;
    else if (address == desc_.ctrlfclr_address) {
        ctrlf_ &= ~(value & 0x1F);
        if (value & 0x01) cca_buf_valid_ = false;
        if (value & 0x02) ccb_buf_valid_ = false;
        if (value & 0x04) ccc_buf_valid_ = false;
        if (value & 0x08) ccd_buf_valid_ = false;
        if (value & 0x10) per_buf_valid_ = false;
    }
    else if (address == desc_.ctrlfset_address) {
        ctrlf_ |= (value & 0x1F);
    }
    else if (address == desc_.ctrlgclr_address) ctrlg_ &= ~value;
    else if (address == desc_.ctrlgset_address) ctrlg_ |= value;
    else if (address == desc_.intflags_address) intflags_ &= ~value;
    else if (address == desc_.temp_address) temp_ = value;
    else if (address == desc_.cnt_address) { cnt_ = (cnt_ & 0xFF00) | value; }
    else if (address == desc_.cnt_address + 1) { cnt_ = (static_cast<u16>(value) << 8) | (cnt_ & 0xFF); }
    else if (address == desc_.period_address) { per_ = (per_ & 0xFF00) | value; }
    else if (address == desc_.period_address + 1) { per_ = (static_cast<u16>(value) << 8) | (per_ & 0xFF); }
    else if (address == desc_.cca_address) { cca_ = (cca_ & 0xFF00) | value; }
    else if (address == desc_.cca_address + 1) { cca_ = (static_cast<u16>(value) << 8) | (cca_ & 0xFF); }
    else if (address == desc_.ccb_address) { ccb_ = (ccb_ & 0xFF00) | value; }
    else if (address == desc_.ccb_address + 1) { ccb_ = (static_cast<u16>(value) << 8) | (ccb_ & 0xFF); }
    else if (desc_.ccc_address && address == desc_.ccc_address) { ccc_ = (ccc_ & 0xFF00) | value; }
    else if (desc_.ccc_address && address == desc_.ccc_address + 1) { ccc_ = (static_cast<u16>(value) << 8) | (ccc_ & 0xFF); }
    else if (desc_.ccd_address && address == desc_.ccd_address) { ccd_ = (ccd_ & 0xFF00) | value; }
    else if (desc_.ccd_address && address == desc_.ccd_address + 1) { ccd_ = (static_cast<u16>(value) << 8) | (ccd_ & 0xFF); }
    else if (address == desc_.perbuf_address) { per_buf_ = (per_buf_ & 0xFF00) | value; }
    else if (address == desc_.perbuf_address + 1) { per_buf_ = (static_cast<u16>(value) << 8) | (per_buf_ & 0xFF); per_buf_valid_ = true; ctrlf_ |= 0x10; }
    else if (address == desc_.ccabuf_address) { cca_buf_ = (cca_buf_ & 0xFF00) | value; }
    else if (address == desc_.ccabuf_address + 1) { cca_buf_ = (static_cast<u16>(value) << 8) | (cca_buf_ & 0xFF); cca_buf_valid_ = true; ctrlf_ |= 0x01; }
    else if (address == desc_.ccbbuf_address) { ccb_buf_ = (ccb_buf_ & 0xFF00) | value; }
    else if (address == desc_.ccbbuf_address + 1) { ccb_buf_ = (static_cast<u16>(value) << 8) | (ccb_buf_ & 0xFF); ccb_buf_valid_ = true; ctrlf_ |= 0x02; }
    else if (desc_.cccbuf_address && address == desc_.cccbuf_address) { ccc_buf_ = (ccc_buf_ & 0xFF00) | value; }
    else if (desc_.cccbuf_address && address == desc_.cccbuf_address + 1) { ccc_buf_ = (static_cast<u16>(value) << 8) | (ccc_buf_ & 0xFF); ccc_buf_valid_ = true; ctrlf_ |= 0x04; }
    else if (desc_.ccdbuf_address && address == desc_.ccdbuf_address) { ccd_buf_ = (ccd_buf_ & 0xFF00) | value; }
    else if (desc_.ccdbuf_address && address == desc_.ccdbuf_address + 1) { ccd_buf_ = (static_cast<u16>(value) << 8) | (ccd_buf_ & 0xFF); ccd_buf_valid_ = true; ctrlf_ |= 0x08; }
}

} // namespace vioavr::core
