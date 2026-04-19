#include "vioavr/core/tcb.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/evsys.hpp"
#include <algorithm>

namespace vioavr::core {

Tcb::Tcb(const TcbDescriptor& desc) : desc_(desc) {
    if (desc_.ctrla_address != 0U) {
        std::vector<u16> addrs = {
            desc_.ctrla_address, desc_.ctrlb_address, desc_.evctrl_address,
            desc_.intctrl_address, desc_.intflags_address, desc_.status_address,
            desc_.dbgctrl_address, desc_.temp_address,
            desc_.cnt_address, static_cast<u16>(desc_.cnt_address + 1),
            desc_.ccmp_address, static_cast<u16>(desc_.ccmp_address + 1)
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

void Tcb::reset() noexcept {
    ctrla_ = 0;
    ctrlb_ = 0;
    evctrl_ = 0;
    intctrl_ = 0;
    intflags_ = 0;
    status_ = 0;
    dbgctrl_ = 0;
    temp_ = 0;
    cnt_ = 0;
    ccmp_ = 0;
}

std::span<const AddressRange> Tcb::mapped_ranges() const noexcept {
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

u8 Tcb::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.ctrlb_address) return ctrlb_;
    if (address == desc_.evctrl_address) return evctrl_;
    if (address == desc_.intctrl_address) return intctrl_;
    if (address == desc_.intflags_address) return intflags_;
    if (address == desc_.status_address) return status_;
    if (address == desc_.dbgctrl_address) return dbgctrl_;
    if (address == desc_.temp_address) return temp_;

    if (address == desc_.cnt_address) return static_cast<u8>(cnt_ & 0xFFU);
    if (address == desc_.cnt_address + 1) return static_cast<u8>((cnt_ >> 8U) & 0xFFU);
    
    if (address == desc_.ccmp_address) return static_cast<u8>(ccmp_ & 0xFFU);
    if (address == desc_.ccmp_address + 1) return static_cast<u8>((ccmp_ >> 8U) & 0xFFU);

    return 0x00;
}

void Tcb::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value;
    } else if (address == desc_.ctrlb_address) {
        ctrlb_ = value;
    } else if (address == desc_.evctrl_address) {
        evctrl_ = value;
    } else if (address == desc_.intctrl_address) {
        intctrl_ = value;
    } else if (address == desc_.intflags_address) {
        intflags_ &= ~value;
    } else if (address == desc_.cnt_address) {
        cnt_ = (cnt_ & 0xFF00U) | value;
    } else if (address == desc_.cnt_address + 1) {
        cnt_ = (cnt_ & 0x00FFU) | (static_cast<u16>(value) << 8U);
    } else if (address == desc_.ccmp_address) {
        ccmp_ = (ccmp_ & 0xFF00U) | value;
    } else if (address == desc_.ccmp_address + 1) {
        ccmp_ = (ccmp_ & 0x00FFU) | (static_cast<u16>(value) << 8U);
    }
}

void Tcb::tick(u64 elapsed_cycles) noexcept {
    if (!is_enabled()) return;
    
    for (u64 i = 0; i < elapsed_cycles; ++i) {
        handle_matches();
        perform_tick();
    }
}

void Tcb::handle_matches() {
    // Basic periodic interrupt (Mode 0)
    if (get_mode() == 0 && cnt_ == ccmp_) {
        intflags_ |= 0x01; // CAPT
    }
}

void Tcb::perform_tick() {
    u8 mode = get_mode();
    if (mode == 0) { // Periodic Interrupt
        if (cnt_ == ccmp_) {
            cnt_ = 0;
        } else {
            cnt_++;
        }
    } else if (mode == 7) { // Single-shot
        if (cnt_ == ccmp_) {
            ctrla_ &= ~0x01; // Disable
            intflags_ |= 0x01;
        } else {
            cnt_++;
        }
    } else {
        // Capture/Measurement modes: counter usually free-runs or is reset by event
        cnt_++;
    }
}

void Tcb::set_event_system(EventSystem* evsys) noexcept {
    evsys_ = evsys;
    if (evsys_ && desc_.user_event_address != 0) {
        // Calculate user index from address relative to EVSYS users base
        u8 user_index = static_cast<u8>(desc_.user_event_address - evsys_->users_base());
        evsys_->register_user_callback(user_index, [this]() { on_event(); });
    }
}

void Tcb::on_event() noexcept {
    if (!is_enabled()) return;
    if (!(evctrl_ & 0x01)) return; // CAPTEI

    u8 mode = get_mode();
    if (mode == 2 || mode == 3) { // Capture or Frequency Measurement
        ccmp_ = cnt_;
        intflags_ |= 0x01; // CAPT
        if (mode == 3) {
            cnt_ = 0; // Reset on capture
        }
    }
}

bool Tcb::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if ((intflags_ & 0x01) && (intctrl_ & 0x01)) {
        request.vector_index = desc_.vector_index;
        return true;
    }
    return false;
}

bool Tcb::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (pending_interrupt_request(request)) {
        intflags_ &= ~0x01;
        return true;
    }
    return false;
}

} // namespace vioavr::core
