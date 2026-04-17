#include "vioavr/core/timer10.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/memory_bus.hpp"
#include <iostream>

namespace vioavr::core {

Timer10::Timer10(std::string_view name, const Timer10Descriptor& desc) noexcept
    : name_(name), desc_(desc)
{
    u8 count = 0;
    if (desc.tcnt_address) ranges_[count++] = {desc.tcnt_address, desc.tcnt_address};
    if (desc.tc4h_address) ranges_[count++] = {desc.tc4h_address, desc.tc4h_address};
    if (desc.ocra_address) ranges_[count++] = {desc.ocra_address, desc.ocra_address};
    if (desc.ocrb_address) ranges_[count++] = {desc.ocrb_address, desc.ocrb_address};
    if (desc.ocrc_address) ranges_[count++] = {desc.ocrc_address, desc.ocrc_address};
    if (desc.ocrd_address) ranges_[count++] = {desc.ocrd_address, desc.ocrd_address};
    if (desc.tccra_address) ranges_[count++] = {desc.tccra_address, desc.tccra_address};
    if (desc.tccrb_address) ranges_[count++] = {desc.tccrb_address, desc.tccrb_address};
    if (desc.tccrc_address) ranges_[count++] = {desc.tccrc_address, desc.tccrc_address};
    if (desc.tccrd_address) ranges_[count++] = {desc.tccrd_address, desc.tccrd_address};
    if (desc.tccre_address) ranges_[count++] = {desc.tccre_address, desc.tccre_address};
    if (desc.dt4_address) ranges_[count++] = {desc.dt4_address, desc.dt4_address};
    if (desc.timsk_address) ranges_[count++] = {desc.timsk_address, desc.timsk_address};
    if (desc.tifr_address) ranges_[count++] = {desc.tifr_address, desc.tifr_address};
    ranges_count_ = count;
}

std::string_view Timer10::name() const noexcept { return name_; }

std::span<const AddressRange> Timer10::mapped_ranges() const noexcept {
    return {ranges_.data(), ranges_count_};
}

void Timer10::reset() noexcept {
    tcnt_ = 0;
    tc4h_ = 0;
    tccra_ = desc_.tccra_reset;
    tccrb_ = desc_.tccrb_reset;
    tccrc_ = desc_.tccrc_reset;
    tccrd_ = desc_.tccrd_reset;
    tccre_ = desc_.tccre_reset;
    ocra_ = ocrb_ = ocrc_ = ocrd_ = 0;
    ocra_buf_ = ocrb_buf_ = ocrc_buf_ = ocrd_buf_ = 0;
    dt4_ = 0;
    timsk_ = 0;
    tifr_ = 0;
    dt_a_ = 0; dt_a_neg_ = 0;
    dt_b_ = 0; dt_b_neg_ = 0;
    dt_d_ = 0; dt_d_neg_ = 0;
    target_a_ = false; target_a_neg_ = false;
    target_b_ = false; target_b_neg_ = false;
    target_d_ = false; target_d_neg_ = false;
    clock_ratio_ = 1;
}

void Timer10::tick(u64 elapsed_cycles) noexcept {
    if (power_reduction_enabled()) return;
    
    if (bus_ && desc_.pllcsr_address) {
        u8 pllcsr = bus_->read_data(desc_.pllcsr_address);
        bool pcke = (pllcsr & 0x04U); 
        bool plock = (pllcsr & 0x01U);
        clock_ratio_ = (pcke && plock) ? 8 : 1;
    }

    u64 timer_ticks = elapsed_cycles * clock_ratio_;
    for (u64 i = 0; i < timer_ticks; ++i) {
        perform_tick();
    }
}

u8 Timer10::read(u16 address) noexcept {
    if (address == desc_.tcnt_address) return tcnt_ & 0xFFU;
    if (address == desc_.tc4h_address) return tc4h_;
    if (address == desc_.ocra_address) return ocra_buf_ & 0xFFU;
    if (address == desc_.ocrb_address) return ocrb_buf_ & 0xFFU;
    if (address == desc_.ocrc_address) return ocrc_buf_ & 0xFFU;
    if (address == desc_.ocrd_address) return ocrd_buf_ & 0xFFU;
    if (address == desc_.tccra_address) return tccra_;
    if (address == desc_.tccrb_address) return tccrb_;
    if (address == desc_.tccrc_address) return tccrc_;
    if (address == desc_.tccrd_address) return tccrd_;
    if (address == desc_.tccre_address) return tccre_;
    if (address == desc_.dt4_address) return dt4_;
    if (address == desc_.timsk_address) return timsk_;
    if (address == desc_.tifr_address) return tifr_;
    return 0;
}

void Timer10::write(u16 address, u8 value) noexcept {
    if (address == desc_.tc4h_address) {
        tc4h_ = value & 0x03U;
    } else if (address == desc_.tcnt_address) {
        tcnt_ = (static_cast<u16>(tc4h_) << 8) | value;
    } else if (address == desc_.ocra_address) {
        ocra_buf_ = (static_cast<u16>(tc4h_) << 8) | value;
        if (!(tccra_ & 0x03U)) ocra_ = ocra_buf_;
    } else if (address == desc_.ocrb_address) {
        ocrb_buf_ = (static_cast<u16>(tc4h_) << 8) | value;
        if (!(tccra_ & 0x03U)) ocrb_ = ocrb_buf_;
    } else if (address == desc_.ocrc_address) {
        ocrc_buf_ = (static_cast<u16>(tc4h_) << 8) | value;
        ocrc_ = ocrc_buf_;
    } else if (address == desc_.ocrd_address) {
        ocrd_buf_ = (static_cast<u16>(tc4h_) << 8) | value;
        if (!(tccrd_ & 0x03U)) ocrd_ = ocrd_buf_;
    } else if (address == desc_.tccra_address) {
        tccra_ = value;
    } else if (address == desc_.tccrb_address) {
        tccrb_ = value;
    } else if (address == desc_.tccrc_address) {
        tccrc_ = value;
    } else if (address == desc_.tccrd_address) {
        tccrd_ = value;
    } else if (address == desc_.tccre_address) {
        tccre_ = value;
    } else if (address == desc_.dt4_address) {
        dt4_ = value;
    } else if (address == desc_.timsk_address) {
        timsk_ = value;
    } else if (address == desc_.tifr_address) {
        tifr_ &= ~value;
    }
}

bool Timer10::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if ((tifr_ & 0x40U) && (timsk_ & 0x40U)) { request.vector_index = desc_.compare_a_vector_index; return true; }
    if ((tifr_ & 0x20U) && (timsk_ & 0x20U)) { request.vector_index = desc_.compare_b_vector_index; return true; }
    if ((tifr_ & 0x80U) && (timsk_ & 0x80U)) { request.vector_index = desc_.compare_d_vector_index; return true; }
    if ((tifr_ & 0x04U) && (timsk_ & 0x04U)) { request.vector_index = desc_.overflow_vector_index; return true; }
    return false;
}

