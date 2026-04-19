#include "vioavr/core/rtc.hpp"
#include "vioavr/core/evsys.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/logger.hpp"
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
    cnt_ = 0; per_ = 0xFFFF; cmp_ = 0;
    pitctrla_ = 0; pitstatus_ = 0; pitintctrl_ = 0; pitintflags_ = 0;
    internal_ticks_ = 0;
    pit_ticks_ = 0;
    sync_busy_cycles_rtc_ = 0;
    sync_busy_cycles_pit_ = 0;
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
    if (address == desc_.temp_address) return temp_;

    // 16-bit Read: Low byte first latches High byte into TEMP
    if (address == desc_.cnt_address) {
        temp_ = static_cast<u8>((cnt_ >> 8U) & 0xFFU);
        return static_cast<u8>(cnt_ & 0xFFU);
    }
    if (address == desc_.cnt_address + 1) return temp_;
    
    if (address == desc_.per_address) {
        temp_ = static_cast<u8>((per_ >> 8U) & 0xFFU);
        return static_cast<u8>(per_ & 0xFFU);
    }
    if (address == desc_.per_address + 1) return temp_;

    if (address == desc_.cmp_address) {
        temp_ = static_cast<u8>((cmp_ >> 8U) & 0xFFU);
        return static_cast<u8>(cmp_ & 0xFFU);
    }
    if (address == desc_.cmp_address + 1) return temp_;

    if (address == desc_.pitctrla_address) return pitctrla_;
    if (address == desc_.pitstatus_address) return pitstatus_;
    if (address == desc_.pitintctrl_address) return pitintctrl_;
    if (address == desc_.pitintflags_address) return pitintflags_;

    return 0;
}


void Rtc::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value;
        status_ |= 0x01; // CTRLABUSY
        sync_busy_cycles_rtc_ = 64;
    } else if (address == desc_.intctrl_address) {
        intctrl_ = value;
    } else if (address == desc_.intflags_address) {
        intflags_ &= ~value;
    } else if (address == desc_.temp_address) {
        temp_ = value;
    } else if (address == desc_.cnt_address + 1) {

        temp_ = value;
    } else if (address == desc_.cnt_address) {
        cnt_ = (static_cast<u16>(temp_) << 8U) | value;
        status_ |= 0x10; // CNTBUSY
        sync_busy_cycles_rtc_ = 64;
    } else if (address == desc_.per_address + 1) {
        temp_ = value;
    } else if (address == desc_.per_address) {
        per_ = (static_cast<u16>(temp_) << 8U) | value;
        status_ |= 0x02; // PERBUSY
        sync_busy_cycles_rtc_ = 64;

    } else if (address == desc_.cmp_address + 1) {
        temp_ = value;
    } else if (address == desc_.cmp_address) {
        cmp_ = (static_cast<u16>(temp_) << 8U) | value;
        status_ |= 0x08; // CMPBUSY
        sync_busy_cycles_rtc_ = 64;

    } else if (address == desc_.pitctrla_address) {
        pitctrla_ = value;
        pitstatus_ |= 0x01; // CTRLBUSY
        sync_busy_cycles_pit_ = 64;
    } else if (address == desc_.pitintctrl_address) {
        pitintctrl_ = value;
    } else if (address == desc_.pitintflags_address) {
        pitintflags_ &= ~value;
    }
}

void Rtc::tick(u64 elapsed_cycles) noexcept {
    if (elapsed_cycles == 0) return;

    if (sync_busy_cycles_rtc_ > 0) {
        if (elapsed_cycles >= sync_busy_cycles_rtc_) {
            sync_busy_cycles_rtc_ = 0;
            status_ = 0; // Clear busy bits
        } else {
            sync_busy_cycles_rtc_ -= static_cast<u32>(elapsed_cycles);
        }
    }

    if (sync_busy_cycles_pit_ > 0) {
        if (elapsed_cycles >= sync_busy_cycles_pit_) {
            sync_busy_cycles_pit_ = 0;
            pitstatus_ = 0;
        } else {
            sync_busy_cycles_pit_ -= static_cast<u32>(elapsed_cycles);
        }
    }

    if (!(ctrla_ & 0x01) && !(pitctrla_ & 0x01)) return;

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
        u32 period = get_pit_period();
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
    Logger::debug("RTC: tick cnt=" + std::to_string(cnt_) + " per=" + std::to_string(per_));
    if (cnt_ == per_) {
        cnt_ = 0;
        intflags_ |= 0x01; // OVF
        if (evsys_ && desc_.ovf_generator_id != 0) {
            evsys_->trigger_event(desc_.ovf_generator_id);
        }
    } else {
        cnt_++;
    }
    
    if (cnt_ == cmp_) {
        intflags_ |= 0x02; // CMP
        if (evsys_ && desc_.cmp_generator_id != 0) {
            evsys_->trigger_event(desc_.cmp_generator_id);
        }
    }
}

void Rtc::handle_pit_tick() {
    pitintflags_ |= 0x01; // PIT
}

u16 Rtc::get_prescaler() const noexcept {
    u8 p = (ctrla_ >> 3) & 0x0F;
    if (p == 0) return 1;
    if (p > 10) p = 10; // Max 1024
    return 1U << p;
}

u16 Rtc::get_pit_period() const noexcept {
    u8 p = (pitctrla_ >> 3) & 0x0F;
    if (p == 0) return 4;
    // Period = 2^(p+1)
    return 1U << (p + 1);
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
        intflags_ &= ~0x03;
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
