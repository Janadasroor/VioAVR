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
                // Some ACs might use events to trigger comparison or something
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

void Ac8x::tick(u64 elapsed_cycles) noexcept {
    (void)elapsed_cycles;
    if (!is_enabled()) return;

    double p_volts = 0.5;
    double n_volts = 0.0;

    if (signal_bank_) {
        u8 p_mux = (muxctrla_ >> 3) & 0x07U;
        u8 n_mux = muxctrla_ & 0x07U;

        // Simplified mapping for now
        p_volts = signal_bank_->voltage(p_mux);
        
        if (n_mux == 0x03U) { // DACREF
            n_volts = static_cast<double>(dacctrla_) / 255.0;
        } else {
            n_volts = signal_bank_->voltage(n_mux + 4); // Dummy offset for negative pins
        }
    }
    
    bool old_state = (status_ & 0x10U) != 0;
    bool new_state = (p_volts > n_volts);

    if (new_state) status_ |= 0x10U;
    else status_ &= ~0x10U;

    if (old_state != new_state) {
        // Check for interrupt trigger
        u8 mode = (ctrla_ >> 4) & 0x03U;
        bool trigger = false;
        switch (mode) {
            case 0: trigger = true; break; // BOTHEDGE
            case 2: trigger = !new_state; break; // NEGEDGE
            case 3: trigger = new_state; break; // POSEDGE
            default: break;
        }

        if (trigger) {
            status_ |= 0x01U; // CMP flag
        }

        if (evsys_ && desc_.out_generator_id != 0) {
            evsys_->trigger_event(desc_.out_generator_id, new_state);
        }
    }
}

} // namespace vioavr::core
