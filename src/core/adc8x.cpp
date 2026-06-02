#include "vioavr/core/adc8x.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/evsys.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/logger.hpp"
#include <algorithm>
#include <cstdio>
#include <vector>

namespace vioavr::core {

Adc8x::Adc8x(const Adc8xDescriptor& desc) noexcept : desc_(desc) {
    std::vector<u16> addrs;
    auto add_if_valid = [&](u16 addr) {
        if (addr != 0) addrs.push_back(addr);
    };

    add_if_valid(desc_.ctrla_address);
    add_if_valid(desc_.ctrlb_address);
    add_if_valid(desc_.ctrlc_address);
    add_if_valid(desc_.ctrld_address);
    add_if_valid(desc_.ctrle_address);
    add_if_valid(desc_.sampctrl_address);
    add_if_valid(desc_.muxpos_address);
    add_if_valid(desc_.muxneg_address);
    add_if_valid(desc_.command_address);
    add_if_valid(desc_.evctrl_address);
    add_if_valid(desc_.intctrl_address);
    add_if_valid(desc_.intflags_address);
    add_if_valid(desc_.dbgctrl_address);
    add_if_valid(desc_.temp_address);
    add_if_valid(desc_.res_address);
    add_if_valid(desc_.res_address + 1);
    add_if_valid(desc_.winlt_address);
    add_if_valid(desc_.winlt_address + 1);
    add_if_valid(desc_.winht_address);
    add_if_valid(desc_.winht_address + 1);
    
    // Sort and compact ranges
    std::sort(addrs.begin(), addrs.end());
    addrs.erase(std::unique(addrs.begin(), addrs.end()), addrs.end());
    
    size_t count = 0;
    if (!addrs.empty()) {
        u16 start = addrs[0];
        u16 end = addrs[0];
        for (size_t i = 1; i < addrs.size(); ++i) {
            if (addrs[i] == end + 1) {
                end = addrs[i];
            } else {
                ranges_[count++] = {start, end};
                start = addrs[i];
                end = addrs[i];
                if (count >= ranges_.size()) break;
            }
        }
        if (count < ranges_.size()) {
            ranges_[count++] = {start, end};
        }
    }
    num_ranges_ = count;
}

void Adc8x::set_event_system(EventSystem* evsys) noexcept {
    evsys_ = evsys;
    if (evsys_ && desc_.user_event_address != 0) {
        u16 user_base = evsys_->users_base();
        if (desc_.user_event_address >= user_base) {
            u8 user_index = static_cast<u8>(desc_.user_event_address - user_base);
            evsys_->register_user_callback(user_index, [this](bool) {
                if (is_enabled() && (evctrl_ & 0x01U)) {
                    start_conversion();
                }
            });
        }
    }
}

void Adc8x::reset() noexcept {
    ctrla_ = 0;
    ctrlb_ = 0;
    ctrlc_ = 0;
    ctrld_ = 0;
    ctrle_ = 0;
    sampctrl_ = 0;
    muxpos_ = 0; // ATmega4809 resets MUXPOS to 0 (AIN0)
    muxneg_ = 0;
    command_ = 0;
    evctrl_ = 0;
    intctrl_ = 0;
    intflags_ = 0;
    dbgctrl_ = 0;
    calib_ = 0;
    res_ = 0;
    winlt_ = 0;
    winht_ = 0;
    converting_ = false;
    state_ = AdcPhase::idle;
    fractional_cycles_ = 0;
    cycles_in_phase_ = 0;
    current_sample_ = 0;
    accumulated_res_ = 0;
    res_latch_ = 0;
    res_latch_valid_ = false;
}

void Adc8x::tick(u64 elapsed_cycles) noexcept {
            // debug is_enabled()?1:0, (int)state_, (int)converting_);
    if (!is_enabled()) {
        state_ = AdcPhase::idle;
        converting_ = false;
        return;
    }

    if (state_ == AdcPhase::idle) {
        return;
    }

    u16 prescaler = get_prescaler();
    u64 total_cycles = elapsed_cycles + fractional_cycles_;
    u64 adc_ticks = total_cycles / prescaler;
    fractional_cycles_ = total_cycles % prescaler;

    for (u64 t = 0; t < adc_ticks; ++t) {
        if (cycles_in_phase_ > 0) {
            cycles_in_phase_--;
        }

        if (cycles_in_phase_ == 0) {
            switch (state_) {
            case AdcPhase::startup:
                state_ = AdcPhase::sample;
                cycles_in_phase_ = 2 + (sampctrl_ & 0x1FU); 
                break;

            case AdcPhase::sample:
                state_ = AdcPhase::convert;
                cycles_in_phase_ = 13;
                break;

            case AdcPhase::convert: {
                complete_conversion();
                current_sample_++;
                if (current_sample_ < get_sample_count()) {
                    state_ = AdcPhase::sample;
                    cycles_in_phase_ = 2 + (sampctrl_ & 0x1FU);
                } else {
                    u32 result = accumulated_res_;
                    process_sample_result(result);
                    if (ctrla_ & 0x02U) { // FREERUN — auto-restart
                        state_ = AdcPhase::sample;
                        converting_ = true;
                        current_sample_ = 0;
                        accumulated_res_ = 0;
                        cycles_in_phase_ = 2 + (sampctrl_ & 0x1FU);
                    } else {
                        state_ = AdcPhase::idle;
                        converting_ = false;
                    }
                }
                break;
            }

            default:
                break;
            }
        }
    }
}

u8 Adc8x::read(u16 address) noexcept {
    u8 val = 0;
    if (address == desc_.ctrla_address) val = ctrla_;
    else if (address == desc_.ctrlb_address) val = ctrlb_;
    else if (address == desc_.ctrlc_address) val = ctrlc_;
    else if (address == desc_.ctrld_address) val = ctrld_;
    else if (address == desc_.ctrle_address) val = ctrle_;
    else if (address == desc_.sampctrl_address) val = sampctrl_;
    else if (address == desc_.muxpos_address) val = muxpos_;
    else if (address == desc_.muxneg_address) val = muxneg_;
    else if (address == desc_.command_address) val = 0;
    else if (address == desc_.evctrl_address) val = evctrl_;
    else if (address == desc_.intctrl_address) val = intctrl_;
    else if (address == desc_.intflags_address) val = intflags_;
    else if (address == desc_.dbgctrl_address) val = dbgctrl_;
    else if (address == desc_.temp_address) val = 0;
    else if (desc_.calib_address != 0 && address == desc_.calib_address) val = calib_;
    else if (address == desc_.res_address) {
        val = static_cast<u8>(res_);
        res_latch_ = res_;
        res_latch_valid_ = true;
        intflags_ &= ~0x01U;
        sync_bus_data();
    }
    else if (address == desc_.res_address + 1) val = static_cast<u8>((res_latch_valid_ ? res_latch_ : res_) >> 8);
    else if (address == desc_.winlt_address) val = static_cast<u8>(winlt_);
    else if (address == desc_.winlt_address + 1) val = static_cast<u8>(winlt_ >> 8);
    else if (address == desc_.winht_address) val = static_cast<u8>(winht_);
    else if (address == desc_.winht_address + 1) val = static_cast<u8>(winht_ >> 8);
    
    // Status at 0x01? (Usually STATUS is addr+1 of CTRLA but varies)
    // ATmega4809: ADC.STATUS is at 0x60D
    if (desc_.ctrla_address != 0 && address == desc_.ctrla_address + 13) { // STATUS is usually +13 from CTRLA in ATmega4809
       if (converting_) val |= 0x01U; // ADCBUSY 
    }

    return val;
}

void Adc8x::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value;
    } else if (address == desc_.ctrlb_address) {
        ctrlb_ = value;
    } else if (address == desc_.ctrlc_address) {
        ctrlc_ = value;
    } else if (address == desc_.ctrld_address) {
        ctrld_ = value;
    } else if (address == desc_.ctrle_address) {
        ctrle_ = value;
    } else if (address == desc_.sampctrl_address) {
        sampctrl_ = value;
    } else if (address == desc_.muxpos_address) {
        muxpos_ = value;
    } else if (address == desc_.muxneg_address) {
        muxneg_ = value;
    } else if (address == desc_.command_address) {
        // debug removed
        if (value & 0x01U) start_conversion();
    } else if (address == desc_.evctrl_address) {
        evctrl_ = value;
    } else if (address == desc_.intctrl_address) {
        intctrl_ = value;
    } else if (address == desc_.intflags_address) {
        intflags_ &= ~value; // Clear on write 1
        sync_bus_data();
    } else if (address == desc_.dbgctrl_address) {
        dbgctrl_ = value;
    } else if (address == desc_.winlt_address) {
        winlt_ = (winlt_ & 0xFF00U) | value;
    } else if (address == desc_.winlt_address + 1) {
        winlt_ = (winlt_ & 0x00FFU) | (static_cast<u16>(value) << 8);
    } else if (address == desc_.winht_address) {
        winht_ = (winht_ & 0xFF00U) | value;
    } else if (address == desc_.winht_address + 1) {
        winht_ = (winht_ & 0x00FFU) | (static_cast<u16>(value) << 8);
    } else if (desc_.calib_address != 0 && address == desc_.calib_address) {
        calib_ = value;
    }
}

