#include "vioavr/core/tca.hpp"
#include "vioavr/core/port_mux.hpp"
#include "vioavr/core/evsys.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/logger.hpp"
#include <algorithm>

namespace vioavr::core {

Tca::Tca(std::string name, const TcaDescriptor& desc) 
    : name_(std::move(name)), desc_(desc) {
    if (desc_.ctrla_address != 0U) {
        std::vector<u16> addrs = {
            desc_.ctrla_address, desc_.ctrlb_address, desc_.ctrlc_address,
            desc_.ctrld_address, desc_.ctrleclr_address, desc_.ctrleset_address,
            desc_.ctrlfclr_address, desc_.ctrlfset_address, desc_.evctrl_address,
            desc_.intctrl_address, desc_.intflags_address, desc_.dbgctrl_address,
            desc_.temp_address,
            desc_.tcnt_address, static_cast<u16>(desc_.tcnt_address + 1),
            desc_.period_address, static_cast<u16>(desc_.period_address + 1),
            desc_.cmp0_address, static_cast<u16>(desc_.cmp0_address + 1),
            desc_.cmp1_address, static_cast<u16>(desc_.cmp1_address + 1),
            desc_.cmp2_address, static_cast<u16>(desc_.cmp2_address + 1)
        };
        std::sort(addrs.begin(), addrs.end());
        addrs.erase(std::unique(addrs.begin(), addrs.end()), addrs.end());
        
        size_t ri = 0;
        for (u16 addr : addrs) {
            if (addr == 0) continue;
            if (ri > 0 && addr == ranges_[ri-1].end + 1) {
                ranges_[ri-1].end = addr;
            } else if (ri < ranges_.size()) {
                ranges_[ri++] = {addr, addr};
            }
        }
    }
}

void Tca::reset() noexcept {
    ctrla_ = 0;
    ctrlb_ = 0;
    ctrlc_ = 0;
    ctrld_ = 0;
    evctrl_ = 0;
    intctrl_ = 0;
    intflags_ = 0;
    dbgctrl_ = 0;
    temp_ = 0;
    ctrle_ = 0;
    ctrlf_ = 0;
    norm_ = { .tcnt = 0x0000, .per = 0xFFFF, .cmp0 = 0, .cmp1 = 0, .cmp2 = 0 };
    buf_ = { .per = 0xFFFF, .cmp0 = 0, .cmp1 = 0, .cmp2 = 0, .per_valid = false, .cmp0_valid = false, .cmp1_valid = false, .cmp2_valid = false };
    counting_up_ = true;
    last_event_level_a_ = false;
    last_event_level_b_ = false;
    prescaler_counter_ = 0;
    prescaler_limit_ = 1;
    wo_states_.fill(false);
    update_interrupt_state();
    update_active_state();
}

std::span<const AddressRange> Tca::mapped_ranges() const noexcept {
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

void Tca::set_event_system(EventSystem* evsys) noexcept {
    evsys_ = evsys;
    if (evsys_ == nullptr) return;
    const u16 base = evsys_->users_base();
    if (desc_.user_event_address >= base && desc_.user_event_address != 0U) {
        const u8 idx = static_cast<u8>(desc_.user_event_address - base);
        evsys_->register_user_callback(idx, [this](bool level) {
            this->on_event(level);
        });
    }
    // AVR DA/DB drive a second event user (CNTB) from its own USER register.
    if (desc_.user_event_b_address >= base && desc_.user_event_b_address != 0U) {
        const u8 idx = static_cast<u8>(desc_.user_event_b_address - base);
        evsys_->register_user_callback(idx, [this](bool level) {
            this->on_event_b(level);
        });
    }
}

void Tca::update_direction_from_events() noexcept {
    // EVACTA == UPDOWN and/or EVACTB == UPDOWN. When both inputs drive the
    // direction the event levels are OR'ed and the counter counts up only
    // while both are low (DS40002233A 21.3.4).
    const bool a_dir = evact_a() == 3U && (evctrl_ & 0x01U) != 0U;
    const bool b_dir = evact_b() == 3U && (evctrl_ & 0x10U) != 0U;
    if (a_dir && b_dir) {
        counting_up_ = !(last_event_level_a_ || last_event_level_b_);
    } else if (a_dir) {
        counting_up_ = !last_event_level_a_;
    } else if (b_dir) {
        counting_up_ = !last_event_level_b_;
    }
}

void Tca::on_event(bool level) noexcept {
    // EVCTRL decoding is shared by ATmega4809 (EVACT[2:0] + CNTEI) and
    // AVR DA/DB (EVACTA[2:0] + CNTAEI); both number the actions the same
    // way: 0 POSEDGE, 1 ANYEDGE, 2 HIGHLVL, 3 UPDOWN, 4-6 RESTART_*.
    const bool rising = level && !last_event_level_a_;
    const bool falling = !level && last_event_level_a_;
    last_event_level_a_ = level;

    // "Event inputs are not used in Split mode" (DS40002233A 21.3.4).
    if (is_split_mode()) return;

    const u8 action = evact_a();
    const bool cnte = (evctrl_ & 0x01U) != 0U;

    if (is_enabled() && cnte) {
        switch (action) {
        case 0: // CNT_POSEDGE: each positive edge is one counter clock
            if (rising) perform_tick();
            break;
        case 1: // CNT_ANYEDGE
            if (rising || falling) perform_tick();
            break;
        case 2: // CNT_HIGHLVL: counting is gated in tick()
            break;
        case 3: // UPDOWN: count direction follows the event level(s)
            update_direction_from_events();
            break;
        case 4: // RESTART_POSEDGE
        case 6: // RESTART_HIGHLVL
            if (rising) handle_cmd(2);
            break;
        case 5: // RESTART_ANYEDGE
            if (rising || falling) handle_cmd(2);
            break;
        default:
            break;
        }
    }
    if (is_enabled() && cnte) update_interrupt_state();
}

void Tca::on_event_b(bool level) noexcept {
    // AVR DA/DB input B: EVACTB[2:0] (bits 7:5) + CNTBEI (bit 4). Only
    // UPDOWN (0x3) and the restart actions (0x4-0x6) exist for input B;
    // 0x0 is NONE and 0x1/0x2 are reserved (DS40002233A 21.5.4).
    const bool rising = level && !last_event_level_b_;
    const bool falling = !level && last_event_level_b_;
    last_event_level_b_ = level;

    if (is_split_mode()) return;
    if (!is_enabled() || (evctrl_ & 0x10U) == 0U) return;

    switch (evact_b()) {
    case 3: // UPDOWN: the event level controls the count direction
        update_direction_from_events();
        break;
    case 4: // RESTART_POSEDGE
    case 6: // RESTART_HIGHLVL
        if (rising) handle_cmd(2);
        break;
    case 5: // RESTART_ANYEDGE
        if (rising || falling) handle_cmd(2);
        break;
    default:
        break;
    }
    update_interrupt_state();
}

u8 Tca::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.ctrlb_address) return ctrlb_;
    if (address == desc_.ctrlc_address) return ctrlc_;
    if (address == desc_.ctrld_address) return ctrld_;
    if (address == desc_.evctrl_address) return evctrl_;
    if (address == desc_.intctrl_address) return intctrl_;
    if (address == desc_.intflags_address) return intflags_;
    if (address == desc_.dbgctrl_address) return dbgctrl_;
    if (address == desc_.temp_address) return temp_;
    if (address == desc_.ctrleclr_address || address == desc_.ctrleset_address) {
        // Normal mode holds DIR and LUPD; CMD always reads as 0. Split mode
        // has CMD/CMDEN only, and both are write-only.
        return is_split_mode() ? 0U : (ctrle_ & 0x03U);
    }
    if (address == desc_.ctrlfclr_address || address == desc_.ctrlfset_address) {
        return ctrlf_;
    }

