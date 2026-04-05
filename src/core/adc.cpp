#include "vioavr/core/adc.hpp"
#include "vioavr/core/analog_comparator.hpp"
#include "vioavr/core/ext_interrupt.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/logger.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace vioavr::core {

namespace {
constexpr u8 kAdenMask = 0x80U;
constexpr u8 kAdscMask = 0x40U;
constexpr u8 kAdateMask = 0x20U;
constexpr u8 kAdifMask = 0x10U;
constexpr u8 kAdieMask = 0x08U;
constexpr u8 kAdlarMask = 0x20U;
}

Adc::Adc(std::string_view name,
         const AdcDescriptor& descriptor,
         PinMux& pin_mux,
         u8 source_id,
         u16 conversion_cycles) noexcept
    : name_(name),
      desc_(descriptor),
      pin_mux_(&pin_mux),
      source_id_(source_id),
      conversion_cycles_(conversion_cycles)
{
    std::vector<u16> addrs = { 
        desc_.adcl_address, desc_.adch_address, desc_.adcsra_address, 
        desc_.adcsrb_address, desc_.admux_address, desc_.didr0_address
    };
    std::sort(addrs.begin(), addrs.end());
    
    size_t ri = 0;
    ranges_.fill({0, 0});
    for (size_t i = 0; i < addrs.size(); ++i) {
        if (addrs[i] == 0) continue;
        if (ri > 0 && addrs[i] == ranges_[ri-1].end + 1) {
             ranges_[ri-1].end = addrs[i];
        } else {
             ranges_[ri++] = AddressRange{addrs[i], addrs[i]};
        }
    }
    trigger_select_register_ = (desc_.adcsrb_address != 0U) ? 1U : 0U;
}

std::string_view Adc::name() const noexcept { return name_; }

std::span<const AddressRange> Adc::mapped_ranges() const noexcept {
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

ClockDomain Adc::clock_domain() const noexcept
{
    return ClockDomain::adc;
}

void Adc::reset() noexcept {
    adcsra_ = desc_.adcsra_reset;
    adcsrb_ = desc_.adcsrb_reset;
    admux_ = desc_.admux_reset;
    didr0_ = 0x00U;
    result_ = 0x00U;
    converting_ = false;
    pending_ = false;
    cycles_remaining_ = 0;
    update_pin_ownership();
}

void Adc::tick(u64 elapsed_cycles) noexcept {
    if (!converting_ || (adcsra_ & kAdenMask) == 0U) return;

    if (elapsed_cycles >= cycles_remaining_) {
        complete_conversion();
    } else {
        cycles_remaining_ -= static_cast<u16>(elapsed_cycles);
    }
}

u8 Adc::read(u16 address) noexcept {
    if (address == desc_.adcl_address) return static_cast<u8>(result_ & 0xFFU);
    if (address == desc_.adch_address) return static_cast<u8>((result_ >> 8U) & 0xFFU);
    if (address == desc_.adcsra_address) return adcsra_;
    if (address == desc_.adcsrb_address) return adcsrb_;
    if (address == desc_.admux_address) return admux_;
    if (address == desc_.didr0_address) return didr0_;
    return 0x00U;
}

void Adc::write(u16 address, u8 value) noexcept {
    if (address == desc_.adcsra_address) {
        const bool was_enabled = (adcsra_ & kAdenMask);
        const bool next_enabled = (value & kAdenMask);
        
        // ADIF is cleared by writing 1
        if (value & kAdifMask) {
            adcsra_ &= ~kAdifMask;
        }
        
        // Update components of ADCSRA except ADIF which was handled above
        adcsra_ = (adcsra_ & kAdifMask) | (value & ~kAdifMask);
        
        if (!was_enabled && next_enabled) {
            update_pin_ownership();
        } else if (was_enabled && !next_enabled) {
            update_pin_ownership();
            converting_ = false;
        }

        if (next_enabled && (value & kAdscMask)) {
            start_conversion();
        }
    } else if (address == desc_.admux_address) {
        admux_ = value;
    } else if (address == desc_.adcsrb_address) {
        adcsrb_ = value;
        update_auto_trigger_source_from_register();
    } else if (address == desc_.didr0_address) {
        didr0_ = value;
        update_pin_ownership();
    }
}

bool Adc::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if ((adcsra_ & kAdieMask) && (adcsra_ & kAdifMask)) {
        request.vector_index = desc_.vector_index;
        return true;
    }
    return false;
}

