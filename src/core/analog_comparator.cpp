#include "vioavr/core/analog_comparator.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/psc.hpp"
#include "vioavr/core/timer16.hpp"
#include <cmath>
#include <algorithm>

namespace vioavr::core {

AnalogComparator::AnalogComparator(std::string_view name, const AnalogComparatorDescriptor& desc, PinMux& pin_mux, u8 source_id, double hysteresis) noexcept
    : name_(name), desc_(desc), pin_mux_(&pin_mux), source_id_(source_id), hysteresis_(hysteresis) {
    mapped_ranges_.push_back({desc.acsr_address, desc.acsr_address});
    if (desc.accon_address != 0) {
        mapped_ranges_.push_back({desc.accon_address, desc.accon_address});
    }
    reset();
}

void AnalogComparator::reset() noexcept {
    acsr_ = 0x00U;
    accon_ = 0x00U;
    positive_input_ = 0.0;
    negative_input_ = 0.0;
    output_high_ = false;
    raw_output_ = false;
    pending_ = false;
    delay_counter_ = 0;
    propagation_delay_ = 2;
}

void AnalogComparator::on_event(u64 cycle) noexcept {
    Logger::debug("AC on_event [" + std::string(name_) + "]: cycle=" + std::to_string(cycle) + " raw=" + (raw_output_ ? "1" : "0"));
    if (is_disabled()) return;
    
    if (pending_) {
        const bool old_output = output_high_;
        output_high_ = raw_output_;
        pending_ = false;

        if (output_high_ != old_output) {
            // Notify interested peripherals
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

            if (trigger) {
                raise_interrupt_flag();
            }
        }
    }
}

u8 AnalogComparator::read(u16 address) noexcept {
    if (address == desc_.acsr_address) {
        u8 val = acsr_;
        if (output_high_ && !is_disabled()) val |= desc_.aco_mask;
        return val;
    }
    if (address == desc_.accon_address) return accon_;
    return 0;
}

void AnalogComparator::write(u16 address, u8 value) noexcept {
    if (address == desc_.acsr_address) {
        const u8 writable_mask = ~(desc_.aco_mask | desc_.acif_mask);
        acsr_ = (acsr_ & ~writable_mask) | (value & writable_mask);
        
        // Handle interrupt flag clearing (write 1 to clear)
        if (value & desc_.acif_mask) {
            acsr_ &= ~desc_.acif_mask;
        }
        
        evaluate_output();
    } else if (address == desc_.accon_address) {
        const u8 old = accon_;
        accon_ = value;
        
        // If transitioning from disabled to enabled, reset state to avoid stale high from sequential pin updates
        if (desc_.acd_mask && !(old & desc_.acd_mask) && (accon_ & desc_.acd_mask)) {
            raw_output_ = false;
            output_high_ = false;
        }
        
        evaluate_output();
    }
}

void AnalogComparator::on_analog_pin_change(u16 pin_address, u8 bit_index, double voltage) noexcept {
    bool changed = false;
    if (pin_address == desc_.aip_pin_address && bit_index == desc_.aip_pin_bit) {
        positive_input_ = voltage;
        changed = true;
    } else if (pin_address == desc_.aim_pin_address && bit_index == desc_.aim_pin_bit) {
        negative_input_ = voltage;
        changed = true;
    }

    if (changed) {
        evaluate_output();
    }
}

void AnalogComparator::evaluate_output() noexcept {
    if (is_disabled()) {
        return;
    }

    const double diff = positive_input_ - negative_input_;
    
    // Dynamic Hysteresis based on ACHYST
    double h = hysteresis_;
    if (desc_.achyst_mask) {
        u8 h_bits = (accon_ & desc_.achyst_mask) >> (__builtin_ctz(desc_.achyst_mask));
        if (h_bits == 1) h = 0.004; // ~20mV @ 5V
        else if (h_bits == 2) h = 0.01; // ~50mV @ 5V
        else h = 0.0;
    }

    Logger::debug("AC evaluate [" + std::string(name_) + "]: pos=" + std::to_string(positive_input_) + " neg=" + std::to_string(negative_input_) + " diff=" + std::to_string(diff) + " h=" + std::to_string(h) + " raw=" + (raw_output_ ? "1" : "0"));

    bool next_raw = raw_output_;
    if (raw_output_) {
        if (diff < -h) next_raw = false;
    } else {
        if (diff > h) next_raw = true;
    }

    if (next_raw != raw_output_) {
        raw_output_ = next_raw;
        // Start or reset the delay counter when raw comparator toggles
        if (raw_output_ != output_high_) {
            const double overdrive = std::abs(diff);
            if (overdrive >= 0.1) propagation_delay_ = 1;
            else if (overdrive > 0.02) propagation_delay_ = 2;
            else if (overdrive > 0.01) propagation_delay_ = 4;
            else propagation_delay_ = 8;
            
            delay_counter_ = propagation_delay_;
            if (bus_) {
                bus_->scheduler().cancel(ac_callback, this);
                bus_->scheduler().schedule(delay_counter_, ac_callback, this);
                pending_ = true;
            }
        } else {
            delay_counter_ = 0;
            if (bus_) bus_->scheduler().cancel(ac_callback, this);
            pending_ = false;
        }
    }
}

bool AnalogComparator::is_disabled() const noexcept {
    if (desc_.acd_mask && (acsr_ & desc_.acd_mask)) return true;
    if (desc_.acen_mask && !(accon_ & desc_.acen_mask)) return true;
    return false;
}

void AnalogComparator::raise_interrupt_flag() noexcept {
    if (acsr_ & desc_.acif_mask) return;
    acsr_ |= desc_.acif_mask;

    bool ie = (acsr_ & desc_.acie_mask) || (accon_ & desc_.acie_mask);
    if (ie) {
        if (bus_) bus_->notify_interrupt_state_change(this, true);
    }
}

u8 AnalogComparator::interrupt_mode() const noexcept {
    u8 mode_bits = (acsr_ & desc_.acis_mask) | (accon_ & desc_.acis_mask);
    return mode_bits >> (__builtin_ctz(desc_.acis_mask));
}

void AnalogComparator::ac_callback(u64 cycle, void* user_data) {
    static_cast<AnalogComparator*>(user_data)->on_event(cycle);
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

void AnalogComparator::connect_psc_fault(class Psc& psc) noexcept {
    psc_fault_listeners_.push_back(&psc);
}

void AnalogComparator::connect_dac(const class Dac& dac) noexcept {
    dac_ = &dac;
}

void AnalogComparator::connect_adc(const class Adc& adc) noexcept {
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

std::string_view AnalogComparator::name() const noexcept {
    return name_;
}

std::span<const AddressRange> AnalogComparator::mapped_ranges() const noexcept {
    return mapped_ranges_;
}

void AnalogComparator::tick(u64 elapsed_cycles) noexcept {
    if (signal_bank_) {
        refresh_bound_inputs();
    }
    (void)elapsed_cycles;
}

void AnalogComparator::refresh_bound_inputs() noexcept {
    if (signal_bank_) {
        positive_input_ = signal_bank_->voltage(positive_channel_);
        negative_input_ = signal_bank_->voltage(negative_channel_);
        evaluate_output();
    }
}

void AnalogComparator::update_pin_ownership() noexcept {
    // Implement if needed for GPIO override
}

bool AnalogComparator::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (acsr_ & desc_.acif_mask) {
        bool ie = (acsr_ & desc_.acie_mask) || (accon_ & desc_.acie_mask);
        if (ie) {
            request.vector_index = desc_.vector_index;
            request.source_id = source_id_;
            return true;
        }
    }
    return false;
}

bool AnalogComparator::consume_interrupt_request(InterruptRequest& request) noexcept {
    bool ie = (acsr_ & desc_.acie_mask) || (accon_ & desc_.acie_mask);
    if ((acsr_ & desc_.acif_mask) && ie) {
        request.vector_index = desc_.vector_index;
        request.source_id = source_id_;
        // NOTE: ACIF is usually cleared by hardware on vector jump, 
        // but some AVRs require manual clear. 
        // For fidelity, we assume hardware clear here.
        acsr_ &= ~desc_.acif_mask;
        if (bus_) bus_->notify_interrupt_state_change(this, false);
        return true;
    }
    return false;
}

} // namespace vioavr::core