    if (is_split_mode()) {
        // Split Mode Aliases skip TEMP
        if (address == desc_.tcnt_address) return cnt_l();
        if (address == desc_.tcnt_address + 1) return cnt_h();
        if (address == desc_.period_address) return per_l();
        if (address == desc_.period_address + 1) return per_h();
        if (address == desc_.cmp0_address) return cmp0_l();
        if (address == desc_.cmp0_address + 1) return cmp0_h();
        if (address == desc_.cmp1_address) return cmp1_l();
        if (address == desc_.cmp1_address + 1) return cmp1_h();
        if (address == desc_.cmp2_address) return cmp2_l();
        if (address == desc_.cmp2_address + 1) return cmp2_h();
        // Split mode: BUF addresses map to H-timer registers
        if (address == desc_.perbuf_address) return per_h() & 0x0FU;
        if (address == desc_.cmp0buf_address) return cmp0_h();
        if (address == desc_.cmp1buf_address) return cmp1_h();
        if (address == desc_.cmp2buf_address) return cmp2_h();
        
        return 0;
    }

    // Normal (16-bit) Mode: Low byte first latches High byte into TEMP
    if (address == desc_.tcnt_address) {
        temp_ = static_cast<u8>((norm_.tcnt >> 8U) & 0xFFU);
        return static_cast<u8>(norm_.tcnt & 0xFFU);
    }
    if (address == desc_.tcnt_address + 1) return temp_;

