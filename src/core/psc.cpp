#include "vioavr/core/psc.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/logger.hpp"
#include <algorithm>

namespace vioavr::core {

Psc::Psc(std::string_view name, const PscDescriptor& desc, PinMux* pin_mux)
    : name_(name), desc_(desc), pin_mux_(pin_mux)
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
    
    fault_a_.blanking_duration = desc_.blanking_duration;
    fault_b_.blanking_duration = desc_.blanking_duration;

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
    fault_a_.blanking_counter = 0;
    fault_b_.blanking_counter = 0;
    last_output_a_ = false;
    last_output_b_ = false;
    output_a_ = false;
    output_b_ = false;
    output_c_ = false;
    output_d_ = false;
    
    update_outputs();
}

u8 Psc::read(u16 address) noexcept {
    if (address == desc_.pctl_address) return pctl_;
    if (address == desc_.psoc_address) return psoc_;
    if (address == desc_.pconf_address) return pconf_;
    if (address == desc_.pim_address) return pim_;
    if (address == desc_.pifr_address) return pifr_;
    if (address == desc_.pfrc0a_address) return pfrc0a_;
    if (address == desc_.pfrc0b_address) return pfrc0b_;
    
    if (address == desc_.ocrsa_address) {
        temp_ = (ocrsa_ >> 8) & 0x0F;
        return ocrsa_ & 0xFF;
    }
    if (address == desc_.ocrsa_address + 1) return temp_;
    
    if (address == desc_.ocrra_address) {
        temp_ = (ocrra_ >> 8) & 0x0F;
        return ocrra_ & 0xFF;
    }
    if (address == desc_.ocrra_address + 1) return temp_;
    
    if (address == desc_.ocrsb_address) {
        temp_ = (ocrsb_ >> 8) & 0x0F;
        return ocrsb_ & 0xFF;
    }
    if (address == desc_.ocrsb_address + 1) return temp_;
    
    if (address == desc_.ocrrb_address) {
        temp_ = (ocrrb_ >> 8) & 0x0F;
        return ocrrb_ & 0xFF;
    }
    if (address == desc_.ocrrb_address + 1) return temp_;
    
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
    } else if (address == desc_.ocrsa_address + 1) {
        temp_ = value;
    } else if (address == desc_.ocrsa_address) {
        ocrsa_ = (static_cast<u16>(temp_ & 0x0F) << 8) | value;
    } else if (address == desc_.ocrra_address + 1) {
        temp_ = value;
    } else if (address == desc_.ocrra_address) {
        ocrra_ = (static_cast<u16>(temp_ & 0x0F) << 8) | value;
    } else if (address == desc_.ocrsb_address + 1) {
        temp_ = value;
    } else if (address == desc_.ocrsb_address) {
        ocrsb_ = (static_cast<u16>(temp_ & 0x0F) << 8) | value;
    } else if (address == desc_.ocrrb_address + 1) {
        temp_ = value;
    } else if (address == desc_.ocrrb_address) {
        ocrrb_ = (static_cast<u16>(temp_ & 0x0F) << 8) | value;
    }
    update_outputs();
}

