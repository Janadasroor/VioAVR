#include "vioavr/core/opamp.hpp"
#include "vioavr/core/analog_signal_bank.hpp"

namespace vioavr::core {

static double opamp_gain_from_resmux(u8 resmux) noexcept {
    u8 gain_bits = resmux & 0x07U;
    switch (gain_bits) {
        case 0: return 1.0;
        case 1: return 2.0;
        case 2: return 4.0;
        case 3: return 8.0;
        case 4: return 16.0;
        case 5: return 32.0;
        case 6: return 64.0;
        default: return 1.0;
    }
}

Opamp::Opamp(const OpampDescriptor& desc, u8 instance_index) noexcept
    : name_("OPAMP" + std::to_string(instance_index)),
      desc_(desc), instance_index_(instance_index)
{
    if (desc_.ctrla_address != 0) {
        ranges_[0] = {desc_.ctrla_address, static_cast<u16>(desc_.muxctrl_address)};
    }
}

std::span<const AddressRange> Opamp::mapped_ranges() const noexcept {
    return {ranges_.data(), desc_.ctrla_address != 0 ? 1U : 0U};
}

void Opamp::reset() noexcept {
    ctrla_ = 0;
    ctrlb_ = 0;
    resctrl_ = 0;
    muxctrl_ = 0;
    vout_ = 0.0;
}

void Opamp::evaluate() {
    if (!is_enabled()) {
        vout_ = 0.0;
        return;
    }

    double vpos = 0.0, vneg = 0.0;
    if (desc_.pos_pin_address != 0 && signal_bank_) {
        // Map pos pin port+bit to signal bank channel
        u8 port_idx = 0;
        u16 addr = desc_.pos_pin_address;
        while (addr > 0x400) { addr -= 0x20; ++port_idx; }
        u8 bank_ch = port_idx * 8 + desc_.pos_pin_bit;
        vpos = signal_bank_->voltage(bank_ch);
    }
    if (desc_.neg_pin_address != 0 && signal_bank_) {
        u8 port_idx = 0;
        u16 addr = desc_.neg_pin_address;
        while (addr > 0x400) { addr -= 0x20; ++port_idx; }
        u8 bank_ch = port_idx * 8 + desc_.neg_pin_bit;
        vneg = signal_bank_->voltage(bank_ch);
    }

    double gain = opamp_gain_from_resmux(resctrl_);
    vout_ = gain * (vpos - vneg);
    if (vout_ < 0.0) vout_ = 0.0;
    if (vout_ > vdd_) vout_ = vdd_;

    if (signal_bank_) {
        signal_bank_->set_voltage(96 + instance_index_, vout_);
    }
}

void Opamp::tick(u64) noexcept {
    evaluate();
}

u8 Opamp::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.ctrlb_address) return ctrlb_;
    if (address == desc_.resctrl_address) return resctrl_;
    if (address == desc_.muxctrl_address) return muxctrl_;
    return 0;
}

void Opamp::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) ctrla_ = value;
    else if (address == desc_.ctrlb_address) ctrlb_ = value;
    else if (address == desc_.resctrl_address) resctrl_ = value;
    else if (address == desc_.muxctrl_address) muxctrl_ = value;
}

} // namespace vioavr::core