    if (address == desc_.period_address) {
        temp_ = static_cast<u8>((norm_.per >> 8U) & 0xFFU);
        return static_cast<u8>(norm_.per & 0xFFU);
    }
    if (address == desc_.period_address + 1) return temp_;

    if (address == desc_.cmp0_address) {
        temp_ = static_cast<u8>((norm_.cmp0 >> 8U) & 0xFFU);
        return static_cast<u8>(norm_.cmp0 & 0xFFU);
    }
    if (address == desc_.cmp0_address + 1) return temp_;

    if (address == desc_.cmp1_address) {
        temp_ = static_cast<u8>((norm_.cmp1 >> 8U) & 0xFFU);
        return static_cast<u8>(norm_.cmp1 & 0xFFU);
    }
    if (address == desc_.cmp1_address + 1) return temp_;

    if (address == desc_.cmp2_address) {
        temp_ = static_cast<u8>((norm_.cmp2 >> 8U) & 0xFFU);
        return static_cast<u8>(norm_.cmp2 & 0xFFU);
    }
    if (address == desc_.cmp2_address + 1) return temp_;

    // Buffer Registers also use TEMP
    if (address == desc_.perbuf_address) {
        temp_ = static_cast<u8>((buf_.per >> 8U) & 0xFFU);
        return static_cast<u8>(buf_.per & 0xFFU);
    }
    if (address == desc_.perbuf_address + 1) return temp_;

    if (address == desc_.cmp0buf_address) {
        temp_ = static_cast<u8>((buf_.cmp0 >> 8U) & 0xFFU);
        return static_cast<u8>(buf_.cmp0 & 0xFFU);
    }
    if (address == desc_.cmp0buf_address + 1) return temp_;

    if (address == desc_.cmp1buf_address) {
        temp_ = static_cast<u8>((buf_.cmp1 >> 8U) & 0xFFU);
        return static_cast<u8>(buf_.cmp1 & 0xFFU);
    }
    if (address == desc_.cmp1buf_address + 1) return temp_;

    if (address == desc_.cmp2buf_address) {
        temp_ = static_cast<u8>((buf_.cmp2 >> 8U) & 0xFFU);
        return static_cast<u8>(buf_.cmp2 & 0xFFU);
    }
    if (address == desc_.cmp2buf_address + 1) return temp_;

    return 0x00;
}


