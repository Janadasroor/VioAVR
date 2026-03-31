#include "vioavr/core/analog_comparator.hpp"

#include "vioavr/core/adc.hpp"

#include <algorithm>

namespace vioavr::core {

namespace {
constexpr u8 kAcdMask = 0x80U;
constexpr u8 kAcoMask = 0x20U;
constexpr u8 kAciMask = 0x10U;
constexpr u8 kAcieMask = 0x08U;
constexpr u8 kAcisMask = 0x03U;
}

AnalogComparator::AnalogComparator(const std::string_view name,
                                   const u16 acsr_address,
                                   const u8 vector_index,
                                   const u8 source_id,
                                   const double hysteresis) noexcept
     : name_(name),
       range_({acsr_address, acsr_address}),
       acsr_address_(acsr_address),
       vector_index_(vector_index),
       source_id_(source_id),
       hysteresis_(std::max(hysteresis, 0.0))
{}

std::string_view AnalogComparator::name() const noexcept
{
     return name_;
}

std::span<const AddressRange> AnalogComparator::mapped_ranges() const noexcept
{
     return std::span<const AddressRange>(&range_, 1);
}

void AnalogComparator::reset() noexcept
{
    acsr_ = 0U;
    pending_ = false;
    refresh_bound_inputs();
    const double delta = positive_input_ - negative_input_;
    const double positive_threshold = hysteresis_ * 0.5;
    const double negative_threshold = -positive_threshold;
    if (delta >= positive_threshold) {
        output_high_ = true;
    } else if (delta <= negative_threshold) {
        output_high_ = false;
    }
    if (output_high_) {
        acsr_ |= kAcoMask;
    }
}

void AnalogComparator::tick(const u64 elapsed_cycles) noexcept
{
    (void)elapsed_cycles;
}

u8 AnalogComparator::read(const u16 address) noexcept
{
    refresh_bound_inputs();
    return address == acsr_address_ ? acsr_ : 0U;
}

void AnalogComparator::write(const u16 address, const u8 value) noexcept
{
    refresh_bound_inputs();
    if (address != acsr_address_) {
        return;
    }

    const u8 preserved_status = static_cast<u8>(acsr_ & kAcoMask);
    acsr_ = static_cast<u8>(preserved_status | (value & static_cast<u8>(~kAcoMask)));
    if ((value & kAciMask) != 0U) {
        acsr_ &= static_cast<u8>(~kAciMask);
        pending_ = false;
    }
    evaluate_output();
}

bool AnalogComparator::pending_interrupt_request(InterruptRequest& request) const noexcept
{
    const_cast<AnalogComparator*>(this)->refresh_bound_inputs();
    if (pending_ && (acsr_ & kAcieMask) != 0U && (acsr_ & kAcdMask) == 0U) {
        request = {.vector_index = vector_index_, .source_id = source_id_};
        return true;
    }
    return false;
}

bool AnalogComparator::consume_interrupt_request(InterruptRequest& request) noexcept
{
    refresh_bound_inputs();
    if (!pending_interrupt_request(request)) {
        return false;
    }

    if (request.vector_index == vector_index_) {
        pending_ = false;
        acsr_ &= static_cast<u8>(~kAciMask);
        return true;
    }
    return false;
}

void AnalogComparator::bind_signal_bank(const AnalogSignalBank& signal_bank,
                                        const u8 positive_channel,
                                        const u8 negative_channel) noexcept
{
    signal_bank_ = &signal_bank;
    positive_channel_ = positive_channel;
    negative_channel_ = negative_channel;
    refresh_bound_inputs();
}

void AnalogComparator::connect_adc_auto_trigger(Adc& adc) noexcept
{
    auto_trigger_adc_ = &adc;
}

void AnalogComparator::set_positive_input_voltage(const double normalized_voltage) noexcept
{
    positive_input_ = std::clamp(normalized_voltage, 0.0, 1.0);
    evaluate_output();
}

void AnalogComparator::set_negative_input_voltage(const double normalized_voltage) noexcept
{
    negative_input_ = std::clamp(normalized_voltage, 0.0, 1.0);
    evaluate_output();
}

void AnalogComparator::refresh_bound_inputs() noexcept
{
    if (signal_bank_ == nullptr) {
        return;
    }

    const double next_positive = signal_bank_->voltage(positive_channel_);
    const double next_negative = signal_bank_->voltage(negative_channel_);
    if (next_positive == positive_input_ && next_negative == negative_input_) {
        return;
    }

    positive_input_ = next_positive;
    negative_input_ = next_negative;
    evaluate_output();
}

void AnalogComparator::evaluate_output() noexcept
{
    const bool previous = output_high_;
    const double delta = positive_input_ - negative_input_;
    const double positive_threshold = hysteresis_ * 0.5;
    const double negative_threshold = -positive_threshold;

    if (delta >= positive_threshold) {
        output_high_ = true;
    } else if (delta <= negative_threshold) {
        output_high_ = false;
    }

    if (output_high_) {
        acsr_ |= kAcoMask;
    } else {
        acsr_ &= static_cast<u8>(~kAcoMask);
    }

    if ((acsr_ & kAcdMask) != 0U || output_high_ == previous) {
        return;
    }

    if (auto_trigger_adc_ != nullptr) {
        auto_trigger_adc_->notify_auto_trigger(Adc::AutoTriggerSource::comparator);
    }

    switch (interrupt_mode()) {
    case 0x00U:
    case 0x01U:
        raise_interrupt_flag();
        break;
    case 0x02U:
        if (previous && !output_high_) {
            raise_interrupt_flag();
        }
        break;
    case 0x03U:
        if (!previous && output_high_) {
            raise_interrupt_flag();
        }
        break;
    default:
        break;
    }
}

void AnalogComparator::raise_interrupt_flag() noexcept
{
    acsr_ |= kAciMask;
    pending_ = true;
}

u8 AnalogComparator::interrupt_mode() const noexcept
{
    return static_cast<u8>(acsr_ & kAcisMask);
}

}  // namespace vioavr::core
