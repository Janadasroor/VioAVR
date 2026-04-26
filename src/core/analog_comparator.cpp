#include "vioavr/core/analog_comparator.hpp"

#include "vioavr/core/adc.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/psc.hpp"
#include "vioavr/core/dac.hpp"
#include "vioavr/core/logger.hpp"

#include <cmath>

namespace vioavr::core {

AnalogComparator::AnalogComparator(std::string_view name,
                                   const AnalogComparatorDescriptor& descriptor,
                                   PinMux& pin_mux,
                                   u8 source_id,
                                   double hysteresis) noexcept
    : name_(name),
      desc_(descriptor),
      pin_mux_(&pin_mux),
      source_id_(source_id),
      hysteresis_(hysteresis)
{
    // Map multiple registers if they exist
    mapped_ranges_.push_back({desc_.acsr_address, desc_.acsr_address});
    if (desc_.accon_address && desc_.accon_address != desc_.acsr_address) {
        mapped_ranges_.push_back({desc_.accon_address, desc_.accon_address});
    }
    if (desc_.didr_address && desc_.didr_address != desc_.acsr_address && desc_.didr_address != desc_.accon_address) {
        mapped_ranges_.push_back({desc_.didr_address, desc_.didr_address});
    }
}

std::string_view AnalogComparator::name() const noexcept { return name_; }

std::span<const AddressRange> AnalogComparator::mapped_ranges() const noexcept {
    return {mapped_ranges_.data(), mapped_ranges_.size()};
}

void AnalogComparator::reset() noexcept {
    acsr_ = 0x00U;
    accon_ = 0x00U;
    didr_ = 0x00U;
    output_high_ = false;
    pending_ = false;
    update_pin_ownership();
}

void AnalogComparator::tick(u64) noexcept {
    if (is_disabled()) return;
    refresh_bound_inputs();
    evaluate_output();
}

u8 AnalogComparator::read(u16 address) noexcept {
    if (address == desc_.acsr_address) {
        u8 val = acsr_;
        if (output_high_) val |= desc_.aco_mask;
        return val;
    }
    if (address == desc_.accon_address) return accon_;
    if (address == desc_.didr_address) return didr_;
    return 0x00U;
}

void AnalogComparator::write(u16 address, u8 value) noexcept {
    if (address == desc_.acsr_address) {
        const u8 old_acsr = acsr_;
        // Clear flag if writing 1
        if (value & desc_.acif_mask) {
            acsr_ &= ~desc_.acif_mask;
        }
        // Update other bits except flag and read-only output
        const u8 mask = ~(desc_.acif_mask | desc_.aco_mask);
        acsr_ = (acsr_ & ~mask) | (value & mask);

        if ((old_acsr ^ acsr_) & desc_.acd_mask) {
            update_pin_ownership();
        }
    } else if (address == desc_.accon_address) {
        accon_ = value;
        update_pin_ownership();
        evaluate_output();
    } else if (address == desc_.didr_address) {
        didr_ = value;
        update_pin_ownership();
    }
}

bool AnalogComparator::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if ((acsr_ & desc_.acie_mask) && (acsr_ & desc_.acif_mask)) {
        request.vector_index = desc_.vector_index;
        return true;
    }
    // Also check ACCON if it has IE bits
    if (desc_.accon_address && (accon_ & desc_.acie_mask) && (acsr_ & desc_.acif_mask)) {
        request.vector_index = desc_.vector_index;
        return true;
    }
    return false;
}