void Tca::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value;
        update_prescaler();
        update_interrupt_state();
        update_active_state();
    } else if (address == desc_.ctrlb_address) {
        ctrlb_ = value;
    } else if (address == desc_.ctrlc_address) {
        ctrlc_ = value;
    } else if (address == desc_.ctrld_address) {
        ctrld_ = value;
        update_interrupt_state();
    } else if (address == desc_.evctrl_address) {
        evctrl_ = value;
    } else if (address == desc_.intctrl_address) {
        intctrl_ = value;
        update_interrupt_state();
    } else if (address == desc_.intflags_address) {
        intflags_ &= static_cast<u8>(~value); // Clear on write 1
        update_interrupt_state();
    } else if (address == desc_.temp_address) {
        temp_ = value;
    } else if (address == desc_.ctrleclr_address || address == desc_.ctrleset_address) {
        // Bit layout (ATDF NVMCTRL-independent: TCA CTRLE, datasheet
        // 21.5.5 normal / 21.7.5 split): CMD = bits 3:2. Normal mode has
        // DIR = bit 0 and LUPD = bit 1, split mode has CMDEN = bits 1:0.
        const bool set = address == desc_.ctrleset_address;
        const u8 cmd = static_cast<u8>((value & 0x0CU) >> 2U);
        if (is_split_mode()) {
            const u8 cmden = value & 0x03U;
            if (cmden == 0x03U) handle_split_cmd(cmd); // CMDEN = BOTH
        } else {
            if (value & 0x01U) {
                if (set) ctrle_ |= 0x01U; else ctrle_ &= 0xFEU; // DIR
            }
            if (value & 0x02U) {
                if (set) ctrle_ |= 0x02U; else ctrle_ &= 0xFDU; // LUPD
            }
            handle_cmd(cmd);
        }
    } else if (address == desc_.ctrlfclr_address) {
        ctrlf_ &= static_cast<u8>(~value);
    } else if (address == desc_.ctrlfset_address) {
        ctrlf_ |= value;
    }
    // Data Registers
    else if (is_split_mode()) {
        if (address == desc_.tcnt_address) cnt_l() = value;
        else if (address == desc_.tcnt_address + 1) cnt_h() = value;
        else if (address == desc_.period_address) per_l() = value;
        else if (address == desc_.period_address + 1) per_h() = value;
        else if (address == desc_.cmp0_address) cmp0_l() = value;
        else if (address == desc_.cmp0_address + 1) cmp0_h() = value;
        else if (address == desc_.cmp1_address) cmp1_l() = value;
        else if (address == desc_.cmp1_address + 1) cmp1_h() = value;
        else if (address == desc_.cmp2_address) cmp2_l() = value;
        else if (address == desc_.cmp2_address + 1) cmp2_h() = value;
        // Split mode: BUF addresses map to H-timer registers (HPER, HCMP0-2)
        else if (address == desc_.perbuf_address) { per_h() = (per_h() & 0xF0U) | (value & 0x0FU); }
        else if (address == desc_.cmp0buf_address) { cmp0_h() = value; }
        else if (address == desc_.cmp1buf_address) { cmp1_h() = value; }
        else if (address == desc_.cmp2buf_address) { cmp2_h() = value; }
    } else {
        // Normal (16-bit) mode with TEMP logic
        // GCC writes low byte first, then high byte
        if (address == desc_.tcnt_address) temp_ = value;
        else if (address == desc_.tcnt_address + 1) norm_.tcnt = (static_cast<u16>(value) << 8U) | temp_;
        else if (address == desc_.period_address) temp_ = value;
        else if (address == desc_.period_address + 1) norm_.per = (static_cast<u16>(value) << 8U) | temp_;
        else if (address == desc_.cmp0_address) temp_ = value;
        else if (address == desc_.cmp0_address + 1) { norm_.cmp0 = (static_cast<u16>(value) << 8U) | temp_; }
        else if (address == desc_.cmp1_address) temp_ = value;
        else if (address == desc_.cmp1_address + 1) norm_.cmp1 = (static_cast<u16>(value) << 8U) | temp_;
        else if (address == desc_.cmp2_address) temp_ = value;
        else if (address == desc_.cmp2_address + 1) norm_.cmp2 = (static_cast<u16>(value) << 8U) | temp_;
        
        // BUF registers
        else if (address == desc_.perbuf_address) temp_ = value;
        else if (address == desc_.perbuf_address + 1) {
            buf_.per = (static_cast<u16>(value) << 8U) | temp_;
            buf_.per_valid = true;
            ctrlf_ |= 0x01U; // PERBV
        }
        else if (address == desc_.cmp0buf_address) temp_ = value;
        else if (address == desc_.cmp0buf_address + 1) {
            buf_.cmp0 = (static_cast<u16>(value) << 8U) | temp_;
            buf_.cmp0_valid = true;
            ctrlf_ |= 0x02U; // CMP0BV
        }
        else if (address == desc_.cmp1buf_address) temp_ = value;
        else if (address == desc_.cmp1buf_address + 1) {
            buf_.cmp1 = (static_cast<u16>(value) << 8U) | temp_;
            buf_.cmp1_valid = true;
            ctrlf_ |= 0x04U; // CMP1BV
        }
        else if (address == desc_.cmp2buf_address) temp_ = value;
        else if (address == desc_.cmp2buf_address + 1) {
            buf_.cmp2 = (static_cast<u16>(value) << 8U) | temp_;
            buf_.cmp2_valid = true;
            ctrlf_ |= 0x08U; // CMP2BV
        }
    }
    update_outputs();
}

