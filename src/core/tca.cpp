#include "vioavr/core/tca.hpp"
#include "vioavr/core/evsys.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/logger.hpp"
#include <algorithm>

namespace vioavr::core {

Tca::Tca(const TcaDescriptor& desc) : desc_(desc) {
    if (desc_.ctrla_address != 0U) {
        std::vector<u16> addrs = {
            desc_.ctrla_address, desc_.ctrlb_address, desc_.ctrlc_address,
            desc_.ctrld_address, desc_.ctrleclr_address, desc_.ctrleset_address,
            desc_.ctrlfclr_address, desc_.ctrlfset_address, desc_.evctrl_address,
            desc_.intctrl_address, desc_.intflags_address, desc_.dbgctrl_address,
            desc_.temp_address,
            desc_.tcnt_address, static_cast<u16>(desc_.tcnt_address + 1),
            desc_.period_address, static_cast<u16>(desc_.period_address + 1),
            desc_.cmp0_address, static_cast<u16>(desc_.cmp0_address + 1),
            desc_.cmp1_address, static_cast<u16>(desc_.cmp1_address + 1),
            desc_.cmp2_address, static_cast<u16>(desc_.cmp2_address + 1)
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

void Tca::reset() noexcept {
    ctrla_ = 0;
    ctrlb_ = 0;
    ctrlc_ = 0;
    ctrld_ = 0;
    evctrl_ = 0;
    intctrl_ = 0;
    intflags_ = 0;
    dbgctrl_ = 0;
    temp_ = 0;
    tcnt_ = 0;
    period_ = 0xFFFF;
    cmp0_ = 0;
    cmp1_ = 0;
    cmp2_ = 0;
    counting_up_ = true;
    prescaler_counter_ = 0;
    prescaler_limit_ = 1;
}

std::span<const AddressRange> Tca::mapped_ranges() const noexcept {
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

void Tca::set_event_system(EventSystem* evsys) noexcept {
    evsys_ = evsys;
    if (evsys_ && desc_.user_event_address != 0) {
        u16 base = evsys_->users_base();
        if (desc_.user_event_address >= base) {
            u8 idx = static_cast<u8>(desc_.user_event_address - base);
            evsys_->register_user_callback(idx, [this]() {
                this->on_event();
            });
        }
    }
}

void Tca::on_event() noexcept {
    if (is_enabled()) {
        perform_tick();
    }
}

u8 Tca::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.ctrlb_address) return ctrlb_;
    if (address == desc_.ctrlc_address) return ctrlc_;
    if (address == desc_.ctrld_address) return ctrld_;
    if (address == desc_.evctrl_address) return evctrl_;
    if (address == desc_.intctrl_address) return intctrl_;
    if (address == desc_.intflags_address) return intflags_;
    if (address == desc_.dbgctrl_address) return dbgctrl_;
    if (address == desc_.temp_address) return temp_;

    // 16-bit registers with TEMP buffer
    if (address == desc_.tcnt_address) return static_cast<u8>(tcnt_ & 0xFFU);
    if (address == desc_.tcnt_address + 1) return static_cast<u8>((tcnt_ >> 8U) & 0xFFU);
    
    if (address == desc_.period_address) return static_cast<u8>(period_ & 0xFFU);
    if (address == desc_.period_address + 1) return static_cast<u8>((period_ >> 8U) & 0xFFU);

    if (address == desc_.cmp0_address) return static_cast<u8>(cmp0_ & 0xFFU);
    if (address == desc_.cmp0_address + 1) return static_cast<u8>((cmp0_ >> 8U) & 0xFFU);
    
    if (address == desc_.cmp1_address) return static_cast<u8>(cmp1_ & 0xFFU);
    if (address == desc_.cmp1_address + 1) return static_cast<u8>((cmp1_ >> 8U) & 0xFFU);
    
    if (address == desc_.cmp2_address) return static_cast<u8>(cmp2_ & 0xFFU);
    if (address == desc_.cmp2_address + 1) return static_cast<u8>((cmp2_ >> 8U) & 0xFFU);

    return 0x00;
}

void Tca::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value;
        update_prescaler();
    } else if (address == desc_.ctrlb_address) {
        ctrlb_ = value;
    } else if (address == desc_.ctrlc_address) {
        ctrlc_ = value;
    } else if (address == desc_.ctrld_address) {
        ctrld_ = value;
    } else if (address == desc_.intctrl_address) {
        intctrl_ = value;
    } else if (address == desc_.intflags_address) {
        intflags_ &= ~value; // Clear on write 1
    } else if (address == desc_.tcnt_address) {
        tcnt_ = (tcnt_ & 0xFF00U) | value;
    } else if (address == desc_.tcnt_address + 1) {
        tcnt_ = (tcnt_ & 0x00FFU) | (static_cast<u16>(value) << 8U);
    } else if (address == desc_.period_address) {
        period_ = (period_ & 0xFF00U) | value;
    } else if (address == desc_.period_address + 1) {
        period_ = (period_ & 0x00FFU) | (static_cast<u16>(value) << 8U);
    } else if (address == desc_.cmp0_address) {
        cmp0_ = (cmp0_ & 0xFF00U) | value;
    } else if (address == desc_.cmp0_address + 1) {
        cmp0_ = (cmp0_ & 0x00FFU) | (static_cast<u16>(value) << 8U);
    } else if (address == desc_.cmp1_address) {
        cmp1_ = (cmp1_ & 0xFF00U) | value;
    } else if (address == desc_.cmp1_address + 1) {
        cmp1_ = (cmp1_ & 0x00FFU) | (static_cast<u16>(value) << 8U);
    } else if (address == desc_.cmp2_address) {
        cmp2_ = (cmp2_ & 0xFF00U) | value;
    } else if (address == desc_.cmp2_address + 1) {
        cmp2_ = (cmp2_ & 0x00FFU) | (static_cast<u16>(value) << 8U);
    }
}

void Tca::tick(u64 elapsed_cycles) noexcept {
    if (elapsed_cycles == 0) return;
    if (!is_enabled()) return;
    
    for (u64 i = 0; i < elapsed_cycles; ++i) {
        if (++prescaler_counter_ >= prescaler_limit_) {
            prescaler_counter_ = 0;
            if (is_split_mode()) {
                perform_tick_split();
            } else {
                perform_tick();
            }
        }
    }
}

void Tca::handle_matches() {
    u8 wgmode = ctrlb_ & 0x07;
    
    // Overflow / Underflow
    if (wgmode == 0x00 || wgmode == 0x03) { // Normal / Single Slope
        if (tcnt_ == period_) {
            intflags_ |= 0x01; // OVF
            if (evsys_) evsys_->trigger_event(128); // TCA0_OVF
        }
    } else if (wgmode >= 0x05) { // Dual Slope
        if (tcnt_ == 0 && !counting_up_) { // BOTTOM
             intflags_ |= 0x01;
             if (evsys_) evsys_->trigger_event(128);
        }
    }

    // Compare matches
    if (tcnt_ == cmp0_) intflags_ |= 0x10;
    if (tcnt_ == cmp1_) intflags_ |= 0x20;
    if (tcnt_ == cmp2_) intflags_ |= 0x40;
}

bool Tca::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (!is_enabled()) return false;
    