bool Adc::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (pending_interrupt_request(request)) {
        adcsra_ &= ~kAdifMask; // Clear flag on entry
        return true;
    }
    return false;
}

void Adc::start_conversion() noexcept {
    if (!(adcsra_ & kAdenMask)) return;
    
    converting_ = true;
    cycles_remaining_ = conversion_cycles_;
    adcsra_ |= kAdscMask;
}

void Adc::complete_conversion() noexcept {
    const u8 channel = selected_channel();
    double voltage = 0.0;
    if (signal_bank_) {
        voltage = signal_bank_->voltage(channel);
    } else if (channel < local_channel_voltage_.size()) {
        voltage = local_channel_voltage_[channel];
    }

    result_ = static_cast<u16>(std::clamp(voltage * 1024.0, 0.0, 1023.0));
    
    adcsra_ &= ~kAdscMask;
    adcsra_ |= kAdifMask;
    converting_ = false;

    // Check for free-running mode restart
    if (free_running_enabled()) {
        start_conversion();
    }
}

u8 Adc::selected_channel() const noexcept {
    return admux_ & 0x07U; // Simplified 3-bit channel select
}

void Adc::update_pin_ownership() noexcept {
    if (!pin_mux_) return;

    const bool enabled = (adcsra_ & kAdenMask);
    for (u8 i = 0; i < 8; ++i) {
        const u16 pin_addr = desc_.adc_pin_address[i];
        const u8 pin_bit = desc_.adc_pin_bit[i];
        if (pin_addr == 0) continue;

        const bool disabled_digitally = (didr0_ & (1U << i));
        if (enabled && disabled_digitally) {
            pin_mux_->claim_pin_by_address(pin_addr, pin_bit, PinOwner::adc);
        } else {
            pin_mux_->release_pin_by_address(pin_addr, pin_bit, PinOwner::adc);
        }
    }
}

void Adc::bind_signal_bank(const AnalogSignalBank& signal_bank) noexcept {
    signal_bank_ = &signal_bank;
}

void Adc::select_auto_trigger_source(AutoTriggerSource source) noexcept {
    auto_trigger_source_ = source;
}

void Adc::connect_comparator_auto_trigger(AnalogComparator& comparator) noexcept {
    comparator.connect_adc_auto_trigger(*this);
}

void Adc::connect_external_interrupt_0_auto_trigger(ExtInterrupt& ext_interrupt) noexcept {
    ext_interrupt.connect_adc_auto_trigger(*this);
}

void Adc::connect_timer_compare_auto_trigger(Timer8& timer) noexcept {
    timer.connect_adc_auto_trigger(*this);
}

void Adc::connect_timer_overflow_auto_trigger(Timer8& timer) noexcept {
    timer.connect_adc_overflow_auto_trigger(*this);
}

void Adc::set_channel_voltage(u8 channel, double normalized_voltage) noexcept {
    if (channel < local_channel_voltage_.size()) {
        local_channel_voltage_[channel] = normalized_voltage;
    }
}

bool Adc::auto_trigger_enabled() const noexcept {
    return (adcsra_ & 0x20U) != 0;
}

bool Adc::free_running_enabled() const noexcept {
    return auto_trigger_enabled() && (auto_trigger_source_ == AutoTriggerSource::free_running);
}

Adc::AutoTriggerSource Adc::resolve_auto_trigger_source(u8 selector) const noexcept {
    static const AutoTriggerSource sources[] = {
        AutoTriggerSource::free_running,
        AutoTriggerSource::comparator,
        AutoTriggerSource::external_interrupt_0,
        AutoTriggerSource::timer_compare,
        AutoTriggerSource::timer_overflow,
        AutoTriggerSource::timer1_compare_b,
        AutoTriggerSource::timer1_overflow,
        AutoTriggerSource::timer1_capture
    };
    return sources[selector & 0x07U];
}

void Adc::update_auto_trigger_source_from_register() noexcept {
    auto_trigger_source_ = resolve_auto_trigger_source(adcsrb_ & 0x07U);
}

void Adc::restart_free_running_conversion() noexcept {
    if (free_running_enabled()) {
        start_conversion();
    }
}

void Adc::notify_auto_trigger(AutoTriggerSource source) noexcept {
    if (auto_trigger_enabled() && source == auto_trigger_source_) {
        start_conversion();
    }
}

} // namespace vioavr::core
