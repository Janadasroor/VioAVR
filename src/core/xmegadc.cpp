#include "vioavr/core/xmegadc.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include <algorithm>
#include <vector>

namespace vioavr::core {

XmegaAdc::XmegaAdc(const XmegaAdcDescriptor& desc) noexcept : desc_(desc) {
    std::vector<u16> addrs;
    auto add_if = [&](u16 a) { if (a) addrs.push_back(a); };

    add_if(desc_.ctrla_address);
    add_if(desc_.ctrlb_address);
    add_if(desc_.refctrl_address);
    add_if(desc_.evctrl_address);
    add_if(desc_.prescaler_address);
    add_if(desc_.intflags_address);
    add_if(desc_.temp_address);
    add_if(desc_.sampctrl_address);
    add_if(desc_.cal_address);
    add_if(desc_.cal_address + 1);
    add_if(desc_.cal_address + 2);
    add_if(desc_.cal_address + 3);
    add_if(desc_.ch0res_address);
    add_if(desc_.ch0res_address + 1);
    add_if(desc_.cmp_address);
    add_if(desc_.cmp_address + 1);
    add_if(desc_.muxctrl_address);
    add_if(desc_.ch_intctrl_address);

    std::sort(addrs.begin(), addrs.end());
    addrs.erase(std::unique(addrs.begin(), addrs.end()), addrs.end());

    size_t count = 0;
    if (!addrs.empty()) {
        u16 start = addrs[0], end = addrs[0];
        for (size_t i = 1; i < addrs.size(); ++i) {
            if (addrs[i] == end + 1) { end = addrs[i]; }
            else {
                ranges_[count++] = {start, end};
                start = addrs[i]; end = addrs[i];
                if (count >= ranges_.size()) break;
            }
        }
        if (count < ranges_.size()) ranges_[count++] = {start, end};
    }
    num_ranges_ = count;
}

void XmegaAdc::reset() noexcept {
    ctrla_ = 0;
    ctrlb_ = 0;
    refctrl_ = 0;
    evctrl_ = 0;
    intflags_ = 0;
    prescaler_ = 0;
    temp_ = 0;
    sampctrl_ = 0;
    ch0res_ = 0;
    cmp_ = 0;
    ch0res_latch_ = 0;
    ch0res_latch_valid_ = false;
    state_ = AdcPhase::idle;
    fractional_cycles_ = 0;
    cycles_remaining_ = 0;
}

bool XmegaAdc::is_12bit() const noexcept {
    return (ctrla_ & 0x18U) == 0x00U || (ctrla_ & 0x18U) == 0x10U;
}

u32 XmegaAdc::conversion_cycles() const noexcept {
    return is_12bit() ? 21U : 13U;
}

u16 XmegaAdc::adc_cycle_divider() const noexcept {
    u8 div = prescaler_ & 0x07U;
    return static_cast<u16>(4U << div);
}

void XmegaAdc::start_conversion() noexcept {
    if (!is_enabled()) return;
    state_ = AdcPhase::converting;
    cycles_remaining_ = conversion_cycles();
}

void XmegaAdc::complete_conversion() noexcept {
    double vref = 5.0;
    switch (refctrl_ & 0x0FU) {
        case 0: vref = 1.0; break;    // INT1V
        case 1: vref = 0.256; break;  // INT256
        case 2: vref = 0.512; break;  // INT512
        case 3: vref = 5.0; break;    // VCC
        case 4: vref = 5.0; break;    // AREF
        case 5: vref = 5.0; break;    // DAC
        default: vref = 5.0; break;
    }

    double vin = 0.0;
    u8 muxpos = 0;
    if (desc_.muxctrl_address) {
        muxpos = ctrlb_;
    }

    if (signal_bank_) {
        vin = signal_bank_->voltage(muxpos);
    }

    u32 max_val = is_12bit() ? 4095U : 255U;
    u32 raw = static_cast<u32>(std::clamp(vin / vref * static_cast<double>(max_val), 0.0, static_cast<double>(max_val)));

    ch0res_ = static_cast<u16>(raw & max_val);

    intflags_ |= 0x01U;
    if (ch0res_ >= cmp_) {
        intflags_ |= 0x10U;
    }

    if (is_free_running()) {
        cycles_remaining_ = conversion_cycles();
    } else {
        state_ = AdcPhase::idle;
    }

    update_interrupt_state();
}

void XmegaAdc::tick(u64 elapsed_cycles) noexcept {
    if (!is_enabled() || state_ != AdcPhase::converting) return;

    u16 divider = adc_cycle_divider();
    u64 total = elapsed_cycles + fractional_cycles_;
    u64 adc_ticks = total / divider;
    fractional_cycles_ = total % divider;

    for (u64 t = 0; t < adc_ticks; ++t) {
        if (cycles_remaining_ > 0) cycles_remaining_--;
        if (cycles_remaining_ == 0) {
            complete_conversion();
            if (state_ != AdcPhase::converting) break;
        }
    }
}

u8 XmegaAdc::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.ctrlb_address || address == desc_.muxctrl_address) return ctrlb_;
    if (address == desc_.refctrl_address || address == desc_.ch_intctrl_address) return refctrl_;
    if (address == desc_.evctrl_address || address == desc_.intflags_address) return intflags_;
    if (address == desc_.prescaler_address) return prescaler_;
    if (address == desc_.temp_address) return temp_;
    if (address == desc_.sampctrl_address) return sampctrl_;

    // CAL (4 bytes)
    if (desc_.cal_address && address >= desc_.cal_address && address < desc_.cal_address + 4) {
        return 0xFF; // Default calibration
    }

    if (address == desc_.ch0res_address) {
        ch0res_latch_ = ch0res_;
        ch0res_latch_valid_ = true;
        return static_cast<u8>(ch0res_);
    }
    if (desc_.ch0res_address && address == desc_.ch0res_address + 1) {
        return static_cast<u8>((ch0res_latch_valid_ ? ch0res_latch_ : ch0res_) >> 8);
    }

    if (address == desc_.cmp_address) return static_cast<u8>(cmp_);
    if (desc_.cmp_address && address == desc_.cmp_address + 1) return static_cast<u8>(cmp_ >> 8);

    return 0;
}

