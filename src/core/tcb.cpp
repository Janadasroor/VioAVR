#include "vioavr/core/tcb.hpp"
#include "vioavr/core/port_mux.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/evsys.hpp"
#include <algorithm>

namespace vioavr::core {

Tcb::Tcb(std::string name, const TcbDescriptor& desc) 
    : name_(std::move(name)), desc_(desc) {
    if (name_.size() > 3 && std::isdigit(name_[3])) {
        index_ = static_cast<u8>(name_[3] - '0');
    }
    if (desc_.ctrla_address != 0U) {
        std::vector<u16> addrs = {
            desc_.ctrla_address, desc_.ctrlb_address, desc_.evctrl_address,
            desc_.intctrl_address, desc_.intflags_address, desc_.status_address,
            desc_.dbgctrl_address, desc_.temp_address,
            desc_.cnt_address, static_cast<u16>(desc_.cnt_address + 1),
            desc_.ccmp_address, static_cast<u16>(desc_.ccmp_address + 1)
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

void Tcb::reset() noexcept {
    ctrla_ = 0;
    ctrlb_ = 0;
    evctrl_ = 0;
    intctrl_ = 0;
    intflags_ = 0;
    status_ = 0;
    dbgctrl_ = 0;
    temp_ = 0;
    cnt_ = 0;
    ccmp_ = 0;
}

std::span<const AddressRange> Tcb::mapped_ranges() const noexcept {
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

u8 Tcb::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.ctrlb_address) return ctrlb_;
    if (address == desc_.evctrl_address) return evctrl_;
    if (address == desc_.intctrl_address) return intctrl_;
    if (address == desc_.intflags_address) return intflags_;
    if (address == desc_.status_address) return status_;
    if (address == desc_.dbgctrl_address) return dbgctrl_;
    if (address == desc_.temp_address) return temp_;

    // 16-bit registers: Low byte first latches High byte into TEMP
    if (address == desc_.cnt_address) {
        temp_ = static_cast<u8>((cnt_ >> 8U) & 0xFFU);
        return static_cast<u8>(cnt_ & 0xFFU);
    }
    if (address == desc_.cnt_address + 1) return temp_;
    
    if (address == desc_.ccmp_address) {
        temp_ = static_cast<u8>((ccmp_ >> 8U) & 0xFFU);
        return static_cast<u8>(ccmp_ & 0xFFU);
    }
    if (address == desc_.ccmp_address + 1) return temp_;

    return 0x00;
}


void Tcb::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value;
    } else if (address == desc_.ctrlb_address) {
        ctrlb_ = value;
    } else if (address == desc_.evctrl_address) {
        evctrl_ = value;
    } else if (address == desc_.intctrl_address) {
        intctrl_ = value;
    } else if (address == desc_.intflags_address) {
        intflags_ &= ~value;
    } else if (address == desc_.temp_address) {
        temp_ = value;
    } else if (address == desc_.cnt_address + 1) {
        temp_ = value;
    } else if (address == desc_.cnt_address) {
        cnt_ = (static_cast<u16>(temp_) << 8U) | value;
    } else if (address == desc_.ccmp_address + 1) {
        temp_ = value;
    } else if (address == desc_.ccmp_address) {
        ccmp_ = (static_cast<u16>(temp_) << 8U) | value;
    }
    update_outputs();
}


void Tcb::tick(u64 elapsed_cycles) noexcept {
    if (!is_enabled()) return;
    
    u8 clksel = get_clksel();

    if (clksel == 0) { // CLK_PER
        for (u64 i = 0; i < elapsed_cycles; ++i) {
            perform_tick(true);
        }
    } else if (clksel == 1) { // CLK_PER / 2
        for (u64 i = 0; i < elapsed_cycles; ++i) {
            bool fire = false;
            if (++prescaler_counter_ >= 2) {
                prescaler_counter_ = 0;
                fire = true;
            }
            perform_tick(fire);
        }
    } else {
        for (u64 i = 0; i < elapsed_cycles; ++i) {
            perform_tick(false);
        }
    }
    update_outputs();
}

void Tcb::handle_matches() {
    u8 mode = get_mode();
    if (mode == 0 || mode == 1) { // Periodic or 8-bit PWM
        // Handled in perform_tick for precise reset
    }
}

void Tcb::perform_tick(bool clock_event) {
    if (!clock_event) return;

    u8 mode = get_mode();
    
    // Periodic Interrupt / PWM
    if (mode == 0) { // Periodic Interrupt
        if (cnt_ >= ccmp_) {
            cnt_ = 0;
            intflags_ |= 0x01; // CAPT/OVF
        } else {
            cnt_++;
        }
    } else if (mode == 1) { // 8-bit PWM
        // Low byte counts to CCMP-L, High byte is Compare Value
        u8 per = static_cast<u8>(ccmp_ & 0xFFU);
        u8 cnt_l = static_cast<u8>(cnt_ & 0xFFU);
        if (cnt_l >= per) {
            cnt_l = 0;
            intflags_ |= 0x01;
        } else {
            cnt_l++;
        }
        cnt_ = (cnt_ & 0xFF00U) | cnt_l;
    } else if (mode == 6) { // Single-shot
        if (cnt_ >= ccmp_) {
            cnt_ = 0; // Reset on match
            ctrla_ &= ~0x01; // Disable
            intflags_ |= 0x01;
        } else {
            cnt_++;
        }
    } else {
        // Measurement Modes: counter free-runs
        cnt_++;
        if (cnt_ == 0) { // Overflow
            // Some TCB implementations set a flag on overflow in measurement modes
            // In 4809, it can trigger an interrupt if enabled?
            // Actually, in measurement modes, overflow often sets the CAPT flag too
            // or just wraps around.
        }
    }
}

void Tcb::set_event_system(EventSystem* evsys) noexcept {
    evsys_ = evsys;
    if (evsys_) {
        // 1. Capture Trigger
        if (desc_.user_event_address != 0) {
            u8 user_index = static_cast<u8>(desc_.user_event_address - evsys_->users_base());
            evsys_->register_user_callback(user_index, [this]() { on_event(); });
        }
        
        // 2. Cascaded Clock (Listen to TCA0 OVF - Generator 128)
        // Note: Generator ID 128 is a common constant for TCA0.OVF in AVR8X.
        // We should ideally use desc_.tca_ovf_generator_id if added to descriptor.
        // For now, assume 128 as per most ATDFs for TCA0.
        evsys_->register_generator_callback(128, [this]() {
            if (is_enabled() && get_clksel() == 2) {
                perform_tick(true);
            }
        });
    }
}

void Tcb::on_event() noexcept {
    if (!is_enabled()) return;
    if (!(evctrl_ & 0x01)) return; // CAPTEI

    u8 mode = get_mode();
    
    if (mode == 2) { // Input Capture
        ccmp_ = cnt_;
        intflags_ |= 0x01; // CAPT
    } else if (mode == 3) { // Frequency Measurement
        ccmp_ = cnt_;
        cnt_ = 0;
        intflags_ |= 0x01; // CAPT
    } else if (mode == 4 || mode == 5) { // Pulse Width or Frequency/Pulse Width
        // This requires knowing if it's the first or second edge.
        // Bit 0 of STATUS usually tracks the input signal level or capture state.
        if (!(status_ & 0x01)) { // First edge
            cnt_ = 0;
            status_ |= 0x01; // Mark as "Running"
        } else { // Second edge
            ccmp_ = cnt_;
            status_ &= ~0x01;
            intflags_ |= 0x01; // CAPT
        }
    }
}

bool Tcb::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if ((intflags_ & 0x01) && (intctrl_ & 0x01)) {
        request.vector_index = desc_.vector_index;
        return true;
    }
    return false;
}

bool Tcb::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (pending_interrupt_request(request)) {
        intflags_ &= ~0x01;
        return true;
    }
    return false;
}

bool Tcb::get_wo_level() const noexcept {
    if (!is_enabled()) return false;
    u8 mode = get_mode();
    if (mode == 0) return cnt_ < ccmp_; // Simple comparison
    if (mode == 1) return (cnt_ & 0xFF) < (ccmp_ & 0xFF); // 8-bit PWM
    if (mode == 2) return cnt_ < ccmp_; // Single shot
    return false;
}

void Tcb::on_routing_changed() noexcept {
    update_outputs();
}

void Tcb::update_outputs() noexcept {
    if (!port_mux_) return;
    bool en = ctrlb_ & 0x10; // CCEN
    port_mux_->drive_tcb_wo(index_, get_wo_level(), en);
}
} // namespace vioavr::core
