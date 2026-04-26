#include "vioavr/core/dac.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"
#include <algorithm>

namespace vioavr::core {

Dac::Dac(std::string_view name, const DacDescriptor& desc)
    : name_(name), desc_(desc)
{
    std::vector<u16> addrs = {
        desc.dacon_address, desc.dacl_address, desc.dach_address
    };
    std::sort(addrs.begin(), addrs.end());
    for (u16 addr : addrs) {
        if (addr == 0) continue;
        if (!ranges_.empty() && addr == ranges_.back().end + 1) {
            ranges_.back().end = addr;
        } else {
            ranges_.push_back({addr, addr});
        }
    }
}

std::span<const AddressRange> Dac::mapped_ranges() const noexcept {
    return {ranges_.data(), ranges_.size()};
}

void Dac::reset() noexcept {
    dacon_ = 0;
    data_ = 0;
    voltage_ = 0.0;
}

void Dac::tick(u64 elapsed_cycles) noexcept {
    (void)elapsed_cycles;
    if (power_reduction_enabled()) return;

    bool enabled = (dacon_ & desc_.daen_mask);
    if (enabled && (dacon_ & desc_.dacoe_mask)) {
        if (desc_.dac_pin_address) {
            bus_->pin_mux()->claim_pin_by_address(desc_.dac_pin_address, desc_.dac_pin_bit, PinOwner::dac);
            bus_->pin_mux()->update_analog_pin_by_address(desc_.dac_pin_address, desc_.dac_pin_bit, PinOwner::dac, voltage_);
        }
    } else if (desc_.dac_pin_address) {
        bus_->pin_mux()->release_pin_by_address(desc_.dac_pin_address, desc_.dac_pin_bit, PinOwner::dac);
    }
}

u8 Dac::read(u16 address) noexcept {
    if (address == desc_.dacon_address) return dacon_;
    if (address == desc_.dacl_address) return data_ & 0xFF;
    if (address == desc_.dach_address) return (data_ >> 8) & 0x03;
    return 0;
}

void Dac::write(u16 address, u8 value) noexcept {
    if (address == desc_.dacon_address) {
        dacon_ = value;
        update_voltage();
    } else if (address == desc_.dacl_address) {
        buffer_value_ = (buffer_value_ & 0xFF00) | value;
        if (!(dacon_ & desc_.daate_mask)) {
            data_ = buffer_value_;
            update_voltage();
        }
    } else if (address == desc_.dach_address) {
        buffer_value_ = (buffer_value_ & 0x00FF) | (static_cast<u16>(value & 0x03) << 8);
        if (!(dacon_ & desc_.daate_mask)) {
            data_ = buffer_value_;
            update_voltage();
        }
    }
}

void Dac::notify_auto_trigger(AdcAutoTriggerSource source) noexcept {
    if (!(dacon_ & desc_.daate_mask)) return;
    
    // Simple 1:1 mapping for common sources for now
    // DATS (bits 6:4) selector.
    u8 trigger_val = (dacon_ & desc_.dats_mask) >> 4;
    
    bool triggered = false;
    switch (trigger_val) {
        case 0: triggered = (source == AdcAutoTriggerSource::analog_comparator); break;
        case 1: triggered = (source == AdcAutoTriggerSource::analog_comparator_1); break;
        case 2: triggered = (source == AdcAutoTriggerSource::external_interrupt_0); break;
        case 3: triggered = (source == AdcAutoTriggerSource::timer0_compare_a); break;
        case 4: triggered = (source == AdcAutoTriggerSource::timer0_overflow); break;
        case 5: triggered = (source == AdcAutoTriggerSource::timer1_compare_b); break;
        case 6: triggered = (source == AdcAutoTriggerSource::timer1_overflow); break;
        case 7: triggered = (source == AdcAutoTriggerSource::timer1_capture); break;
        default: break;
    }

    if (triggered) {
        data_ = buffer_value_;
        update_voltage();
    }
}

bool Dac::power_reduction_enabled() const noexcept {
    if (!bus_ || desc_.pr_address == 0 || desc_.pr_bit == 0xFF) return false;
    return (bus_->read_data(desc_.pr_address) & (1 << desc_.pr_bit)) != 0;
}

void Dac::update_voltage() noexcept {
    if (power_reduction_enabled() || !(dacon_ & desc_.daen_mask)) {
        voltage_ = 0.0;
        return;
    }
    // Convert 10-bit data to 0.0-1.0 range
    voltage_ = static_cast<double>(data_) / 1023.0;
}

} // namespace vioavr::core
