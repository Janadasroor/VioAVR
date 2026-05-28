#include "vioavr/core/tce.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/evsys.hpp"
#include "vioavr/core/port_mux.hpp"

namespace vioavr::core {

Tce::Tce(std::string name, const TceDescriptor& desc) noexcept
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
    add_range(desc_.ctrleclr_address);
    add_range(desc_.ctrleset_address);
    add_range(desc_.ctrlfclr_address);
    add_range(desc_.ctrlfset_address);
    add_range(desc_.evgenctrl_address);
    add_range(desc_.evctrl_address);
    add_range(desc_.intctrl_address);
    add_range(desc_.intflags_address);
    add_range(desc_.dbgctrl_address);
    add_range(desc_.tcnt_address);
    add_range(desc_.period_address);
    add_range(desc_.cmp0_address);
    add_range(desc_.cmp1_address);
    add_range(desc_.cmp2_address);
    add_range(desc_.perbuf_address);
    add_range(desc_.cmp0buf_address);
    add_range(desc_.cmp1buf_address);
    add_range(desc_.cmp2buf_address);
    for (; ri < ranges_.size(); ++ri) ranges_[ri] = {0, 0};
}

std::span<const AddressRange> Tce::mapped_ranges() const noexcept {
    if (desc_.ctrla_address == 0) return {};
    size_t count = 0;
    for (auto& r : ranges_) { if (r.begin == 0 && r.end == 0) break; ++count; }
    return {ranges_.data(), count};
}

void Tce::reset() noexcept {
    ctrla_ = 0;
    ctrlb_ = 0;
    ctrlc_ = 0;
    ctrle_ = 0;
    ctrlf_ = 0;
    evgenctrl_ = 0;
    evctrl_ = 0;
    intctrl_ = 0;
    intflags_ = 0;
    dbgctrl_ = 0;
    temp_ = 0;
    tcnt_ = 0;
    per_ = 0xFFFF;
    cmp0_ = 0;
    cmp1_ = 0;
    cmp2_ = 0;
    per_buf_ = 0;
    cmp0_buf_ = 0;
    cmp1_buf_ = 0;
    cmp2_buf_ = 0;
    per_buf_valid_ = false;
    cmp0_buf_valid_ = false;
    cmp1_buf_valid_ = false;
    cmp2_buf_valid_ = false;
    prescaler_counter_ = 0;
    prescaler_limit_ = 1;
    counting_up_ = true;
    last_event_level_ = false;
    wo_states_[0] = false;
    wo_states_[1] = false;
    wo_states_[2] = false;
    update_interrupt_state();
    update_active_state();
}

u8 Tce::reg_offset(u16 address) const noexcept {
    return static_cast<u8>(address - desc_.ctrla_address);
}

bool Tce::is_enabled() const noexcept {
    return (ctrla_ & 0x01) != 0;
}

u8 Tce::get_prescaler() const noexcept {
    return (ctrla_ >> 1) & 0x07;
}

u8 Tce::get_wg_mode() const noexcept {
    return ctrlb_ & 0x07;
}

void Tce::set_event_system(EventSystem* evsys) noexcept {
    evsys_ = evsys;
    if (evsys_ && desc_.user_event_address != 0) {
        u16 base = evsys_->users_base();
        if (desc_.user_event_address >= base) {
            u8 idx = static_cast<u8>(desc_.user_event_address - base);
            evsys_->register_user_callback(idx, [this](bool level) {
                on_event(level);
            });
        }
    }
}

