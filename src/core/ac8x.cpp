#include "vioavr/core/ac8x.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/evsys.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/logger.hpp"

namespace vioavr::core {

Ac8x::Ac8x(std::string name, const Ac8xDescriptor& desc) noexcept 
    : name_(std::move(name)), desc_(desc) {
    ranges_[0] = {desc_.ctrla_address, static_cast<u16>(desc_.status_address)};
}
 
void Ac8x::set_event_system(EventSystem* evsys) noexcept {
    evsys_ = evsys;
    if (evsys_ && desc_.user_event_address != 0) {
        u16 user_base = evsys_->users_base();
        if (desc_.user_event_address >= user_base) {
            u8 user_index = static_cast<u8>(desc_.user_event_address - user_base);
            evsys_->register_user_callback(user_index, [this](bool) {
                if (is_enabled() && !(ctrla_ & 0x08U)) {
                    evaluate();
                }
            });
        }
    }
}

bool Ac8x::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if ((intctrl_ & 0x01U) && (status_ & 0x01U)) { // CMPIE and CMP flag
        request.vector_index = desc_.vector_index;
        return true;
    }
    return false;
}

bool Ac8x::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (pending_interrupt_request(request)) {
        status_ &= ~0x01U; // H7: Clear CMP flag on consume
        update_interrupt_state();
        return true;
    }
    return false;
}

void Ac8x::reset() noexcept {
    ctrla_ = 0;
    muxctrla_ = 0;
    dacctrla_ = 0xFF; // Specific initval for DACREF
    intctrl_ = 0;
    status_ = 0;
    pending_state_ = false;
    settle_counter_ = 0;
    vref_ = vdd_;
}

u8 Ac8x::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.muxctrla_address) return muxctrla_;
    if (address == desc_.dacctrla_address) return dacctrla_;
    if (address == desc_.intctrl_address) return intctrl_;
    if (address == desc_.status_address) return status_;
    return 0;
}

void Ac8x::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value;
    } else if (address == desc_.muxctrla_address) {
        muxctrla_ = value;
    } else if (address == desc_.dacctrla_address) {
        dacctrla_ = value;
    } else if (address == desc_.intctrl_address) {
        intctrl_ = value;
    } else if (address == desc_.status_address) {
        // Clear CMP flag on write 1
        if (value & 0x01U) {
            status_ &= ~0x01U;
        }
    }
}

void Ac8x::evaluate() noexcept {
    double p_volts = 0.0;
    double n_volts = 0.0;

    if (signal_bank_) {
        u8 p_mux = (muxctrla_ >> 3) & 0x07U;
        u8 n_mux = muxctrla_ & 0x03U;
        p_volts = signal_bank_->voltage(desc_.muxpos_base + p_mux);
        if (n_mux == 0x03U) {
            n_volts = static_cast<double>(dacctrla_) * vref_ / 256.0;
        } else {
            n_volts = signal_bank_->voltage(desc_.muxneg_base + n_mux);
        }
    }
    
    bool old_state = (status_ & 0x10U) != 0;
    bool new_state = (p_volts > n_volts);
    if (muxctrla_ & 0x80U) new_state = !new_state;

    u8 hys = (ctrla_ >> 1) & 0x03U;
    if (hys > 0) {
        double hyst_v = 0.0;
        if (hys == 1) hyst_v = 0.010;
        else if (hys == 2) hyst_v = 0.025;
        else hyst_v = 0.050;
        if (old_state) {
            new_state = (p_volts > n_volts - hyst_v);
            if (muxctrla_ & 0x80U) new_state = !new_state;
        } else {
            new_state = (p_volts > n_volts + hyst_v);
            if (muxctrla_ & 0x80U) new_state = !new_state;
        }
    }
    pending_state_ = new_state;
}

void Ac8x::tick(u64 elapsed_cycles) noexcept {
    (void)elapsed_cycles;
    if (!is_enabled()) return;
    if (ctrla_ & 0x08U) return;

    evaluate();

    bool old_state = (status_ & 0x10U) != 0;
    if (pending_state_ == old_state) {
        settle_counter_ = 0;
        return;
    }

    if (settle_counter_ > 0) {
        settle_counter_--;
        if (settle_counter_ > 0) return;
    } else {
        settle_counter_ = PROPAGATION_DELAY;
        if (settle_counter_ > 0) return;
    }

    if (pending_state_) status_ |= 0x10U;
    else status_ &= ~0x10U;

    u8 mode = (ctrla_ >> 4) & 0x03U;
    bool trigger = false;
    switch (mode) {
        case 0: trigger = true; break;
        case 2: trigger = !pending_state_; break;
        case 3: trigger = pending_state_; break;
        default: break;
    }
    if (trigger) status_ |= 0x01U;

    if (evsys_ && desc_.out_generator_id != 0 && (ctrla_ & 0x40U)) {
        evsys_->trigger_event(desc_.out_generator_id, pending_state_);
    }
}

void Ac8x::update_interrupt_state() noexcept {
    InterruptRequest req;
    set_interrupt_pending(pending_interrupt_request(req));
}

} // namespace vioavr::core
