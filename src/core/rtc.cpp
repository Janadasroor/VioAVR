#include "vioavr/core/rtc.hpp"
#include "vioavr/core/memory_bus.hpp"
#include <algorithm>

namespace vioavr::core {

Rtc::Rtc(const RtcDescriptor& desc) : desc_(desc) {
    if (desc_.ctrla_address != 0U) {
        std::vector<u16> addrs = {
            desc_.ctrla_address, desc_.status_address, desc_.intctrl_address,
            desc_.intflags_address, desc_.temp_address, desc_.dbgctrl_address,
            desc_.clksel_address,
            desc_.cnt_address, static_cast<u16>(desc_.cnt_address + 1),
            desc_.per_address, static_cast<u16>(desc_.per_address + 1),
            desc_.cmp_address, static_cast<u16>(desc_.cmp_address + 1),
            desc_.pitctrla_address, desc_.pitstatus_address,
            desc_.pitintctrl_address, desc_.pitintflags_address
        };
        std::sort(addrs.begin(), addrs.end());
        addrs.erase(std::unique(addrs.begin(), addrs.end()), addrs.end());
        
        size_t ri = 0;
        for (u16 addr : addrs) {
            if (addr == 0) continue;
            if (ri > 0 && addr == ranges_[ri-1].end + 1) {
                ranges_[ri-1].end = addr;
            } else if (ri < ranges_.size()) {
                ranges_[ri++] = {addr, addr};
            }
        }
    }
}

void Rtc::reset() noexcept {
    ctrla_ = 0; status_ = 0; intctrl_ = 0; intflags_ = 0;
    temp_ = 0; dbgctrl_ = 0; clksel_ = 0;
    cnt_ = 0; per_ = 0; cmp_ = 0;
    pitctrla_ = 0; pitstatus_ = 0; pitintctrl_ = 0; pitintflags_ = 0;
    internal_ticks_ = 0;
    pit_ticks_ = 0;
}

std::span<const AddressRange> Rtc::mapped_ranges() const noexcept {
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

u8 Rtc::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.status_address) return status_;
    if (address == desc_.intctrl_address) return intctrl_;
    if (address == desc_.intflags_address) return intflags_;
    if (address == desc_.clksel_address) return clksel_;
    if (address == desc_.cnt_address) return static_cast<u8>(cnt_ & 0xFFU);
    if (address == desc_.cnt_address + 1) return static_cast<u8>((cnt_ >> 8U) & 0xFFU);
    if (address == desc_.per_address) return static_cast<u8>(per_ & 0xFFU);
    if (address == desc_.per_address + 1) return static_cast<u8>((per_ >> 8U) & 0xFFU);
    if (address == desc_.cmp_address) return static_cast<u8>(cmp_ & 0xFFU);
    if (address == desc_.cmp_address + 1) return static_cast<u8>((cmp_ >> 8U) & 0xFFU);

    if (address == desc_.pitctrla_address) return pitctrla_;
    if (address == desc_.pitstatus_address) return pitstatus_;
    if (address == desc_.pitintctrl_address) return pitintctrl_;
    if (address == desc_.pitintflags_address) return pitintflags_;

    return 0;
}

void Rtc::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value;
    } else if (address == desc_.intctrl_address) {
        intctrl_ = value;
    } else if (address == desc_.intflags_address) {
        intflags_ &= ~value;
    } else if (address == desc_.cnt_address) {
        cnt_ = (cnt_ & 0xFF00U) | value;
    } else if (address == desc_.cnt_address + 1) {
        cnt_ = (cnt_ & 0x00FFU) | (static_cast<u16>(value) << 8U);
    } else if (address == desc_.per_address) {
        per_ = (per_ & 0xFF00U) | value;
    } else if (address == desc_.per_address + 1) {
        per_ = (per_ & 0x00FFU) | (static_cast<u16>(value) << 8U);
    } else if (address == desc_.cmp_address) {
        cmp_ = (cmp_ & 0xFF00U) | value;
    } else if (address == desc_.cmp_address + 1) {
        cmp_ = (cmp_ & 0x00FFU) | (static_cast<u16>(value) << 8U);
    } else if (address == desc_.pitctrla_address) {
        pitctrla_ = value;
    } else if (address == desc_.pitintctrl_address) {
        pitintctrl_ = value;
    } else if (address == desc_.pitintflags_address) {
        pitintflags_ &= ~value;
    }
}

void Rtc::tick(u64 elapsed_cycles) noexcept {
    // Note: In real chips, RTC runs at a much slower clock (e.g. 32kHz).
    // Here we tick it at CPU clock because of VioAVR simplification,
    // but the prescaler will make it behave slowly enough.
    
    if (is_rtc_enabled()) {
        u16 div = get_prescaler();
        for (u64 i = 0; i < elapsed_cycles; ++i) {
            internal_ticks_++;
            if (internal_ticks_ >= div) {
                internal_ticks_ = 0;
                handle_rtc_tick();
            }
        }
    }

    if (is_pit_enabled()) {
        u16 period = get_pit_period();
        for (u64 i = 0; i < elapsed_cycles; ++i) {
            pit_ticks_++;
            if (pit_ticks_ >= period) {
                pit_ticks_ = 0;
                handle_pit_tick();
            }
        }
    }
}

void Rtc::handle_rtc_tick() {
    cnt_++;
    if (cnt_ == per_) {
        intflags_ |= 0x01; // OVF
        cnt_ = 0;
    }
    if (cnt_ == cmp_) {
        intflags_ |= 0x02; // CMP
    }
}

void Rtc::handle_pit_tick() {
    pitintflags_ |= 0x01; // PIT
}

u16 Rtc::get_prescaler() const noexcept {
    u8 p = (ctrla_ >> 3) & 0x0F;
    if (p == 0) return 1;
    return 1U << p;
}

u16 Rtc::get_pit_period() const noexcept {
    // PIT period depends on CTRLA.PERIOD bits 3:6 usually
    u8 p = (pitctrla_ >> 3) & 0x0F;
    // Period = 2^(p+2)? No, ATmega4809 datasheet says bits 3-6
    // 0: OFF, 1: 4 cycles, 2: 8, ... 15: 32768
    if (p == 0) return 4; // Should be handled by is_pit_enabled but safe
    return 1U << (p + 2);
}

bool Rtc::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if ((intflags_ & intctrl_) & 0x03) {
        request.vector_index = desc_.ovf_vector_index;
        return true;
    }
    if ((pitintflags_ & 0x01) && (pitintctrl_ & 0x01)) {
        request.vector_index = desc_.pit_vector_index;
        return true;
    }
    return false;
}

bool Rtc::consume_interrupt_request(InterruptRequest& request) noexcept {
    if ((intflags_ & intctrl_) & 0x03) {
        request.vector_index = desc_.ovf_vector_index;
        intflags_ &= ~0x03; // Clear OLF/CMP? Actually usually only one.
        return true;
    }
    if ((pitintflags_ & 0x01) && (pitintctrl_ & 0x01)) {
        request.vector_index = desc_.pit_vector_index;
        pitintflags_ &= ~0x01;
        return true;
    }
    return false;
}

} // namespace vioavr::core
