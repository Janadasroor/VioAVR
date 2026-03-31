#include "vioavr/core/adc.hpp"

#include "vioavr/core/analog_comparator.hpp"
#include "vioavr/core/ext_interrupt.hpp"
#include "vioavr/core/timer8.hpp"

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

Adc::Adc(const std::string_view name,
         const u16 adcl_address,
         const u16 adch_address,
         const u16 adcsra_address,
         const u16 admux_address,
         const u16 trigger_select_address,
         const u8 vector_index,
         const u8 source_id,
         const u16 conversion_cycles) noexcept
    : name_(name),
      adcl_address_(adcl_address),
      adch_address_(adch_address),
      adcsra_address_(adcsra_address),
      admux_address_(admux_address),
      trigger_select_address_(trigger_select_address),
      vector_index_(vector_index),
      source_id_(source_id),
      conversion_cycles_(conversion_cycles)
{
    std::vector<u16> addrs = {adcl_address, adch_address, adcsra_address, admux_address, trigger_select_address};
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
}

Adc::Adc(const std::string_view name, const DeviceDescriptor& device, const u8 source_id, const u16 conversion_cycles) noexcept
    : Adc(
          name,
          device.adc.adcl_address,
          device.adc.adch_address,
          device.adc.adcsra_address,
          device.adc.admux_address,
          device.adc.adcsrb_address,
          device.adc.vector_index,
          source_id,
          conversion_cycles
      )
{
    device_ = &device;
}

std::string_view Adc::name() const noexcept
{
    return "adc";
}