bool Timer10::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (request.vector_index == desc_.compare_a_vector_index) { tifr_ &= ~0x40U; return true; }
    if (request.vector_index == desc_.compare_b_vector_index) { tifr_ &= ~0x20U; return true; }
    if (request.vector_index == desc_.compare_d_vector_index) { tifr_ &= ~0x80U; return true; }
    if (request.vector_index == desc_.overflow_vector_index) { tifr_ &= ~0x04U; return true; }
    return false;
}

ClockDomain Timer10::clock_domain() const noexcept { return ClockDomain::cpu; }

void Timer10::connect_compare_output_a(GpioPort& port, u8 bit) noexcept { pin_a_ = {&port, bit}; }
void Timer10::connect_compare_output_a_inverted(GpioPort& port, u8 bit) noexcept { pin_a_neg_ = {&port, bit}; }
void Timer10::connect_compare_output_b(GpioPort& port, u8 bit) noexcept { pin_b_ = {&port, bit}; }
void Timer10::connect_compare_output_b_inverted(GpioPort& port, u8 bit) noexcept { pin_b_neg_ = {&port, bit}; }
void Timer10::connect_compare_output_d(GpioPort& port, u8 bit) noexcept { pin_d_ = {&port, bit}; }
void Timer10::connect_compare_output_d_inverted(GpioPort& port, u8 bit) noexcept { pin_d_neg_ = {&port, bit}; }

