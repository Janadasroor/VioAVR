#include "vioavr/core/zcd.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/memory_bus.hpp"

namespace vioavr::core {

Zcd::Zcd(const ZcdDescriptor& desc, u8 instance_index) noexcept
    : desc_(desc), instance_index_(instance_index) {
    if (desc_.ctrla_address != 0) {
        u16 end = static_cast<u16>(desc_.ctrla_address + 3);
        ranges_[0] = {desc_.ctrla_address, end};
    }
}

std::span<const AddressRange> Zcd::mapped_ranges() const noexcept {
    return {ranges_.data(), desc_.ctrla_address != 0 ? 1U : 0U};
}

void Zcd::reset() noexcept {
    ctrla_ = 0;
    intctrl_ = 0;
    status_ = 0;
    int_pending_ = false;
    sampled_ = false;
}

void Zcd::set_state(bool high) noexcept {
    bool old = (ctrla_ & 0x80) != 0;
    if (high == old) return;
    if (high) ctrla_ |= 0x80;
    else ctrla_ &= ~0x80;
    if (!(ctrla_ & 0x01)) return;
    u8 edge = intctrl_ & 0x03;
    bool fire = (edge == 0x00) || (edge == 0x01 && !high && old) || (edge == 0x02 && high && !old) || (edge == 0x03);
    if (fire) {
        status_ |= 0x01;
        int_pending_ = true;
    }
}

void Zcd::sample_analog() {
    if (!signal_bank_) return;
    double vin = signal_bank_->voltage(input_channel_);
    double vth = (ctrla_ & 0x04) ? 1.5 : vdd_ * 0.5;
    bool high = vin >= vth;
    if (!sampled_) {
        sampled_ = true;
        if (high) ctrla_ |= 0x80;
        else ctrla_ &= ~0x80;
        return;
    }
    set_state(high);
}

void Zcd::tick(u64) noexcept {
    sample_analog();
}

bool Zcd::on_external_pin_change(u16 pin_address, u8 bit_index, PinLevel level) noexcept {
    if (pin_address != desc_.pin_address || bit_index != desc_.pin_bit)
        return false;
    set_state(level == PinLevel::high);
    return true;
}

u8 Zcd::read(u16 address) noexcept {
    u16 offset = address - desc_.ctrla_address;
    switch (offset) {
        case 0: return ctrla_;
        case 1: return intctrl_;
        case 2: return status_;
        default: return 0;
    }
}

void Zcd::write(u16 address, u8 value) noexcept {
    u16 offset = address - desc_.ctrla_address;
    switch (offset) {
        case 0:
            ctrla_ = (ctrla_ & 0x80) | (value & 0x7F);
            break;
        case 1: intctrl_ = value & 0x03; break;
        case 2:
            status_ &= ~(value & 0x01);
            int_pending_ = (intctrl_ != 0) && (status_ & 0x01);
            break;
    }
}

bool Zcd::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (int_pending_ && desc_.vector_index != 0xFF) {
        request.vector_index = desc_.vector_index;
        return true;
    }
    return false;
}

bool Zcd::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (int_pending_ && desc_.vector_index != 0xFF) {
        request.vector_index = desc_.vector_index;
        int_pending_ = false;
        status_ &= ~0x01;
        return true;
    }
    return false;
}

} // namespace vioavr::core