void Tca::tick(u64 elapsed_cycles) noexcept {
    if (elapsed_cycles == 0) return;
    if (!is_enabled()) return;
    
    // CNT_POSEDGE / CNT_ANYEDGE: the events ARE the clock, driven from
    // on_event(); the prescaler path must not run. In Split mode the event
    // inputs are not used at all, so only ignore this in Normal mode.
    const u8 action = evact_a();
    if (!is_split_mode() && (evctrl_ & 0x01U) != 0U &&
        (action == 0U || action == 1U)) return;
    // CNT_HIGHLVL: count prescaled cycles only while the event is high.
    if (!is_split_mode() && (evctrl_ & 0x01U) != 0U && action == 2U &&
        !last_event_level_a_) return;
    
    bool ticked = false;
    for (u64 i = 0; i < elapsed_cycles; ++i) {
        if (++prescaler_counter_ >= prescaler_limit_) {
            prescaler_counter_ = 0;
            if (is_split_mode()) {
                perform_tick_split();
            } else {
                perform_tick();
            }
            ticked = true;
        }
    }
    if (ticked) {
        update_interrupt_state();
    }
}

void Tca::handle_matches() {
    if (is_split_mode()) {
        const u8 l_cnt = cnt_l();
        // L-Matches
        if (l_cnt == cmp0_l()) {
             intflags_ |= 0x10; // LCMP0
             if (evsys_ && desc_.cmp0_generator_id != 0) evsys_->trigger_event(desc_.cmp0_generator_id);
        }
        if (l_cnt == cmp1_l()) {
             intflags_ |= 0x20; // LCMP1
             if (evsys_ && desc_.cmp1_generator_id != 0) evsys_->trigger_event(desc_.cmp1_generator_id);
        }
        if (l_cnt == cmp2_l()) {
             intflags_ |= 0x40; // LCMP2
             if (evsys_ && desc_.cmp2_generator_id != 0) evsys_->trigger_event(desc_.cmp2_generator_id);
        }
        
        // HCMPn have NO interrupt flags in split mode (INTFLAGS bit 7 is
        // reserved; INTFLAGS = LCMP2/LCMP1/LCMP0/HUNF/LUNF). A match only
        // drives WO[n+3] via get_wo_level(), which is level-based.
        return;
    }

    // Compare matches
    if (norm_.tcnt == norm_.cmp0) {
        intflags_ |= 0x10;
        if (evsys_ && desc_.cmp0_generator_id != 0) evsys_->trigger_event(desc_.cmp0_generator_id);
    }
    if (norm_.tcnt == norm_.cmp1) {
        intflags_ |= 0x20;
        if (evsys_ && desc_.cmp1_generator_id != 0) evsys_->trigger_event(desc_.cmp1_generator_id);
    }
    if (norm_.tcnt == norm_.cmp2) {
        intflags_ |= 0x40;
        if (evsys_ && desc_.cmp2_generator_id != 0) evsys_->trigger_event(desc_.cmp2_generator_id);
    }
}