void Tce::tick(u64 elapsed_cycles) noexcept {
    if (elapsed_cycles == 0 || !is_enabled()) return;

    bool event_counting = (evctrl_ & 0x01) && ((evctrl_ >> 1) & 0x07) == 0;
    if (event_counting) return;

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

void Tce::perform_tick() noexcept {
    u8 wg_mode = get_wg_mode();
    bool update_cond = false;

    if (wg_mode == 5) {
        if (counting_up_) {
            ++tcnt_;
            if (tcnt_ >= per_) {
                tcnt_ = per_;
                counting_up_ = false;
                update_cond = true;
            }
        } else {
            if (tcnt_ > 0) --tcnt_;
            if (tcnt_ == 0) {
                counting_up_ = true;
                update_cond = true;
                intflags_ |= 0x01;
            }
        }
    } else if (wg_mode == 1) {
        ++tcnt_;
        if (cmp0_ > 0 && tcnt_ >= cmp0_) {
            tcnt_ = 0;
            wo_states_[0] = !wo_states_[0];
            update_cond = true;
        }
    } else {
        ++tcnt_;
        if (tcnt_ >= per_) {
            tcnt_ = 0;
            update_cond = true;
            intflags_ |= 0x01;
        }
    }

    handle_matches();

    if (update_cond) {
        bool at_top = (wg_mode != 5) || !counting_up_;
        if ((intflags_ & 0x01) && evsys_ && desc_.ovf_generator_id != 0) {
            evsys_->trigger_event(desc_.ovf_generator_id);
        }
        if (at_top || wg_mode != 5) {
            if (per_buf_valid_) { per_ = per_buf_; per_buf_valid_ = false; ctrlf_ &= ~0x01; }
            if (cmp0_buf_valid_) { cmp0_ = cmp0_buf_; cmp0_buf_valid_ = false; ctrlf_ &= ~0x02; }
            if (cmp1_buf_valid_) { cmp1_ = cmp1_buf_; cmp1_buf_valid_ = false; ctrlf_ &= ~0x04; }
            if (cmp2_buf_valid_) { cmp2_ = cmp2_buf_; cmp2_buf_valid_ = false; ctrlf_ &= ~0x08; }
        }
    }

    update_outputs();
}

void Tce::handle_matches() noexcept {
    if (cmp0_ > 0 && tcnt_ == cmp0_) {
        intflags_ |= 0x10;
        if (evsys_ && (evgenctrl_ & 0x10) && desc_.cmp0_generator_id != 0) {
            evsys_->trigger_event(desc_.cmp0_generator_id);
        }
    }
    if (cmp1_ > 0 && tcnt_ == cmp1_) {
        intflags_ |= 0x20;
        if (evsys_ && (evgenctrl_ & 0x20) && desc_.cmp1_generator_id != 0) {
            evsys_->trigger_event(desc_.cmp1_generator_id);
        }
    }
    if (cmp2_ > 0 && tcnt_ == cmp2_) {
        intflags_ |= 0x40;
        if (evsys_ && (evgenctrl_ & 0x40) && desc_.cmp2_generator_id != 0) {
            evsys_->trigger_event(desc_.cmp2_generator_id);
        }
    }
}

void Tce::handle_cmd(u8 cmd) noexcept {
    switch (cmd) {
        case 1: {
            if (!(ctrle_ & 0x10)) {
                if (per_buf_valid_) { per_ = per_buf_; per_buf_valid_ = false; ctrlf_ &= ~0x01; }
                if (cmp0_buf_valid_) { cmp0_ = cmp0_buf_; cmp0_buf_valid_ = false; ctrlf_ &= ~0x02; }
                if (cmp1_buf_valid_) { cmp1_ = cmp1_buf_; cmp1_buf_valid_ = false; ctrlf_ &= ~0x04; }
                if (cmp2_buf_valid_) { cmp2_ = cmp2_buf_; cmp2_buf_valid_ = false; ctrlf_ &= ~0x08; }
            }
            break;
        }
        case 2:
            reset();
            break;
        case 3:
            tcnt_ = 0;
            counting_up_ = true;
            intflags_ = 0;
            if (!(ctrle_ & 0x10)) {
                if (per_buf_valid_) { per_ = per_buf_; per_buf_valid_ = false; ctrlf_ &= ~0x01; }
                if (cmp0_buf_valid_) { cmp0_ = cmp0_buf_; cmp0_buf_valid_ = false; ctrlf_ &= ~0x02; }
                if (cmp1_buf_valid_) { cmp1_ = cmp1_buf_; cmp1_buf_valid_ = false; ctrlf_ &= ~0x04; }
                if (cmp2_buf_valid_) { cmp2_ = cmp2_buf_; cmp2_buf_valid_ = false; ctrlf_ &= ~0x08; }
            }
            update_interrupt_state();
            update_outputs();
            break;
        default: break;
    }
}

void Tce::update_prescaler() noexcept {
    static const u64 presc_table[] = {1, 2, 4, 8, 16, 64, 256, 1024};
    prescaler_limit_ = presc_table[get_prescaler()];
}

void Tce::update_outputs() noexcept {
    if (!port_mux_) return;
    // We drive via PortMux similar to TCA WO outputs
    // TCE has 3 WO channels (WO0 = CMP0, WO1 = CMP1, WO2 = CMP2)
    bool wo0_en = (ctrlb_ & 0x10) != 0;
    bool wo1_en = (ctrlb_ & 0x20) != 0;
    bool wo2_en = (ctrlb_ & 0x40) != 0;
    port_mux_->drive_tca0_wo(0, get_wo_level(0), wo0_en);
    port_mux_->drive_tca0_wo(1, get_wo_level(1), wo1_en);
    port_mux_->drive_tca0_wo(2, get_wo_level(2), wo2_en);
}

bool Tce::get_wo_level(u8 index) const noexcept {
    if (index >= 3) return false;
    u8 cmp_ov = (index == 0) ? 0x01 : (index == 1) ? 0x02 : 0x04;
    u8 cmp_pol = (index == 0) ? 0x10 : (index == 1) ? 0x20 : 0x40;
    if (ctrlc_ & cmp_ov) return (ctrlc_ & cmp_pol) != 0;

    u16 cmp_val = (index == 0) ? cmp0_ : (index == 1) ? cmp1_ : cmp2_;
    u8 wg_mode = get_wg_mode();
    if (wg_mode == 1) {
        return wo_states_[index];
    }
    bool level = tcnt_ < cmp_val;
    if (ctrlc_ & cmp_pol) level = !level;
    return level;
}

bool Tce::pending_interrupt_request(InterruptRequest& request) const noexcept {
    u8 pending = intflags_ & intctrl_;
    if (pending & 0x01) {
        request.vector_index = desc_.ovf_vector_index;
        request.source_id = 0; return true;
    }
    if (pending & 0x10) {
        request.vector_index = desc_.cmp0_vector_index;
        request.source_id = 0; return true;
    }
    if (pending & 0x20) {
        request.vector_index = desc_.cmp1_vector_index;
        request.source_id = 0; return true;
    }
    if (pending & 0x40) {
        request.vector_index = desc_.cmp2_vector_index;
        request.source_id = 0; return true;
    }
    return false;
}

bool Tce::consume_interrupt_request(InterruptRequest& request) noexcept {
    u8 pending = intflags_ & intctrl_;
    if (pending & 0x01) {
        request.vector_index = desc_.ovf_vector_index;
        request.source_id = 0;
        intflags_ &= ~0x01; return true;
    }
    if (pending & 0x10) {
        request.vector_index = desc_.cmp0_vector_index;
        request.source_id = 0;
        intflags_ &= ~0x10; return true;
    }
    if (pending & 0x20) {
        request.vector_index = desc_.cmp1_vector_index;
        request.source_id = 0;
        intflags_ &= ~0x20; return true;
    }
    if (pending & 0x40) {
        request.vector_index = desc_.cmp2_vector_index;
        request.source_id = 0;
        intflags_ &= ~0x40; return true;
    }
    return false;
}

void Tce::on_event(bool level) noexcept {
    if (level == last_event_level_) return;
    last_event_level_ = level;
    if (!level) return;

    u8 evact = (evctrl_ >> 1) & 0x07;

    switch (evact) {
        case 0: perform_tick(); break;
        case 1:
            tcnt_++;
            if (cmp0_ > 0 && tcnt_ >= cmp0_) {
                tcnt_ = 0;
                intflags_ |= 0x01;
                update_outputs();
            }
            break;
        case 2: break;
        case 3:
            tcnt_ = 0;
            break;
        default: break;
    }
}

void Tce::update_interrupt_state() noexcept {
    bool pending = (intflags_ & intctrl_) != 0;
    if (pending) bus_->set_interrupts_dirty();
}

void Tce::update_active_state() noexcept {
    if (bus_) bus_->set_peripheral_active(this, is_enabled());
}

void Tce::on_routing_changed() noexcept {
    update_outputs();
}

u8 Tce::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.ctrlb_address) return ctrlb_;
    if (address == desc_.ctrlc_address) return ctrlc_;
    if (address == desc_.ctrleclr_address || address == desc_.ctrleset_address) return ctrle_ & 0x18;
    if (address == desc_.ctrlfclr_address || address == desc_.ctrlfset_address) return ctrlf_;
    if (address == desc_.evgenctrl_address) return evgenctrl_;
    if (address == desc_.evctrl_address) return evctrl_;
    if (address == desc_.intctrl_address) return intctrl_;
    if (address == desc_.intflags_address) return intflags_;
    if (address == desc_.dbgctrl_address) return dbgctrl_;

    // 16-bit registers via TEMP
    if (address == desc_.tcnt_address) { temp_ = static_cast<u8>(tcnt_ >> 8); return tcnt_ & 0xFF; }
    if (address == desc_.tcnt_address + 1) { u8 v = temp_; temp_ = 0; return v; }

    if (address == desc_.period_address) { temp_ = static_cast<u8>(per_ >> 8); return per_ & 0xFF; }
    if (address == desc_.period_address + 1) { u8 v = temp_; temp_ = 0; return v; }

    if (address == desc_.cmp0_address) { temp_ = static_cast<u8>(cmp0_ >> 8); return cmp0_ & 0xFF; }
    if (address == desc_.cmp0_address + 1) { u8 v = temp_; temp_ = 0; return v; }

    if (address == desc_.cmp1_address) { temp_ = static_cast<u8>(cmp1_ >> 8); return cmp1_ & 0xFF; }
    if (address == desc_.cmp1_address + 1) { u8 v = temp_; temp_ = 0; return v; }

    if (address == desc_.cmp2_address) { temp_ = static_cast<u8>(cmp2_ >> 8); return cmp2_ & 0xFF; }
    if (address == desc_.cmp2_address + 1) { u8 v = temp_; temp_ = 0; return v; }

    if (address == desc_.perbuf_address) { temp_ = static_cast<u8>(per_buf_ >> 8); return per_buf_ & 0xFF; }
    if (address == desc_.perbuf_address + 1) { u8 v = temp_; temp_ = 0; return v; }

    if (address == desc_.cmp0buf_address) { temp_ = static_cast<u8>(cmp0_buf_ >> 8); return cmp0_buf_ & 0xFF; }
    if (address == desc_.cmp0buf_address + 1) { u8 v = temp_; temp_ = 0; return v; }

    if (address == desc_.cmp1buf_address) { temp_ = static_cast<u8>(cmp1_buf_ >> 8); return cmp1_buf_ & 0xFF; }
    if (address == desc_.cmp1buf_address + 1) { u8 v = temp_; temp_ = 0; return v; }

    if (address == desc_.cmp2buf_address) { temp_ = static_cast<u8>(cmp2_buf_ >> 8); return cmp2_buf_ & 0xFF; }
    if (address == desc_.cmp2buf_address + 1) { u8 v = temp_; temp_ = 0; return v; }

    if (address == desc_.temp_address) return temp_;
    return 0;
}

