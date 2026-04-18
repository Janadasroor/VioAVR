#include "vioavr/core/adc8x.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/evsys.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/logger.hpp"
#include <algorithm>
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
            evsys_->register_user_callback(user_index, [this]() {
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
    muxpos_ = 0;
    muxneg_ = 0;
    command_ = 0;
    evctrl_ = 0;
    intctrl_ = 0;
    intflags_ = 0;
    dbgctrl_ = 0;
    res_ = 0;
    winlt_ = 0;
    winht_ = 0;
    converting_ = false;
    cycles_remaining_ = 0;
}

void Adc8x::tick(u64 elapsed_cycles) noexcept {
    if (!is_enabled() || !converting_) return;

    if (elapsed_cycles >= cycles_remaining_) {
        complete_conversion();
    } else {
        cycles_remaining_ -= elapsed_cycles;
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
    else if (address == desc_.command_address) val = 0; // Write-only/strobe
    else if (address == desc_.evctrl_address) val = evctrl_;
    else if (address == desc_.intctrl_address) val = intctrl_;
    else if (address == desc_.intflags_address) val = intflags_;
    else if (address == desc_.dbgctrl_address) val = dbgctrl_;
    else if (address == desc_.res_address) val = static_cast<u8>(res_);
    else if (address == desc_.res_address + 1) val = static_cast<u8>(res_ >> 8);
    
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
        if (value & 0x01U) start_conversion();
    } else if (address == desc_.evctrl_address) {
        evctrl_ = value;
    } else if (address == desc_.intctrl_address) {
        intctrl_ = value;
    } else if (address == desc_.intflags_address) {
        intflags_ &= ~value; // Clear on write 1
    } else if (address == desc_.dbgctrl_address) {
        dbgctrl_ = value;
    } else if (address == desc_.res_address) {
        // RES usually read-only in conversion, but might be writable in some modes
    }
}

bool Adc8x::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if ((intctrl_ & 0x01U) && (intflags_ & 0x01U)) { // RESRDY
        request.vector_index = desc_.res_ready_vector_index;
        return true;
    }
    // WCOMP not implemented yet
    return false;
}

bool Adc8x::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (pending_interrupt_request(request)) {
        // Note: INTFLAGS for ADC are usually cleared by hardware when vector is called,
        // but often only if data is read? Actually, for modern AVR, you clear it by writing 1 or reading RES.
        return true; 
    }
    return false;
}

bool Adc8x::is_enabled() const noexcept {
    return (ctrla_ & 0x01U);
}

void Adc8x::start_conversion() noexcept {
    if (converting_) return;
    converting_ = true;
    cycles_remaining_ = 13; 
}

void Adc8x::complete_conversion() noexcept {
    converting_ = false;
    
    double voltage = 0.5; // Default mid-scale
    if (signal_bank_) {
        // Simple mapping: AIN0-AIN15 map to bank channels 0-15
        if (muxpos_ < AnalogSignalBank::kChannelCount) {
            voltage = signal_bank_->voltage(muxpos_);
        }
    }

    res_ = static_cast<u16>(voltage * 1023.0);
    intflags_ |= 0x01U; // RESRDY
    if (evsys_ && desc_.resrd_generator_id != 0) {
        evsys_->trigger_event(desc_.resrd_generator_id);
    }
}

} // namespace vioavr::core