void XmegaAdc::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value;
    } else if (address == desc_.ctrlb_address || address == desc_.muxctrl_address) {
        ctrlb_ = value;
    } else if (address == desc_.refctrl_address || address == desc_.ch_intctrl_address) {
        refctrl_ = value;
    } else if (address == desc_.evctrl_address || address == desc_.intflags_address) {
        evctrl_ = value;
    } else if (address == desc_.prescaler_address) {
        prescaler_ = value & 0x07U;
    } else if (address == desc_.temp_address) {
        temp_ = value;
    } else if (address == desc_.sampctrl_address) {
        sampctrl_ = value;
    } else if (address == desc_.cmp_address) {
        cmp_ = (cmp_ & 0xFF00) | value;
    } else if (desc_.cmp_address && address == desc_.cmp_address + 1) {
        cmp_ = static_cast<u16>((static_cast<u16>(value) << 8) | (cmp_ & 0x00FF));
    }

    // INTFLAGS write: write-1-to-clear at intflags_address
    if (address == desc_.intflags_address) {
        intflags_ &= ~value;
        update_interrupt_state();
    }

    // CTRLA: STCONV bit (bit 6) starts conversion on CH0
    if (address == desc_.ctrla_address && (value & 0x40U) && is_enabled()) {
        start_conversion();
    }
}

bool XmegaAdc::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (intflags_ & 0x01U) { // CH0IF
        request.vector_index = desc_.vector_index;
        return true;
    }
    if (intflags_ & 0x10U) { // CMPIF
        request.vector_index = desc_.vector_index;
        return true;
    }
    return false;
}

bool XmegaAdc::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (pending_interrupt_request(request)) {
        u8 consumed = 0;
        if (intflags_ & 0x01U) consumed |= 0x01U;
        if (intflags_ & 0x10U) consumed |= 0x10U;
        intflags_ &= ~consumed;
        update_interrupt_state();
        return true;
    }
    return false;
}

void XmegaAdc::update_interrupt_state() noexcept {
    set_interrupt_pending((intflags_ & 0x01U) || (intflags_ & 0x10U));
}

} // namespace vioavr::core