    if ((intflags_ & 0x01) && (intctrl_ & 0x01)) {
        request.vector_index = desc_.luf_ovf_vector_index;
        return true;
    }
    if ((intflags_ & 0x10) && (intctrl_ & 0x10)) {
        request.vector_index = desc_.cmp0_vector_index;
        return true;
    }
    if ((intflags_ & 0x20) && (intctrl_ & 0x20)) {
        request.vector_index = desc_.cmp1_vector_index;
        return true;
    }
    if ((intflags_ & 0x40) && (intctrl_ & 0x40)) {
        request.vector_index = desc_.cmp2_vector_index;
        return true;
    }
    return false;
}

bool Tca::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (pending_interrupt_request(request)) {
        return true;
    }
    return false;
}

void Tca::perform_tick() {
    u8 wgmode = ctrlb_ & 0x07;
    
    if (wgmode == 0x01) { // FRQ
        if (tcnt_ >= cmp0_) {
            tcnt_ = 0;
        } else {
            tcnt_++;
        }
        return;
    }

    if (wgmode >= 0x05) { // DUALSLOPE
        if (counting_up_) {
            if (tcnt_ >= period_) {
                counting_up_ = false;
                if (tcnt_ > 0) tcnt_--;
            } else {
                tcnt_++;
            }
        } else {
            if (tcnt_ == 0) {
                counting_up_ = true;
                tcnt_++;
            } else {
                tcnt_--;
            }
        }
        return;
    }

    if (tcnt_ >= period_) {
        tcnt_ = 0;
    } else {
        tcnt_++;
    }
}

void Tca::perform_tick_split() {
    // Low byte
    if (split_.cnt_l >= split_.per_l) {
        split_.cnt_l = 0;
        intflags_ |= 0x01; // LUNF
    } else {
        split_.cnt_l++;
    }

    // High byte
    if (split_.cnt_h >= split_.per_h) {
        split_.cnt_h = 0;
        intflags_ |= 0x10; // HUNF
    } else {
        split_.cnt_h++;
    }
}

void Tca::update_prescaler() noexcept {
    u8 clksel = (ctrla_ >> 1) & 0x07;
    static constexpr u16 div[] = {1, 2, 4, 8, 16, 64, 256, 1024};
    prescaler_limit_ = div[clksel];
    prescaler_counter_ = 0;
}

} // namespace vioavr::core