void Psc::tick(u64 elapsed_cycles) noexcept {
    if (!(pctl_ & desc_.prun_mask)) {
        cycle_accumulator_ = 0;
        update_outputs();
        return;
    }

    // High-Resolution clocking (PLL 64MHz vs I/O 16MHz)
    u64 effective_cycles = elapsed_cycles;
    if (pconf_ & desc_.clksel_mask) {
        bool pll_locked = true;
        if (bus_ && desc_.pllcsr_address != 0) {
            u8 pllcsr = bus_->read_data(desc_.pllcsr_address);
            pll_locked = (pllcsr & 0x01); // PLOCK
        }
        if (pll_locked) {
            effective_cycles *= 4;
        }
    }

    u8 ppre = (pctl_ & desc_.ppre_mask) >> 6;
    u32 divisor = 1;
    if (ppre == 1) divisor = 4;
    else if (ppre == 2) divisor = 32;
    else if (ppre == 3) divisor = 256;

    cycle_accumulator_ += effective_cycles;
    if (cycle_accumulator_ < divisor) return;

    u64 ticks = cycle_accumulator_ / divisor;
    cycle_accumulator_ %= divisor;

    u8 mode_val = (pconf_ & desc_.mode_mask) >> 3; 
    // PMODE: 0=One-Ramp, 1=Two-Ramp, 2=Four-Ramp, 3=Centered
    bool is_two_ramp = (mode_val == 1);
    bool is_four_ramp = (mode_val == 2);
    bool is_centered = (mode_val == 3);

    for (u64 i = 0; i < ticks; ++i) {
        // 1. Process Fault/Filter Logic for Channel A
        bool trigger_level_a = (pfrc0a_ & 0x20); // PELEV
        bool raw_triggered_a = (fault_a_.level == trigger_level_a);
        
        // Blanking Logic A
        if (fault_a_.blanking_counter > 0) {
            fault_a_.blanking_counter--;
            raw_triggered_a = false; 
        }

        if (pfrc0a_ & 0x10) { // PFLTE (Filter Enable)
            if (raw_triggered_a != fault_a_.filtered_level) {
                fault_a_.filter_samples++;
                if (fault_a_.filter_samples >= 4) {
                    fault_a_.filtered_level = raw_triggered_a;
                }
            } else {
                fault_a_.filter_samples = 0;
            }
        } else {
            fault_a_.filtered_level = raw_triggered_a;
        }

        // 2. Process Fault/Filter Logic for Channel B
        bool trigger_level_b = (pfrc0b_ & 0x20); // PELEV
        bool raw_triggered_b = (fault_b_.level == trigger_level_b);
        
        // Blanking Logic B
        if (fault_b_.blanking_counter > 0) {
            fault_b_.blanking_counter--;
            raw_triggered_b = false;
        }

        if (pfrc0b_ & 0x10) { // PFLTE (Filter Enable)
            if (raw_triggered_b != fault_b_.filtered_level) {
                fault_b_.filter_samples++;
                if (fault_b_.filter_samples >= 4) {
                    fault_b_.filtered_level = raw_triggered_b;
                }
            } else {
                fault_b_.filter_samples = 0;
            }
        } else {
            fault_b_.filtered_level = raw_triggered_b;
        }

        // 3. Handle PRFM (Retrigger and Fault Mode)
        u8 prfm_a = (pfrc0a_ & 0x0F);
        bool fault_occured = false;
        
        if (fault_a_.filtered_level) {
            switch (prfm_a) {
                case 0x01: // Retrigger on Edge
                    if (!last_fault_level_) { counter_ = 0; down_counting_ = false; }
                    break;
                case 0x02: // Retrigger on Level
                    counter_ = 0; down_counting_ = false;
                    break;
                case 0x06: // Fault on Level (Auto Restart)
                case 0x07: // Fault on Level (No Auto Restart)
                    fault_occured = true;
                    break;
                default: break;
            }
        }

        if (fault_occured && !fault_active_) {
            fault_active_ = true;
            pifr_ |= desc_.capt_flag_mask; // Fault interrupt
            if (bus_) bus_->request_analysis_freeze();
        } else if (!fault_occured && fault_active_) {
            if (prfm_a == 0x06) { // Auto-restart mode
                fault_active_ = false;
            }
        }
        
        last_fault_level_ = fault_a_.filtered_level;

        // 4. Counter Logic
        if (fault_active_) {
            u8 prfm = (pfrc0a_ & 0x0F);
            if (prfm == 0x07) continue; // In mode 7, counter stops
        }

        if (down_counting_) {
            if (counter_ > 0) counter_--;
            else {
                down_counting_ = false;
                pifr_ |= desc_.ec_flag_mask;
                notify_adc();
            }
        } else {
            counter_++;
            u16 top = ocrrb_;
            
            if (counter_ >= top && top > 0) {
                if (is_two_ramp || is_centered || is_four_ramp) {
                    down_counting_ = true;
                } else {
                    counter_ = 0;
                    pifr_ |= desc_.ec_flag_mask;
                    notify_adc();
                    
                    if (pctl_ & desc_.pccyc_mask) {
                        pctl_ &= ~desc_.prun_mask;
                    }
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

void Psc::notify_fault(bool level, u8 channel) noexcept {
    if (channel == 0) {
        fault_a_.level = level;
    } else if (channel == 1) {
        fault_b_.level = level;
    }
}

void Psc::handle_fault(bool level) noexcept {
    bool pelev = (pfrc0a_ & 0x20);
    bool triggered = false;
    
    if (!pelev && level) triggered = true;
    else if (pelev && !level) triggered = true;

    if (triggered && !fault_active_) {
        pifr_ |= desc_.capt_flag_mask; 
        fault_active_ = true;
        
        u8 prfm = (pfrc0a_ & 0x0F);
        if (prfm == 1 || prfm == 2 || prfm == 5 || prfm == 6) {
             counter_ = 0;
             down_counting_ = false;
        }
    } else if (!triggered && fault_active_) {
        u8 prfm = (pfrc0a_ & 0x0F);
        if (prfm == 0x03) {
             fault_active_ = false;
        }
    }
    
    last_fault_level_ = level;
    update_outputs();
}

void Psc::update_outputs() noexcept {
    if (!pin_mux_) return;

    if (!(pctl_ & desc_.prun_mask)) {
        output_a_ = output_b_ = false;
    } else {
        bool pulse_a = (counter_ >= ocrsa_ && counter_ < ocrra_);
        bool pulse_b = (counter_ >= ocrsb_ && counter_ < ocrrb_);

        if (pctl_ & desc_.pbfm_mask) {
            u16 top = ocrrb_;
            u16 center = top / 2;
            
            u16 width_a = (ocrra_ > ocrsa_) ? (ocrra_ - ocrsa_) : 0;
            pulse_a = (counter_ >= (center - width_a/2) && counter_ < (center + (width_a + 1)/2));
            
            u16 width_b = (ocrrb_ > ocrsb_) ? (ocrrb_ - ocrsb_) : 0;
            // Note: For Part B, width is usually relative to TOP? 
            // Actually, for PBFM, Part B is also centered.
            pulse_b = (counter_ >= (center - width_b/2) && counter_ < (center + (width_b + 1)/2));
        }

        bool pop = (pconf_ & 0x04) != 0;
        bool paoc_a = (pctl_ & desc_.paoca_mask);
        bool paoc_b = (pctl_ & desc_.paocb_mask);
        

        if (fault_active_) {
            output_a_ = paoc_a ? (pulse_a ^ pop) : ((psoc_ & 0x02) != 0);
            output_b_ = paoc_b ? (pulse_b ^ pop) : ((psoc_ & 0x08) != 0);
            
            if (desc_.psc_index == 2) {
                // For PSC2, we'll assume PAOC A/B also affect C/D or similar
                // (Depends on specific hardware, but usually PAOC is per channel pair)
                output_c_ = paoc_a ? (pulse_a ^ pop) : ((psoc_ & 0x02) != 0);
                output_d_ = paoc_b ? (pulse_b ^ pop) : ((psoc_ & 0x08) != 0);
            }
        } else {
            bool en_a = (psoc_ & 0x01);
            output_a_ = en_a && (pulse_a ^ pop);
            bool en_b = (psoc_ & 0x04);
            output_b_ = en_b && (pulse_b ^ pop);

            if (desc_.psc_index == 2) {
                bool en_c = (psoc_ & 0x02);
                bool en_d = (psoc_ & 0x08);
                output_c_ = en_c && (pulse_a ^ pop);
                output_d_ = en_d && (pulse_b ^ pop);
            }

            // Trigger Blanking on switching events
            if (output_a_ != last_output_a_) {
                fault_a_.blanking_counter = fault_a_.blanking_duration;
                last_output_a_ = output_a_;
            }
            if (output_b_ != last_output_b_) {
                fault_b_.blanking_counter = fault_b_.blanking_duration;
                last_output_b_ = output_b_;
            }
        }
    }

    // Update physical pins only if enabled in PSOC
    if (desc_.outa_pin_address) {
        if (psoc_ & 0x01) {
            pin_mux_->claim_pin_by_address(desc_.outa_pin_address, desc_.outa_pin_bit, PinOwner::psc);
            pin_mux_->update_pin_by_address(desc_.outa_pin_address, desc_.outa_pin_bit, PinOwner::psc, true, output_a_);
        } else {
            pin_mux_->release_pin_by_address(desc_.outa_pin_address, desc_.outa_pin_bit, PinOwner::psc);
        }
    }
    if (desc_.outb_pin_address) {
        if (psoc_ & 0x04) {
            pin_mux_->claim_pin_by_address(desc_.outb_pin_address, desc_.outb_pin_bit, PinOwner::psc);
            pin_mux_->update_pin_by_address(desc_.outb_pin_address, desc_.outb_pin_bit, PinOwner::psc, true, output_b_);
        } else {
            pin_mux_->release_pin_by_address(desc_.outb_pin_address, desc_.outb_pin_bit, PinOwner::psc);
        }
    }
    if (desc_.outc_pin_address) {
        if (psoc_ & 0x02) {
            pin_mux_->claim_pin_by_address(desc_.outc_pin_address, desc_.outc_pin_bit, PinOwner::psc);
            pin_mux_->update_pin_by_address(desc_.outc_pin_address, desc_.outc_pin_bit, PinOwner::psc, true, output_c_); 
        } else {
            pin_mux_->release_pin_by_address(desc_.outc_pin_address, desc_.outc_pin_bit, PinOwner::psc);
        }
    }
    if (desc_.outd_pin_address) {
        if (psoc_ & 0x08) {
            pin_mux_->claim_pin_by_address(desc_.outd_pin_address, desc_.outd_pin_bit, PinOwner::psc);
            pin_mux_->update_pin_by_address(desc_.outd_pin_address, desc_.outd_pin_bit, PinOwner::psc, true, output_d_);
        } else {
            pin_mux_->release_pin_by_address(desc_.outd_pin_address, desc_.outd_pin_bit, PinOwner::psc);
        }
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