void Tce::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value & 0x8F;
        update_prescaler();
        update_active_state();
        update_interrupt_state();
    }
    else if (address == desc_.ctrlb_address) ctrlb_ = value & 0x7F;
    else if (address == desc_.ctrlc_address) ctrlc_ = value & 0x77;
    else if (address == desc_.ctrleclr_address) {
        if (value & 0x08) ctrle_ &= ~0x08;
        if (value & 0x10) ctrle_ &= ~0x10;
        handle_cmd(value & 0x07);
    }
    else if (address == desc_.ctrleset_address) {
        if (value & 0x08) ctrle_ |= 0x08;
        if (value & 0x10) ctrle_ |= 0x10;
        handle_cmd(value & 0x07);
    }
    else if (address == desc_.ctrlfclr_address) ctrlf_ &= ~value;
    else if (address == desc_.ctrlfset_address) ctrlf_ |= value;
    else if (address == desc_.evgenctrl_address) evgenctrl_ = value & 0x70;
    else if (address == desc_.evctrl_address) evctrl_ = value;
    else if (address == desc_.intctrl_address) intctrl_ = value;
    else if (address == desc_.intflags_address) intflags_ &= ~value;
    else if (address == desc_.dbgctrl_address) dbgctrl_ = value & 0x01;
    // 16-bit registers via TEMP
    else if (address == desc_.tcnt_address) { tcnt_ = (tcnt_ & 0xFF00) | value; }
    else if (address == desc_.tcnt_address + 1) { tcnt_ = (static_cast<u16>(value) << 8) | (tcnt_ & 0xFF); }
    else if (address == desc_.period_address) { per_ = (per_ & 0xFF00) | value; }
    else if (address == desc_.period_address + 1) { per_ = (static_cast<u16>(value) << 8) | (per_ & 0xFF); }
    else if (address == desc_.cmp0_address) { cmp0_ = (cmp0_ & 0xFF00) | value; }
    else if (address == desc_.cmp0_address + 1) { cmp0_ = (static_cast<u16>(value) << 8) | (cmp0_ & 0xFF); }
    else if (address == desc_.cmp1_address) { cmp1_ = (cmp1_ & 0xFF00) | value; }
    else if (address == desc_.cmp1_address + 1) { cmp1_ = (static_cast<u16>(value) << 8) | (cmp1_ & 0xFF); }
    else if (address == desc_.cmp2_address) { cmp2_ = (cmp2_ & 0xFF00) | value; }
    else if (address == desc_.cmp2_address + 1) { cmp2_ = (static_cast<u16>(value) << 8) | (cmp2_ & 0xFF); }
    else if (address == desc_.perbuf_address) { per_buf_ = (per_buf_ & 0xFF00) | value; }
    else if (address == desc_.perbuf_address + 1) { per_buf_ = (static_cast<u16>(value) << 8) | (per_buf_ & 0xFF); per_buf_valid_ = true; ctrlf_ |= 0x01; }
    else if (address == desc_.cmp0buf_address) { cmp0_buf_ = (cmp0_buf_ & 0xFF00) | value; }
    else if (address == desc_.cmp0buf_address + 1) { cmp0_buf_ = (static_cast<u16>(value) << 8) | (cmp0_buf_ & 0xFF); cmp0_buf_valid_ = true; ctrlf_ |= 0x02; }
    else if (address == desc_.cmp1buf_address) { cmp1_buf_ = (cmp1_buf_ & 0xFF00) | value; }
    else if (address == desc_.cmp1buf_address + 1) { cmp1_buf_ = (static_cast<u16>(value) << 8) | (cmp1_buf_ & 0xFF); cmp1_buf_valid_ = true; ctrlf_ |= 0x04; }
    else if (address == desc_.cmp2buf_address) { cmp2_buf_ = (cmp2_buf_ & 0xFF00) | value; }
    else if (address == desc_.cmp2buf_address + 1) { cmp2_buf_ = (static_cast<u16>(value) << 8) | (cmp2_buf_ & 0xFF); cmp2_buf_valid_ = true; ctrlf_ |= 0x08; }
    else if (address == desc_.temp_address) temp_ = value;
}

} // namespace vioavr::core
