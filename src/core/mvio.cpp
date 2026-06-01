#include "vioavr/core/mvio.hpp"
#include "vioavr/core/memory_bus.hpp"

namespace vioavr::core {

Mvio::Mvio(const MvioDescriptor& desc) noexcept : desc_(desc) {
    if (desc_.intctrl_address != 0) {
        ranges_[0] = {desc_.intctrl_address, static_cast<u16>(desc_.intctrl_address + 2)};
    }
}

std::span<const AddressRange> Mvio::mapped_ranges() const noexcept {
    return {ranges_.data(), desc_.intctrl_address != 0 ? 1U : 0U};
}

void Mvio::reset() noexcept {
    intctrl_ = 0;
    intflags_ = 0;
    status_ = 0;
    int_pending_ = false;
    vddio2_ = 0.0;
    prev_vddio2_ok_ = false;
}

void Mvio::tick(u64) noexcept {
    if (desc_.intctrl_address == 0) return;
    bool ok = vddio2_ > 1.8;
    if (ok) status_ |= 0x01;
    else status_ &= ~0x01;

    if (ok != prev_vddio2_ok_) {
        prev_vddio2_ok_ = ok;
        if (ok && (intctrl_ & 0x02)) {
            intflags_ |= 0x02;
        } else if (!ok && (intctrl_ & 0x01)) {
            intflags_ |= 0x01;
        }
    }
    update_interrupt_pending();
}

u8 Mvio::read(u16 address) noexcept {
    u16 offset = address - desc_.intctrl_address;
    switch (offset) {
        case 0: return intctrl_;
        case 1: return intflags_;
        case 2: return status_;
        default: return 0;
    }
}

void Mvio::write(u16 address, u8 value) noexcept {
    u16 offset = address - desc_.intctrl_address;
    switch (offset) {
        case 0: intctrl_ = value & 0x03; break;
        case 1:
            intflags_ &= ~(value & 0x03);
            if (!(intflags_ & 0x03)) int_pending_ = false;
            break;
        case 2: status_ = (status_ & 0x01) | (value & 0xFE); break;
    }
}

void Mvio::update_interrupt_pending() noexcept {
    int_pending_ = (intflags_ & intctrl_) != 0;
}

bool Mvio::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (int_pending_ && desc_.vector_index != 0xFF) {
        request.vector_index = desc_.vector_index;
        return true;
    }
    return false;
}

bool Mvio::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (int_pending_ && desc_.vector_index != 0xFF) {
        request.vector_index = desc_.vector_index;
        int_pending_ = false;
        return true;
    }
    return false;
}

} // namespace vioavr::core