bool Tca::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (!is_enabled()) return false;
    
    if (intflags_ & intctrl_ & 0x01) {
        request.vector_index = desc_.luf_ovf_vector_index;
        return true;
    }
    
    if (is_split_mode()) {
        if (intflags_ & intctrl_ & 0x02) { // HUNF
            request.vector_index = desc_.hunf_vector_index;
            return true;
        }
    }

    if (intflags_ & intctrl_ & 0x10) {
        request.vector_index = is_split_mode() ? desc_.lcmp0_vector_index : desc_.cmp0_vector_index;
        return true;
    }
    if (intflags_ & intctrl_ & 0x20) {
        request.vector_index = is_split_mode() ? desc_.lcmp1_vector_index : desc_.cmp1_vector_index;
        return true;
    }
    if (intflags_ & intctrl_ & 0x40) {
        request.vector_index = is_split_mode() ? desc_.lcmp2_vector_index : desc_.cmp2_vector_index;
        return true;
    }
    return false;
}

bool Tca::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (pending_interrupt_request(request)) {
        if (request.vector_index == desc_.luf_ovf_vector_index) {
            intflags_ &= 0xFEU;
        } else if (request.vector_index == desc_.hunf_vector_index) {
            intflags_ &= 0xFDU;
        } else if (request.vector_index == desc_.cmp0_vector_index ||
                   request.vector_index == desc_.lcmp0_vector_index) {
            intflags_ &= 0xEFU;
        } else if (request.vector_index == desc_.cmp1_vector_index ||
                   request.vector_index == desc_.lcmp1_vector_index) {
            intflags_ &= 0xDFU;
        } else if (request.vector_index == desc_.cmp2_vector_index ||
                   request.vector_index == desc_.lcmp2_vector_index) {
            intflags_ &= 0xBFU;
        }
        update_interrupt_state();
        return true;
    }
    return false;
}

void Tca::handle_split_cmd(u8 cmd) noexcept {
    // Split mode (datasheet 21.7.5): CMDEN = BOTH, so RESTART/RESET apply to
    // both 8-bit timers. RESET is ignored while the timer is enabled, and
    // split mode has no double buffering.
    switch (cmd) {
    case 2: // RESTART
        cnt_l() = 0;
        cnt_h() = 0;
        intflags_ = 0;
        update_interrupt_state();
        update_outputs();
        break;
    case 3: // RESET
        if (!is_enabled()) reset();
        break;
    default: // NONE / reserved
        break;
    }
}

void Tca::handle_cmd(u8 cmd) noexcept {
    // Datasheet 21.5.5: CMD = bits 3:2; 0=NONE, 1=UPDATE, 2=RESTART,
    // 3=RESET (ignored while the timer is enabled). LUPD = bit 1.
    auto apply_buffers = [&] {
        if (buf_.per_valid) { norm_.per = buf_.per; buf_.per_valid = false; ctrlf_ &= 0xFEU; }
        if (buf_.cmp0_valid) { norm_.cmp0 = buf_.cmp0; buf_.cmp0_valid = false; ctrlf_ &= 0xFDU; }
        if (buf_.cmp1_valid) { norm_.cmp1 = buf_.cmp1; buf_.cmp1_valid = false; ctrlf_ &= 0xFBU; }
        if (buf_.cmp2_valid) { norm_.cmp2 = buf_.cmp2; buf_.cmp2_valid = false; ctrlf_ &= 0xF7U; }
    };
    const bool lupd = (ctrle_ & 0x02U) != 0U;

    switch (cmd) {
    case 1: // UPDATE
        if (!lupd) apply_buffers();
        break;
    case 2: // RESTART
        norm_.tcnt = 0;
        counting_up_ = true;
        intflags_ = 0;
        if (!lupd) apply_buffers();
        update_interrupt_state();
        update_outputs();
        break;
    case 3: // RESET (ignored when enabled)
        if (!is_enabled()) reset();
        break;
    default: // NONE / reserved
        break;
    }
}