std::span<const AddressRange> Adc::mapped_ranges() const noexcept
{
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

void Adc::reset() noexcept
{
    adcsra_ = 0U;
    admux_ = 0U;
    trigger_select_register_ = 0U;
    result_ = 0U;
    cycles_remaining_ = 0U;
    update_auto_trigger_source_from_register();
    converting_ = false;
    pending_ = false;
}

void Adc::tick(u64 elapsed_cycles) noexcept
{
    if (!converting_) {
        return;
    }

    while (elapsed_cycles > 0U && converting_) {
        if (elapsed_cycles >= cycles_remaining_) {
            elapsed_cycles -= cycles_remaining_;
            cycles_remaining_ = 0U;
            complete_conversion();
        } else {
            cycles_remaining_ = static_cast<u16>(cycles_remaining_ - elapsed_cycles);
            elapsed_cycles = 0U;
        }
    }
}

u8 Adc::read(const u16 address) noexcept
{
    if (address == adcl_address_) {
        return static_cast<u8>(result_ & 0x00FFU);
    }
    if (address == adch_address_) {
        if ((admux_ & kAdlarMask) != 0U) {
            return static_cast<u8>((result_ >> 2U) & 0x00FFU);
        }
        return static_cast<u8>((result_ >> 8U) & 0x03U);
    }
    if (address == adcsra_address_) {
        return adcsra_;
    }
    if (address == admux_address_) {
        return admux_;
    }
    if (address == trigger_select_address_) {
        return trigger_select_register_;
    }
    return 0U;
}

void Adc::write(const u16 address, const u8 value) noexcept
{
    if (address == adcl_address_ || address == adch_address_) {
        return;
    }
    if (address == admux_address_) {
        admux_ = value;
        return;
    }
    if (address == trigger_select_address_) {
        trigger_select_register_ = value;
        update_auto_trigger_source_from_register();
        return;
    }
    if (address == adcsra_address_) {
        const bool start_requested = (value & kAdscMask) != 0U;
        const bool was_running = converting_;

        adcsra_ = static_cast<u8>((adcsra_ & kAdifMask) | (value & static_cast<u8>(~kAdifMask)));
        if ((value & kAdifMask) != 0U) {
            adcsra_ &= static_cast<u8>(~kAdifMask);
            pending_ = false;
        }
        if (start_requested && !was_running && (adcsra_ & kAdenMask) != 0U) {
            start_conversion();
        }
    }
}

bool Adc::pending_interrupt_request(InterruptRequest& request) const noexcept
{
    if (pending_ && (adcsra_ & kAdieMask) != 0U) {
        request = {.vector_index = vector_index_, .source_id = source_id_};
        return true;
    }
    return false;
}

bool Adc::consume_interrupt_request(InterruptRequest& request) noexcept
{
    if (!pending_interrupt_request(request)) {
        return false;
    }

    if (request.vector_index == vector_index_) {
        pending_ = false;
        adcsra_ &= static_cast<u8>(~kAdifMask);
        return true;
    }
    return false;
}

void Adc::bind_signal_bank(const AnalogSignalBank& signal_bank) noexcept
{
    signal_bank_ = &signal_bank;
}

void Adc::select_auto_trigger_source(const AutoTriggerSource source) noexcept
{
    trigger_select_register_ = static_cast<u8>(
        (trigger_select_register_ & static_cast<u8>(~0x07U)) | (static_cast<u8>(source) & 0x07U)
    );
    update_auto_trigger_source_from_register();
}

void Adc::connect_comparator_auto_trigger(AnalogComparator& comparator) noexcept
{
    select_auto_trigger_source(AutoTriggerSource::comparator);
    comparator.connect_adc_auto_trigger(*this);
}

void Adc::connect_external_interrupt_0_auto_trigger(ExtInterrupt& ext_interrupt) noexcept
{
    select_auto_trigger_source(AutoTriggerSource::external_interrupt_0);
    ext_interrupt.connect_adc_auto_trigger(*this);
}

void Adc::connect_timer_compare_auto_trigger(Timer8& timer) noexcept
{
    select_auto_trigger_source(AutoTriggerSource::timer_compare);
    timer.connect_adc_auto_trigger(*this);
}

void Adc::connect_timer_overflow_auto_trigger(Timer8& timer) noexcept
{
    select_auto_trigger_source(AutoTriggerSource::timer_overflow);
    timer.connect_adc_overflow_auto_trigger(*this);
}

void Adc::set_channel_voltage(const u8 channel, const double normalized_voltage) noexcept
{
    if (channel >= local_channel_voltage_.size()) {
        return;
    }

    local_channel_voltage_[channel] = std::clamp(normalized_voltage, 0.0, 1.0);
}

void Adc::notify_auto_trigger(const AutoTriggerSource source) noexcept
{
    if (source == auto_trigger_source_ && auto_trigger_enabled() && !converting_) {
        start_conversion();
    }
}

bool Adc::auto_trigger_enabled() const noexcept
{
    return (adcsra_ & static_cast<u8>(kAdenMask | kAdateMask)) == static_cast<u8>(kAdenMask | kAdateMask);
}

bool Adc::free_running_enabled() const noexcept
{
    return auto_trigger_enabled() && auto_trigger_source_ == AutoTriggerSource::free_running;
}

Adc::AutoTriggerSource Adc::resolve_auto_trigger_source(const u8 selector) const noexcept
{
    const auto map = device_ != nullptr ? device_->adc.auto_trigger_map
                                        : std::array<AdcAutoTriggerSource, 8> {
                                              AdcAutoTriggerSource::free_running,
                                              AdcAutoTriggerSource::analog_comparator,
                                              AdcAutoTriggerSource::external_interrupt_0,
                                              AdcAutoTriggerSource::timer0_compare,
                                              AdcAutoTriggerSource::timer0_overflow,
                                              AdcAutoTriggerSource::timer1_compare_b,
                                              AdcAutoTriggerSource::timer1_overflow,
                                              AdcAutoTriggerSource::timer1_capture
                                          };

    switch (map[selector & 0x07U]) {
    case AdcAutoTriggerSource::free_running:
        return AutoTriggerSource::free_running;
    case AdcAutoTriggerSource::analog_comparator:
        return AutoTriggerSource::comparator;
    case AdcAutoTriggerSource::external_interrupt_0:
        return AutoTriggerSource::external_interrupt_0;
    case AdcAutoTriggerSource::timer0_compare:
        return AutoTriggerSource::timer_compare;
    case AdcAutoTriggerSource::timer0_overflow:
        return AutoTriggerSource::timer_overflow;
    default:
        return AutoTriggerSource::none;
    }
}

void Adc::update_auto_trigger_source_from_register() noexcept
{
    auto_trigger_source_ = resolve_auto_trigger_source(trigger_select_register_);
}

void Adc::start_conversion() noexcept
{
    converting_ = true;
    cycles_remaining_ = conversion_cycles_;
    adcsra_ |= kAdscMask;
    adcsra_ &= static_cast<u8>(~kAdifMask);
    pending_ = false;
}

void Adc::restart_free_running_conversion() noexcept
{
    converting_ = true;
    cycles_remaining_ = conversion_cycles_;
    adcsra_ |= kAdscMask;
}

void Adc::complete_conversion() noexcept
{
    converting_ = false;
    adcsra_ &= static_cast<u8>(~kAdscMask);
    adcsra_ |= kAdifMask;
    pending_ = true;

    const u8 channel = selected_channel();
    const double voltage = signal_bank_ != nullptr ? signal_bank_->voltage(channel) : local_channel_voltage_[channel];
    result_ = static_cast<u16>(std::lround(voltage * 1023.0));

    if (free_running_enabled()) {
        restart_free_running_conversion();
    }
}

u8 Adc::selected_channel() const noexcept
{
    return static_cast<u8>(admux_ & 0x07U);
}

}  // namespace vioavr::core