bool Timer10::power_reduction_enabled() const noexcept {
    if (bus_ && desc_.pr_address) {
        return (bus_->read_data(desc_.pr_address) & desc_.pr_bit);
    }
    return false;
}

void Timer10::perform_tick() noexcept {
    // Process dead-time counters
    if (dt_a_ > 0 && --dt_a_ == 0) apply_pin_level(pin_a_, true);
    if (dt_a_neg_ > 0 && --dt_a_neg_ == 0) apply_pin_level(pin_a_neg_, true);
    if (dt_b_ > 0 && --dt_b_ == 0) apply_pin_level(pin_b_, true);
    if (dt_b_neg_ > 0 && --dt_b_neg_ == 0) apply_pin_level(pin_b_neg_, true);
    if (dt_d_ > 0 && --dt_d_ == 0) apply_pin_level(pin_d_, true);
    if (dt_d_neg_ > 0 && --dt_d_neg_ == 0) apply_pin_level(pin_d_neg_, true);

    u8 wgm = tccrd_ & 0x03U;
    bool pfc = (wgm == 1);

    if (up_direction_) {
        tcnt_ = (tcnt_ + 1) & 0x3FFU;
    } else {
        tcnt_ = (tcnt_ - 1) & 0x3FFU;
    }

    if (tcnt_ == ocra_) handle_compare_match_a();
    if (tcnt_ == ocrb_) handle_compare_match_b();
    if (tcnt_ == ocrd_) handle_compare_match_d();
    
    if (up_direction_ && tcnt_ >= ocrc_) {
        if (pfc) {
            up_direction_ = false;
        } else {
            handle_overflow();
            tcnt_ = 0;
            ocra_ = ocra_buf_;
            ocrb_ = ocrb_buf_;
            ocrd_ = ocrd_buf_;
        }
    } else if (!up_direction_ && tcnt_ == 0) {
        handle_overflow(); // In PFC, overflow is hit at BOTTOM
        up_direction_ = true;
        ocra_ = ocra_buf_;
        ocrb_ = ocrb_buf_;
        ocrd_ = ocrd_buf_;
    }
}

void Timer10::apply_pin_level(std::optional<BoundPin> pin, bool high) noexcept {
    if (!pin) return;
    u16 addr = pin->port->port_address();
    u8 current = pin->port->read(addr);
    if (high) pin->port->write(addr, current | (1 << pin->bit));
    else pin->port->write(addr, current & ~(1 << pin->bit));
}

void Timer10::handle_compare_match_a() noexcept {
    tifr_ |= 0x40U;
    u8 com = (tccra_ >> 6) & 0x03;
    if (com == 2) { // Non-inverting
        if (up_direction_) {
            apply_pin_level(pin_a_, false);
            u8 delay = (dt4_ >> 4) & 0x0F;
            if (delay == 0) apply_pin_level(pin_a_neg_, true);
            else dt_a_neg_ = delay;
        } else {
            apply_pin_level(pin_a_neg_, false);
            u8 delay = dt4_ & 0x0F;
            if (delay == 0) apply_pin_level(pin_a_, true);
            else dt_a_ = delay;
        }
    } else if (com == 3) { // Inverting
        if (up_direction_) {
            apply_pin_level(pin_a_neg_, false);
            u8 delay = dt4_ & 0x0F;
            if (delay == 0) apply_pin_level(pin_a_, true);
            else dt_a_ = delay;
        } else {
            apply_pin_level(pin_a_, false);
            u8 delay = (dt4_ >> 4) & 0x0F;
            if (delay == 0) apply_pin_level(pin_a_neg_, true);
            else dt_a_neg_ = delay;
        }
    }
}

