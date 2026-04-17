#include "vioavr/core/psc.hpp"
#include "vioavr/core/adc.hpp"
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
        bool run = (value & 0x80);
        if (run && !(pctl_ & 0x80)) {
            // Start
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
    if (!(pctl_ & 0x80)) return; // PRUN must be set

    u8 mode = (pctl_ >> 4) & 0x03;
    bool is_centered = (mode == 0x01);

    for (u64 i = 0; i < elapsed_cycles; ++i) {
        if (fault_active_) {
            // In some modes, we wait for end-of-cycle to clear fault?
            // Cycle counting continues in part of 'Fill' mode?
            // Simplified: If mode is Stop, we do nothing.
            u8 prfm = (pfrc0a_ >> 6) & 0x03;
            if (prfm == 0x00) break; // Latch Stop
        }

        if (down_counting_) {
            counter_--;
            if (counter_ == 0) {
                down_counting_ = false;
                pifr_ |= 0x01; // PEV
                notify_adc();
                
                // End of cycle clears fault in some modes
                u8 prfm = (pfrc0a_ >> 6) & 0x03;
                if (prfm == 0x01) fault_active_ = false; 
            }
        } else {
            counter_++;
            const u16 top = ocrrb_;
            if (counter_ >= top && top > 0) {
                if (is_centered) {
                    down_counting_ = true;
                } else {
                    counter_ = 0;
                    pifr_ |= 0x01; // PEV
                    notify_adc();
                    
                    u8 prfm = (pfrc0a_ >> 6) & 0x03;
                    if (prfm == 0x01) fault_active_ = false;
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
    u8 pflt = (pfrc0a_ >> 4) & 0x03;
    bool triggered = false;
    
    if (pflt == 0x01 && last_fault_level_ && !level) triggered = true; // Fall
    else if (pflt == 0x02 && !last_fault_level_ && level) triggered = true; // Rise
    else if (pflt == 0x03 && !level) triggered = true; // Low level

    if (triggered && !fault_active_) {
        pifr_ |= 0x08; // PCAP flag
        fault_active_ = true;
        
        // Retrigger immediately?
        u8 prfm = (pfrc0a_ >> 6) & 0x03;
        if (prfm == 0x01 || prfm == 0x02) {
             counter_ = 0;
             down_counting_ = false;
        }
    }
    
    // In retrigger mode, fault can clear when the signal is gone?
    // Lacking full specs, we assume latching for now unless PRFM allows recovery.
    
    last_fault_level_ = level;
    update_outputs();
}

void Psc::update_outputs() noexcept {
    if (!(pctl_ & 0x80)) {
        output_a_ = output_b_ = false;
        return;
    }

    bool pulse_a = (counter_ >= ocrsa_ && counter_ < ocrra_);
    bool pulse_b = (counter_ >= ocrsb_ && counter_ < ocrrb_);

    if (fault_active_) {
        // Safe level is usually the POL value
        output_a_ = (psoc_ & 0x01) && (psoc_ & 0x02);
        output_b_ = (psoc_ & 0x04) && (psoc_ & 0x08);
    } else {
        bool en_a = (psoc_ & 0x01);
        bool inv_a = (psoc_ & 0x02);
        output_a_ = en_a && (pulse_a ^ inv_a);

        bool en_b = (psoc_ & 0x04);
        bool inv_b = (psoc_ & 0x08);
        output_b_ = en_b && (pulse_b ^ inv_b);
    }
}

bool Psc::pending_interrupt_request(InterruptRequest& req) const noexcept {
    if ((pifr_ & 0x01) && (pim_ & 0x01)) {
        req.vector_index = desc_.ec_vector_index;
        return true;
    }
    if ((pifr_ & 0x08) && (pim_ & 0x08)) {
        req.vector_index = desc_.capt_vector_index;
        return true;
    }
    return false;
}

bool Psc::consume_interrupt_request(InterruptRequest& req) noexcept {
    if (pending_interrupt_request(req)) {
        if (req.vector_index == desc_.ec_vector_index) pifr_ &= ~0x01;
        else pifr_ &= ~0x08;
        return true;
    }
    return false;
}

} // namespace vioavr::core