void Tca::perform_tick() {
    u8 wgmode = ctrlb_ & 0x07;
    bool update_cond = false;
    bool ovf_at_bottom = false;

    if (wgmode == 0x01) { // FRQ
        if (norm_.tcnt >= norm_.cmp0) {
            norm_.tcnt = 0;
            update_cond = true;
            wo_states_[0] = !wo_states_[0];
        } else {
            norm_.tcnt++;
        }
    } else if (wgmode >= 0x05) { // DUALSLOPE
        if (counting_up_) {
            if (norm_.tcnt >= norm_.per) {
                counting_up_ = false;
                update_cond = true; // UPDATE at TOP in Dual Slope
                if (norm_.tcnt > 0) norm_.tcnt--;
            } else {
                norm_.tcnt++;
            }
        } else {
            if (norm_.tcnt == 0) {
                counting_up_ = true;
                ovf_at_bottom = true;
                update_cond = true; // UPDATE at BOTTOM for dual slope
                norm_.tcnt++;
            } else {
                norm_.tcnt--;
            }
        }
    } else { // Normal / Single Slope
        // EVACTA/EVACTB = UPDOWN: the event level(s) select the count
        // direction, counting down while an enabled event input is high
        // (DS40002036 20.5.9, DS40002233A 21.5.9).
        const bool updown = !is_split_mode() &&
                            (((evctrl_ & 0x01U) != 0U && evact_a() == 3U) ||
                             ((evctrl_ & 0x10U) != 0U && evact_b() == 3U));
        if (updown && !counting_up_) {
            if (norm_.tcnt == 0U) {
                norm_.tcnt = norm_.per;
                update_cond = true;
                ovf_at_bottom = true;
            } else {
                norm_.tcnt--;
            }
        } else if (norm_.tcnt >= norm_.per) {
            norm_.tcnt = 0;
            update_cond = true;
            ovf_at_bottom = true;
        } else {
            norm_.tcnt++;
        }
    }

    handle_matches();

    if (update_cond) {
        if (ovf_at_bottom) {
            intflags_ |= 0x01; // OVF
            if (evsys_ && desc_.ovf_generator_id != 0) evsys_->trigger_event(desc_.ovf_generator_id);
        }

        if (buf_.per_valid) { norm_.per = buf_.per; buf_.per_valid = false; ctrlf_ &= 0xFEU; }
        if (buf_.cmp0_valid) { norm_.cmp0 = buf_.cmp0; buf_.cmp0_valid = false; ctrlf_ &= 0xFDU; }
        if (buf_.cmp1_valid) { norm_.cmp1 = buf_.cmp1; buf_.cmp1_valid = false; ctrlf_ &= 0xFBU; }
        if (buf_.cmp2_valid) { norm_.cmp2 = buf_.cmp2; buf_.cmp2_valid = false; ctrlf_ &= 0xF7U; }
    }

    update_outputs();
}

void Tca::perform_tick_split() {
    // Low byte
    bool update_l = false;
    if (cnt_l() >= per_l()) {
        cnt_l() = 0;
        intflags_ |= 0x01; // LUNF/OVF
        update_l = true;
    } else {
        cnt_l()++;
    }

    if (update_l) {
        if (buf_.per_valid) { per_l() = static_cast<u8>(buf_.per & 0xFFU); buf_.per_valid = false; ctrlf_ &= 0xFEU; }
        if (buf_.cmp0_valid) { cmp0_l() = static_cast<u8>(buf_.cmp0 & 0xFFU); buf_.cmp0_valid = false; ctrlf_ &= 0xFDU; }
        if (buf_.cmp1_valid) { cmp1_l() = static_cast<u8>(buf_.cmp1 & 0xFFU); buf_.cmp1_valid = false; ctrlf_ &= 0xFBU; }
        if (buf_.cmp2_valid) { cmp2_l() = static_cast<u8>(buf_.cmp2 & 0xFFU); buf_.cmp2_valid = false; }
    }

    // High byte (completely independent in split mode)
    if (cnt_h() >= per_h()) {
        cnt_h() = 0;
        intflags_ |= 0x02; // HUNF is bit 1
    } else {
        cnt_h()++;
    }

    handle_matches();
    update_outputs();
}