bool AnalogComparator::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (pending_interrupt_request(request)) {
        acsr_ &= ~desc_.acif_mask;
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

void AnalogComparator::connect_timer_input_capture(Timer16& timer) noexcept {
    input_capture_timer_ = &timer;
}

void AnalogComparator::connect_psc_fault(Psc& psc) noexcept {
    psc_fault_listeners_.push_back(&psc);
}

void AnalogComparator::connect_dac(const Dac& dac) noexcept {
    dac_ = &dac;
}

void AnalogComparator::connect_adc(const Adc& adc) noexcept {
    source_adc_ = &adc;
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
    // If not PWM or no ACCON, use simple pin binding
    if (!desc_.accon_address) {
        if (signal_bank_) {
            positive_input_ = signal_bank_->voltage(positive_channel_);
            negative_input_ = signal_bank_->voltage(negative_channel_);
        } else if (pin_mux_) {
            positive_input_ = pin_mux_->get_state_by_address(desc_.aip_pin_address, desc_.aip_pin_bit).voltage;
            negative_input_ = pin_mux_->get_state_by_address(desc_.aim_pin_address, desc_.aim_pin_bit).voltage;
        }
        return;
    }

    // AT90PWM Complex Muxing logic
    const u8 mux = (accon_ & desc_.acm_mask) >> (__builtin_ctz(desc_.acm_mask));
    const double bandgap = 0.22; // 1.1V / 5.0V
    
    // Resolve Positive Input
    switch(mux) {
        case 3: positive_input_ = bandgap; break;
        case 4: if (dac_) positive_input_ = dac_->voltage(); break;
        default: // 0, 1, 2, 5, 6, 7 (Note: Case 6/ADC Mux handled by external binding)
            if (signal_bank_) {
                positive_input_ = signal_bank_->voltage(positive_channel_);
            } else if (pin_mux_) {
                positive_input_ = pin_mux_->get_state_by_address(desc_.aip_pin_address, desc_.aip_pin_bit).voltage;
            }
            break;
    }

    // Resolve Negative Input
    switch(mux) {
        case 0: if (signal_bank_) negative_input_ = signal_bank_->voltage(negative_channel_); break;
        case 1: negative_input_ = bandgap; break;
        case 2: if (dac_) negative_input_ = dac_->voltage(); break;
        default: // 3, 4, 5, 6, 7
            if (signal_bank_) {
                negative_input_ = signal_bank_->voltage(negative_channel_);
            } else if (pin_mux_) {
                negative_input_ = pin_mux_->get_state_by_address(desc_.aim_pin_address, desc_.aim_pin_bit).voltage;
            }
            break;
    }
}

void AnalogComparator::evaluate_output() noexcept {
    if (is_disabled()) {
        output_high_ = false;
        return;
    }

    const bool was_high = output_high_;
    const double diff = positive_input_ - negative_input_;
    
    // Dynamic Hysteresis based on ACHYST
    double h = hysteresis_;
    if (desc_.achyst_mask) {
        u8 h_bits = (accon_ & desc_.achyst_mask) >> (__builtin_ctz(desc_.achyst_mask));
        if (h_bits == 1) h = 0.004; // ~20mV @ 5V
        else if (h_bits == 2) h = 0.01; // ~50mV @ 5V
        else h = 0.0;
    }

    if (output_high_) {
        if (diff < -h) output_high_ = false;
    } else {
        if (diff > h) output_high_ = true;
    }

    if (was_high != output_high_) {
        for (auto* psc : psc_fault_listeners_) {
            psc->notify_fault(output_high_);
        }

        const u8 mode = interrupt_mode();
        bool trigger = false;
        switch (mode) {
            case 0: trigger = true; break; // Toggle
            case 2: trigger = !output_high_; break; // Falling
            case 3: trigger = output_high_; break; // Rising
            default: break;
        }
        if (trigger) raise_interrupt_flag();
    }
}

void AnalogComparator::raise_interrupt_flag() noexcept {
    if (acsr_ & desc_.acif_mask) return;
    acsr_ |= desc_.acif_mask;

    if (auto_trigger_adc_ && (acsr_ & desc_.acic_mask)) {
        auto_trigger_adc_->notify_auto_trigger(AdcAutoTriggerSource::analog_comparator);
    }

    if (input_capture_timer_ && (acsr_ & desc_.acic_mask)) {
        input_capture_timer_->notify_input_capture(output_high_);
    }
}

u8 AnalogComparator::interrupt_mode() const noexcept {
    u8 reg = desc_.accon_address ? accon_ : acsr_;
    if (!desc_.acis_mask) return 0;
    return (reg & desc_.acis_mask) >> (__builtin_ctz(desc_.acis_mask));
}

bool AnalogComparator::is_disabled() const noexcept {
    // ACD is active-high for "Disable" (1 = disabled)
    if (desc_.acd_mask) {
        if (acsr_ & desc_.acd_mask) return true;
        if (desc_.accon_address && (accon_ & desc_.acd_mask)) return true;
    }
    // ACEN is active-high for "Enable" (1 = enabled)
    if (desc_.acen_mask) {
        if (desc_.accon_address) {
            if (!(accon_ & desc_.acen_mask)) return true;
        } else {
            if (!(acsr_ & desc_.acen_mask)) return true;
        }
    }
    return false;
}

void AnalogComparator::update_pin_ownership() noexcept {
    if (!pin_mux_) return;
    
    if (is_disabled()) {
        pin_mux_->release_pin_by_address(desc_.aip_pin_address, desc_.aip_pin_bit, PinOwner::comparator);
        pin_mux_->release_pin_by_address(desc_.aim_pin_address, desc_.aim_pin_bit, PinOwner::comparator);
    } else {
        pin_mux_->claim_pin_by_address(desc_.aip_pin_address, desc_.aip_pin_bit, PinOwner::comparator);
        pin_mux_->claim_pin_by_address(desc_.aim_pin_address, desc_.aim_pin_bit, PinOwner::comparator);
    }
}

} // namespace vioavr::core