void Timer10::handle_compare_match_b() noexcept {
    tifr_ |= 0x20U;
    u8 com = (tccra_ >> 4) & 0x03;
    if (com == 2) {
        if (up_direction_) {
            apply_pin_level(pin_b_, false);
            u8 delay = (dt4_ >> 4) & 0x0F;
            if (delay == 0) apply_pin_level(pin_b_neg_, true);
            else dt_b_neg_ = delay;
        } else {
            apply_pin_level(pin_b_neg_, false);
            u8 delay = dt4_ & 0x0F;
            if (delay == 0) apply_pin_level(pin_b_, true);
            else dt_b_ = delay;
        }
    } else if (com == 3) {
        if (up_direction_) {
            apply_pin_level(pin_b_neg_, false);
            u8 delay = dt4_ & 0x0F;
            if (delay == 0) apply_pin_level(pin_b_, true);
            else dt_b_ = delay;
        } else {
            apply_pin_level(pin_b_, false);
            u8 delay = (dt4_ >> 4) & 0x0F;
            if (delay == 0) apply_pin_level(pin_b_neg_, true);
            else dt_b_neg_ = delay;
        }
    }
}

void Timer10::handle_compare_match_d() noexcept {
    tifr_ |= 0x80U;
    u8 com = (tccrd_ >> 4) & 0x03;
    if (com == 2) {
        if (up_direction_) {
            apply_pin_level(pin_d_, false);
            u8 delay = (dt4_ >> 4) & 0x0F;
            if (delay == 0) apply_pin_level(pin_d_neg_, true);
            else dt_d_neg_ = delay;
        } else {
            apply_pin_level(pin_d_neg_, false);
            u8 delay = dt4_ & 0x0F;
            if (delay == 0) apply_pin_level(pin_d_, true);
            else dt_d_ = delay;
        }
    } else if (com == 3) {
        if (up_direction_) {
            apply_pin_level(pin_d_neg_, false);
            u8 delay = dt4_ & 0x0F;
            if (delay == 0) apply_pin_level(pin_d_, true);
            else dt_d_ = delay;
        } else {
            apply_pin_level(pin_d_, false);
            u8 delay = (dt4_ >> 4) & 0x0F;
            if (delay == 0) apply_pin_level(pin_d_neg_, true);
            else dt_d_neg_ = delay;
        }
    }
}

void Timer10::handle_overflow() noexcept {
    tifr_ |= 0x04U;
    
    u8 wgm = tccrd_ & 0x03U;
    if (wgm != 1) { // Not Phase Correct -> Fast PWM pins set at BOTTOM/TOP
        // OC4A Logic
        u8 com_a = (tccra_ >> 6) & 0x03;
        if (com_a == 2) { // Non-inverting
            apply_pin_level(pin_a_neg_, false);
            u8 delay = dt4_ & 0x0F;
            if (delay == 0) apply_pin_level(pin_a_, true);
            else dt_a_ = delay;
        } else if (com_a == 3) { // Inverting
            apply_pin_level(pin_a_, false);
            u8 delay = (dt4_ >> 4) & 0x0F;
            if (delay == 0) apply_pin_level(pin_a_neg_, true);
            else dt_a_neg_ = delay;
        }

        // OC4B Logic
        u8 com_b = (tccra_ >> 4) & 0x03;
        if (com_b == 2) {
            apply_pin_level(pin_b_neg_, false);
            u8 delay = dt4_ & 0x0F;
            if (delay == 0) apply_pin_level(pin_b_, true);
            else dt_b_ = delay;
        } else if (com_b == 3) {
            apply_pin_level(pin_b_, false);
            u8 delay = (dt4_ >> 4) & 0x0F;
            if (delay == 0) apply_pin_level(pin_b_neg_, true);
            else dt_b_neg_ = delay;
        }

        // OC4D Logic
        u8 com_d = (tccrd_ >> 4) & 0x03;
        if (com_d == 2) {
            apply_pin_level(pin_d_neg_, false);
            u8 delay = dt4_ & 0x0F;
            if (delay == 0) apply_pin_level(pin_d_, true);
            else dt_d_ = delay;
        } else if (com_d == 3) {
            apply_pin_level(pin_d_, false);
            u8 delay = (dt4_ >> 4) & 0x0F;
            if (delay == 0) apply_pin_level(pin_d_neg_, true);
            else dt_d_neg_ = delay;
        }
    }
}

} // namespace vioavr::core
