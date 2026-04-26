#include "vioavr/core/amplifier.hpp"

namespace vioavr::core {

At90Amplifier::At90Amplifier(std::string_view name, const AmplifierDescriptor& desc, PinMux& pin_mux) noexcept
    : name_(name), desc_(desc), pin_mux_(&pin_mux)
{
    range_ = {desc_.ampcsr_address, desc_.ampcsr_address};
}

std::span<const AddressRange> At90Amplifier::mapped_ranges() const noexcept {
    return {&range_, 1};
}

void At90Amplifier::reset() noexcept {
    ampcsr_ = 0;
    voltage_out_ = 0.0;
    update_pin_ownership();
}

void At90Amplifier::tick(u64) noexcept {
    if (!(ampcsr_ & desc_.ampen_mask)) return;
    
    evaluate();
}

u8 At90Amplifier::read(u16 address) noexcept {
    if (address == desc_.ampcsr_address) return ampcsr_;
    return 0;
}

void At90Amplifier::write(u16 address, u8 value) noexcept {
    if (address == desc_.ampcsr_address) {
        const u8 old = ampcsr_;
        ampcsr_ = value;
        if ((old ^ ampcsr_) & desc_.ampen_mask) {
            update_pin_ownership();
        }
        evaluate();
    }
}

void At90Amplifier::evaluate() noexcept {
    if (!(ampcsr_ & desc_.ampen_mask)) {
        voltage_out_ = 0.0;
        return;
    }

    double v_pos = pin_mux_->get_state_by_address(desc_.pos_pin_address, desc_.pos_pin_bit).voltage;
    double v_neg = pin_mux_->get_state_by_address(desc_.neg_pin_address, desc_.neg_pin_bit).voltage;

    // Gain mapping: 00: 5, 01: 10, 10: 20, 11: 40
    u8 g_bits = 0;
    if (desc_.ampg_mask != 0) {
        g_bits = (ampcsr_ & desc_.ampg_mask) >> (__builtin_ctz(desc_.ampg_mask));
    }
    
    double gain = 5.0;
    switch (g_bits) {
        case 1: gain = 10.0; break;
        case 2: gain = 20.0; break;
        case 3: gain = 40.0; break;
    }

    // Differential amplification
    voltage_out_ = (v_pos - v_neg) * gain;
    
    // Clamp to [0, 1] assuming normalized range relative to Vcc
    if (voltage_out_ < 0.0) voltage_out_ = 0.0;
    if (voltage_out_ > 1.0) voltage_out_ = 1.0;
}

void At90Amplifier::update_pin_ownership() noexcept {
    if (ampcsr_ & desc_.ampen_mask) {
        pin_mux_->claim_pin_by_address(desc_.pos_pin_address, desc_.pos_pin_bit, PinOwner::amplifier);
        pin_mux_->claim_pin_by_address(desc_.neg_pin_address, desc_.neg_pin_bit, PinOwner::amplifier);
    } else {
        pin_mux_->release_pin_by_address(desc_.pos_pin_address, desc_.pos_pin_bit, PinOwner::amplifier);
        pin_mux_->release_pin_by_address(desc_.neg_pin_address, desc_.neg_pin_bit, PinOwner::amplifier);
    }
}

} // namespace vioavr::core