void Tca::update_prescaler() noexcept {
    u8 clksel = (ctrla_ >> 1) & 0x07;
    static constexpr u16 div[] = {1, 2, 4, 8, 16, 64, 256, 1024};
    prescaler_limit_ = div[clksel];
    prescaler_counter_ = 0;
}

bool Tca::get_wo_level(u8 index) const noexcept {
    if (!is_enabled() || index >= 6) return false;

    // CTRLC override bits take priority over compare result
    if (index < 3 && (ctrlc_ & (1U << index))) return true;            // LCMPxOV
    if (is_split_mode() && index >= 3 && (ctrlc_ & (1U << (index + 1)))) return true; // HCMPxOV

    if (is_split_mode()) {
        if (index == 0) return cnt_l() < cmp0_l();
        if (index == 1) return cnt_l() < cmp1_l();
        if (index == 2) return cnt_l() < cmp2_l();
        if (index == 3) return cnt_h() < cmp0_h();
        if (index == 4) return cnt_h() < cmp1_h();
        if (index == 5) return cnt_h() < cmp2_h();
    } else {
        u8 wgmode = ctrlb_ & 0x07;
        if (wgmode == 0x01) {
            // FRQ mode: waveform output exists only on WO0 (TOP = CMP0);
            // WO1/WO2 carry no waveform (CTRLC override above still applies).
            if (index == 0) return wo_states_[0];
            return false;
        }
        if (index == 0) return norm_.tcnt < norm_.cmp0;
        if (index == 1) return norm_.tcnt < norm_.cmp1;
        if (index == 2) return norm_.tcnt < norm_.cmp2;
    }
    return false;
}
void Tca::on_routing_changed() noexcept {
    update_outputs();
}

void Tca::update_outputs() {
    if (!port_mux_) return;
    if (is_split_mode()) {
        for (u8 i = 0; i < 3; ++i) {
            bool en_l = ctrlb_ & (1U << i);            // LCMPxEN (CTRLB bits 0-2)
            port_mux_->drive_tca0_wo(i, get_wo_level(i), en_l);
            bool en_h = ctrlb_ & (1U << (4 + i));      // HCMPxEN (CTRLB bits 4-6)
            port_mux_->drive_tca0_wo(u8(3 + i), get_wo_level(u8(3 + i)), en_h);
        }
    } else {
        const bool frq = (ctrlb_ & 0x07) == 0x01; // FRQ drives WO0 only
        for (u8 i = 0; i < 3; ++i) {
            bool en = ctrlb_ & (1U << (4 + i));        // CMPxEN (CTRLB bits 4-6)
            port_mux_->drive_tca0_wo(i, get_wo_level(i), en && (!frq || i == 0));
        }
        for (u8 i = 3; i < 6; ++i) port_mux_->drive_tca0_wo(i, false, false);
    }
}

void Tca::update_interrupt_state() noexcept {
    bool pending = false;
    if (is_enabled()) {
        if (intflags_ & intctrl_ & 0x01) pending = true;
        else if (is_split_mode() && (intflags_ & intctrl_ & 0x02)) pending = true;
        else if (intflags_ & intctrl_ & 0x10) pending = true;
        else if (intflags_ & intctrl_ & 0x20) pending = true;
        else if (intflags_ & intctrl_ & 0x40) pending = true;
    }
    set_interrupt_pending(pending);
}

void Tca::update_active_state() noexcept
{
    if (bus_) {
        bool active = is_enabled();
        bus_->set_peripheral_active(this, active);
    }
}

void Tca::on_power_state_change() noexcept
{
    update_active_state();
}

} // namespace vioavr::core