bool Adc8x::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if ((intctrl_ & 0x01U) && (intflags_ & 0x01U)) { // RESRDY
        request.vector_index = desc_.res_ready_vector_index;
        return true;
    }
    if ((intctrl_ & 0x02U) && (intflags_ & 0x02U)) { // WCOMP
        request.vector_index = desc_.res_ready_vector_index; // Often share same vector or close by
        return true;
    }
    return false;
}

bool Adc8x::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (!pending_interrupt_request(request)) return false;
    // Clear only the interrupt flag that was actually consumed
    if (intflags_ & 0x01U) {
        intflags_ &= ~0x01U;  // RESRDY
    } else {
        intflags_ &= ~0x02U;  // WCOMP
    }
    sync_bus_data();
    update_interrupt_state();
    return true;
}

bool Adc8x::is_enabled() const noexcept {
    return (ctrla_ & 0x01U);
}

u16 Adc8x::get_prescaler() const noexcept {
    u8 p = (ctrlb_ >> 4) & 0x07U;
    return 2U << p; // 0->2, 1->4, ..., 7->256
}

u32 Adc8x::get_sample_count() const noexcept {
    u8 sampnum = ctrlb_ & 0x07U;
    return 1U << sampnum;
}

void Adc8x::start_conversion() noexcept {
    // debug removed
    if (converting_) return;
    converting_ = true;
    state_ = AdcPhase::startup;
    cycles_in_phase_ = 2; // Startup is 2 cycles
    current_sample_ = 0;
    accumulated_res_ = 0;
}

