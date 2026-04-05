#include "vioavr/core/analog_comparator.hpp"

#include "vioavr/core/adc.hpp"
#include "vioavr/core/logger.hpp"

#include <cmath>

namespace vioavr::core {

namespace {
constexpr u8 kAcdMask = 0x80U;
constexpr u8 kAcbgMask = 0x40U;
constexpr u8 kAcoMask = 0x20U;
constexpr u8 kAciMask = 0x10U;
constexpr u8 kAcieMask = 0x08U;
constexpr u8 kAcicMask = 0x04U;
constexpr u8 kAcisMask = 0x03U;
}

AnalogComparator::AnalogComparator(std::string_view name,
                                   const AnalogComparatorDescriptor& descriptor,
                                   PinMux& pin_mux,
                                   u8 source_id,
                                   double hysteresis) noexcept
    : name_(name),
      desc_(descriptor),
      pin_mux_(&pin_mux),
      range_({descriptor.acsr_address, descriptor.acsr_address}),
      source_id_(source_id),
      hysteresis_(hysteresis)
{
}

std::string_view AnalogComparator::name() const noexcept { return name_; }

std::span<const AddressRange> AnalogComparator::mapped_ranges() const noexcept {
    return {&range_, 1};
}

void AnalogComparator::reset() noexcept {
    acsr_ = 0x00U;
    didr1_ = 0x00U;
    output_high_ = false;
    pending_ = false;
    update_pin_ownership();
}

void AnalogComparator::tick(u64) noexcept {
    if (acsr_ & kAcdMask) return;
    refresh_bound_inputs();
    evaluate_output();
}

u8 AnalogComparator::read(u16 address) noexcept {
    if (address == desc_.acsr_address) {
        u8 val = acsr_;
        if (output_high_) val |= kAcoMask;
        return val;
    }
    if (address == desc_.didr1_address) return didr1_;
    return 0x00U;
}

void AnalogComparator::write(u16 address, u8 value) noexcept {
    if (address == desc_.acsr_address) {
        const u8 old_acsr = acsr_;
        acsr_ = (value & ~kAciMask);
        if (value & kAciMask) acsr_ &= ~kAciMask; // ACI is cleared by writing 1

        if ((old_acsr & kAcdMask) && !(acsr_ & kAcdMask)) {
            update_pin_ownership();
        } else if (!(old_acsr & kAcdMask) && (acsr_ & kAcdMask)) {
            update_pin_ownership();
        }
    } else if (address == desc_.didr1_address) {
        didr1_ = value;
        update_pin_ownership();
    }
}

bool AnalogComparator::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if ((acsr_ & kAcieMask) && (acsr_ & kAciMask)) {
        request.vector_index = desc_.vector_index;
        return true;
    }
    return false;
}

bool AnalogComparator::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (pending_interrupt_request(request)) {
        acsr_ &= ~kAciMask;
        return true;
    }
    return false;
}

void AnalogComparator::bind_signal_bank(const AnalogSignalBank& signal_bank, u8 positive_channel, u8 negative_channel) noexcept {
    signal_bank_ = &signal_bank;
    positive_channel_ = positive_channel;
    negative_channel_ = negative_channel;
}

void AnalogComparator::connect_adc_auto_trigger(Adc& adc) noexcept {
    auto_trigger_adc_ = &adc;
}

void AnalogComparator::set_positive_input_voltage(double normalized_voltage) noexcept {
    positive_input_ = normalized_voltage;
    evaluate_output();
}

void AnalogComparator::set_negative_input_voltage(double normalized_voltage) noexcept {
    negative_input_ = normalized_voltage;
    evaluate_output();
}

void AnalogComparator::refresh_bound_inputs() noexcept {
    if (!signal_bank_) return;
    positive_input_ = signal_bank_->voltage(positive_channel_);
    negative_input_ = signal_bank_->voltage(negative_channel_);
}

void AnalogComparator::evaluate_output() noexcept {
    if (acsr_ & kAcdMask) {
        output_high_ = false;
        return;
    }
    const bool was_high = output_high_;
    const double diff = positive_input_ - negative_input_;

    if (output_high_) {
        if (diff < -hysteresis_) output_high_ = false;
    } else {
        if (diff > hysteresis_) output_high_ = true;
    }

    if (was_high != output_high_) {
        const u8 mode = interrupt_mode();
        bool trigger = false;
        if (mode == 0) trigger = true; // Toggle
        else if (mode == 2) trigger = !output_high_; // Falling
        else if (mode == 3) trigger = output_high_; // Rising

        if (trigger) raise_interrupt_flag();
    }
}

void AnalogComparator::raise_interrupt_flag() noexcept {
    acsr_ |= kAciMask;
    if (auto_trigger_adc_ && (acsr_ & kAcicMask)) {
        auto_trigger_adc_->notify_auto_trigger(Adc::AutoTriggerSource::comparator);
    }
}

u8 AnalogComparator::interrupt_mode() const noexcept {
    return acsr_ & kAcisMask;
}

void AnalogComparator::update_pin_ownership() noexcept {
    if (!pin_mux_) return;

    const bool enabled = !(acsr_ & kAcdMask);
    
    // AIN0
    if (enabled && (didr1_ & 1U) && desc_.ain0_pin_address != 0) {
        pin_mux_->claim_pin_by_address(desc_.ain0_pin_address, desc_.ain0_pin_bit, PinOwner::comparator);
    } else if (desc_.ain0_pin_address != 0) {
        pin_mux_->release_pin_by_address(desc_.ain0_pin_address, desc_.ain0_pin_bit, PinOwner::comparator);
    }

    // AIN1
    if (enabled && (didr1_ & 2U) && desc_.ain1_pin_address != 0) {
        pin_mux_->claim_pin_by_address(desc_.ain1_pin_address, desc_.ain1_pin_bit, PinOwner::comparator);
    } else if (desc_.ain1_pin_address != 0) {
        pin_mux_->release_pin_by_address(desc_.ain1_pin_address, desc_.ain1_pin_bit, PinOwner::comparator);
    }
}

} // namespace vioavr::core
