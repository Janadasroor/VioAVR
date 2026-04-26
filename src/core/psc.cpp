#include "vioavr/core/psc.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/logger.hpp"
#include <algorithm>

namespace vioavr::core {

Psc::Psc(std::string_view name, const PscDescriptor& desc)
    : name_(name), desc_(desc)
{
    std::vector<u16> addrs = {
        desc.pctl_address, desc.psoc_address, desc.pconf_address,
        desc.pim_address, desc.pifr_address, desc.picr_address,
        desc.ocrsa_address, static_cast<u16>(desc.ocrsa_address + 1),
        desc.ocrra_address, static_cast<u16>(desc.ocrra_address + 1),
        desc.ocrsb_address, static_cast<u16>(desc.ocrsb_address + 1),
        desc.ocrrb_address, static_cast<u16>(desc.ocrrb_address + 1),
        desc.pfrc0a_address, desc.pfrc0b_address
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

std::span<const AddressRange> Psc::mapped_ranges() const noexcept {
    return {ranges_.data(), ranges_.size()};
}

void Psc::reset() noexcept {
    counter_ = 0;
    pctl_ = 0;
    psoc_ = 0;
    pconf_ = 0;
    pim_ = 0;
    pifr_ = 0;
    ocrsa_ = 0;
    ocrra_ = 0;
    ocrsb_ = 0;
    ocrrb_ = 0;
    pfrc0a_ = 0;
    pfrc0b_ = 0;
    temp_ = 0;
    down_counting_ = false;
    fault_active_ = false;
    last_fault_level_ = false;
    fault_pending_restart_ = false;
    output_a_ = false;
    output_b_ = false;
}

u8 Psc::read(u16 address) noexcept {
    if (address == desc_.pctl_address) return pctl_;
    if (address == desc_.psoc_address) return psoc_;
    if (address == desc_.pconf_address) return pconf_;
    if (address == desc_.pim_address) return pim_;
    if (address == desc_.pifr_address) return pifr_;
    if (address == desc_.pfrc0a_address) return pfrc0a_;
    if (address == desc_.pfrc0b_address) return pfrc0b_;
    
    if (address == desc_.ocrsa_address) return ocrsa_ & 0xFF;
    if (address == desc_.ocrsa_address + 1) return (ocrsa_ >> 8) & 0x0F;
    if (address == desc_.ocrra_address) return ocrra_ & 0xFF;
    if (address == desc_.ocrra_address + 1) return (ocrra_ >> 8) & 0x0F;
    if (address == desc_.ocrsb_address) return ocrsb_ & 0xFF;
    if (address == desc_.ocrsb_address + 1) return (ocrsb_ >> 8) & 0x0F;
    if (address == desc_.ocrrb_address) return ocrrb_ & 0xFF;
    if (address == desc_.ocrrb_address + 1) return (ocrrb_ >> 8) & 0x0F;
    
    return 0;
}

void Psc::write(u16 address, u8 value) noexcept {
    if (address == desc_.pctl_address) {
        bool run = (value & desc_.prun_mask);
        if (run && !(pctl_ & desc_.prun_mask)) {
            counter_ = 0;
            down_counting_ = false;
            fault_active_ = false;
        } else if (!run) {
            fault_active_ = false;
        }
        pctl_ = value;
    } else if (address == desc_.psoc_address) {
        psoc_ = value;
    } else if (address == desc_.pconf_address) {
        pconf_ = value;
    } else if (address == desc_.pim_address) {
        pim_ = value;
    } else if (address == desc_.pifr_address) {
        pifr_ &= ~value;
    } else if (address == desc_.pfrc0a_address) {
        pfrc0a_ = value;
    } else if (address == desc_.pfrc0b_address) {
        pfrc0b_ = value;
    } else if (address == desc_.ocrsa_address) {
        temp_ = value;
    } else if (address == desc_.ocrsa_address + 1) {
        ocrsa_ = (static_cast<u16>(value & 0x0F) << 8) | temp_;
    } else if (address == desc_.ocrra_address) {
        temp_ = value;
    } else if (address == desc_.ocrra_address + 1) {
        ocrra_ = (static_cast<u16>(value & 0x0F) << 8) | temp_;
    } else if (address == desc_.ocrsb_address) {
        temp_ = value;
    } else if (address == desc_.ocrsb_address + 1) {
        ocrsb_ = (static_cast<u16>(value & 0x0F) << 8) | temp_;
    } else if (address == desc_.ocrrb_address) {
        temp_ = value;
    } else if (address == desc_.ocrrb_address + 1) {
        ocrrb_ = (static_cast<u16>(value & 0x0F) << 8) | temp_;
    }
    update_outputs();
}

void Psc::tick(u64 elapsed_cycles) noexcept {
    if (!(pctl_ & desc_.prun_mask)) {
        cycle_accumulator_ = 0;
        return;
    }

    u8 ppre = (pctl_ & desc_.ppre_mask) >> 6; // Usually bits 7:6
    u32 divisor = 1;
    if (ppre == 1) divisor = 4;
    else if (ppre == 2) divisor = 32;
    else if (ppre == 3) divisor = 256;

    cycle_accumulator_ += elapsed_cycles;
    if (cycle_accumulator_ < divisor) return;

    u64 ticks = cycle_accumulator_ / divisor;
    cycle_accumulator_ %= divisor;

    u8 mode_val = (pconf_ & desc_.mode_mask) >> 3; // Bits 4:3
    // PMODE: 0=One-Ramp, 1=Two-Ramp, 2=Four-Ramp, 3=Centered
    bool is_two_ramp = (mode_val == 1);
    bool is_four_ramp = (mode_val == 2);
    bool is_centered = (mode_val == 3);

    for (u64 i = 0; i < ticks; ++i) {
        if (fault_active_) {
            u8 prfm = (pfrc0a_ & 0x0F);
            // PRFM 7 is Halt. If halted, we don't tick the counter.
            if (prfm == 0x07) break; 
        }

        if (down_counting_) {
            counter_--;
            if (counter_ == 0) {
                down_counting_ = false;
                pifr_ |= desc_.ec_flag_mask;
                notify_adc();
                
                u8 prfm = (pfrc0a_ & 0x0F);
                if (prfm == 0x04) fault_active_ = false;
            }
        } else {
            counter_++;
            const u16 top = ocrrb_;
            if (counter_ >= top && top > 0) {
                if (is_two_ramp || is_centered) {
                    down_counting_ = true;
                } else if (is_four_ramp) {
                    // Four-ramp is more complex, but we'll treat it as two-ramp for now
                    down_counting_ = true;
                } else {
                    counter_ = 0;
                    pifr_ |= desc_.ec_flag_mask;
                    notify_adc();
                    
                    u8 prfm = (pfrc0a_ & 0x0F);
                    if (prfm == 0x04) fault_active_ = false;
                }
            }
        }
        update_outputs();
    }
}

void Psc::notify_adc() noexcept {
    if (adc_trigger_) {
        AdcAutoTriggerSource src = AdcAutoTriggerSource::psc0_sync;
        if (desc_.psc_index == 1) src = AdcAutoTriggerSource::psc1_sync;
        else if (desc_.psc_index == 2) src = AdcAutoTriggerSource::psc2_sync;
        adc_trigger_->notify_auto_trigger(src);
    }
}

void Psc::notify_fault(bool level) noexcept {
    handle_fault(level);
}

void Psc::handle_fault(bool level) noexcept {
    // PELEV0A is bit 5
    bool pelev = (pfrc0a_ & 0x20);
    bool triggered = false;
    
    // If PELEV is 0, fault is active when level is HIGH (Pos > Neg in AC)
    // If PELEV is 1, fault is active when level is LOW
    if (!pelev && level) triggered = true;
    else if (pelev && !level) triggered = true;

    if (triggered && !fault_active_) {
        pifr_ |= desc_.capt_flag_mask; 
        fault_active_ = true;
        
        // PRFM logic (bits 3:0)
        u8 prfm = (pfrc0a_ & 0x0F);
        if (prfm == 1 || prfm == 2 || prfm == 5 || prfm == 6) {
             // Retrigger or Stop & Reset modes
             counter_ = 0;
             down_counting_ = false;
        }
    } else if (!triggered && fault_active_) {
        // Clear fault if in purely level-sensitive mode 
        u8 prfm = (pfrc0a_ & 0x0F);
        if (prfm == 0x03) { // Stop signal, Execute Opposite while Fault active
             fault_active_ = false;
        }
    }
    
    last_fault_level_ = level;
    update_outputs();
}

void Psc::update_outputs() noexcept {
    if (!(pctl_ & desc_.prun_mask)) {
        output_a_ = output_b_ = false;
        return;
    }

    bool pulse_a = (counter_ >= ocrsa_ && counter_ < ocrra_);
    bool pulse_b = (counter_ >= ocrsb_ && counter_ < ocrrb_);

    if (fault_active_) {
        // Output state depends on PSOC bits 1 (POV0A) and 3 (POV0B)
        output_a_ = (psoc_ & 0x02) != 0;
        output_b_ = (psoc_ & 0x08) != 0;
    } else {
        bool pop = (pconf_ & 0x04); // Global Polarity bit
        bool en_a = (psoc_ & 0x01);
        output_a_ = en_a && (pulse_a ^ pop);

        bool en_b = (psoc_ & 0x04);
        output_b_ = en_b && (pulse_b ^ pop);
    }
}

bool Psc::pending_interrupt_request(InterruptRequest& req) const noexcept {
    if ((pifr_ & desc_.ec_flag_mask) && (pim_ & desc_.ec_flag_mask)) {
        req.vector_index = desc_.ec_vector_index;
        return true;
    }
    if ((pifr_ & desc_.capt_flag_mask) && (pim_ & desc_.capt_flag_mask)) {
        req.vector_index = desc_.capt_vector_index;
        return true;
    }
    return false;
}

bool Psc::consume_interrupt_request(InterruptRequest& req) noexcept {
    if (pending_interrupt_request(req)) {
        if (req.vector_index == desc_.ec_vector_index) pifr_ &= ~desc_.ec_flag_mask;
        else pifr_ &= ~desc_.capt_flag_mask;
        return true;
    }
    return false;
}

} // namespace vioavr::core