void Adc8x::complete_conversion() noexcept {
    // This is called AFTER conversion phase ticks complete
    double input_voltage = 0.0;
    if (signal_bank_) {
        // Simple mapping: AIN0-AIN15 map to bank channels 0-15
        if (muxpos_ < AnalogSignalBank::kChannelCount) {
            input_voltage = signal_bank_->voltage(muxpos_);
        }
    }

    // Reference Voltage logic
    double vref = vdd_; // Default VDD
    u8 vrefsel = (ctrlc_ & 0x30U) >> 4U;
    if (vrefsel == 0x00U) {
        vref = 1.1; 
    } else if (vrefsel == 0x01U) {
        vref = vdd_; // VDD
    } else if (vrefsel == 0x02U) {
        vref = 2.5;
    }

    // Calculate 10-bit result
    u32 raw_result = static_cast<u32>((input_voltage / vref) * 1024.0);
    if (raw_result > 1023) raw_result = 1023;

    bool eight_bit = (ctrla_ & 0x04U);
    if (eight_bit) {
        raw_result >>= 2U;
    }

    accumulated_res_ += raw_result;
    // debug trace removed
}

void Adc8x::process_sample_result(u32 raw_accumulated) noexcept {
    // Right-shift by SAMPNUM to undo hardware oversampling
    u8 sampnum = ctrlb_ & 0x07U;
    u16 result = static_cast<u16>(raw_accumulated >> sampnum);
    res_ = result;
    res_latch_valid_ = false;
    intflags_ |= 0x01U; // RESRDY
    // debug trace removed
    
    // Window Comparator Logic (WINCM in CTRLE)
    u8 wmode = ctrle_ & 0x07U;
    bool wcomp = false;
    if (wmode == 0x01U) wcomp = (result < winlt_); // BELOW
    else if (wmode == 0x02U) wcomp = (result > winht_); // ABOVE
    else if (wmode == 0x03U) wcomp = (result >= winlt_ && result <= winht_); // INSIDE
    else if (wmode == 0x04U) wcomp = (result < winlt_ || result > winht_); // OUTSIDE
    
    if (wcomp) {
        intflags_ |= 0x02U; // WCOMP
    }

    if (evsys_ && desc_.resrd_generator_id != 0) {
        evsys_->trigger_event(desc_.resrd_generator_id);
    }

    sync_bus_data();
    update_interrupt_state();
}

void Adc8x::sync_bus_data() noexcept {
    if (bus_ == nullptr) return;
    auto sp = bus_->data_space();
    if (desc_.intflags_address != 0) {
        sp[desc_.intflags_address] = intflags_;
    }
    if (desc_.res_address != 0) {
        sp[desc_.res_address] = static_cast<u8>(res_ & 0xFF);
        sp[desc_.res_address + 1] = static_cast<u8>((res_ >> 8) & 0xFF);
    }
}

void Adc8x::update_interrupt_state() noexcept {
    InterruptRequest req;
    set_interrupt_pending(pending_interrupt_request(req));
}

} // namespace vioavr::core
