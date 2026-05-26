#include "vioavr/core/cfd.hpp"

namespace vioavr::core {

namespace {
constexpr u8 XFDIF_MASK = 0x02U;
constexpr u8 XFDIE_MASK = 0x01U;
constexpr u8 RW_MASK   = 0x03U;
} // namespace

Cfd::Cfd(const CfdDescriptor& desc) noexcept : desc_(desc) {
    if (desc_.xfdcsr_address != 0) {
        ranges_[0] = {desc_.xfdcsr_address, desc_.xfdcsr_address};
    }
}

std::span<const AddressRange> Cfd::mapped_ranges() const noexcept {
    return {ranges_.data(), desc_.xfdcsr_address != 0 ? 1U : 0U};
}

void Cfd::tick(u64) noexcept {}

void Cfd::reset() noexcept {
    xfdcsr_ = 0;
    set_interrupt_pending(false);
}

u8 Cfd::read(u16 address) noexcept {
    if (address == desc_.xfdcsr_address) {
        return xfdcsr_;
    }
    return 0;
}

void Cfd::write(u16 address, u8 value) noexcept {
    if (address == desc_.xfdcsr_address) {
        // XFDIF is write-1-to-clear
        if (value & XFDIF_MASK) {
            xfdcsr_ &= ~XFDIF_MASK;
        }
        // XFDIE is normal read/write
        if (value & XFDIE_MASK) {
            xfdcsr_ |= XFDIE_MASK;
        } else {
            xfdcsr_ &= ~XFDIE_MASK;
        }
        update_interrupt_pending();
    }
}

bool Cfd::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if ((xfdcsr_ & XFDIF_MASK) && (xfdcsr_ & XFDIE_MASK)) {
        request.vector_index = desc_.vector_index;
        return true;
    }
    return false;
}

bool Cfd::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (pending_interrupt_request(request)) {
        xfdcsr_ &= ~XFDIF_MASK;
        update_interrupt_pending();
        return true;
    }
    return false;
}

void Cfd::trigger_failure() noexcept {
    xfdcsr_ |= XFDIF_MASK;
    update_interrupt_pending();
}

void Cfd::update_interrupt_pending() noexcept {
    InterruptRequest req;
    set_interrupt_pending(pending_interrupt_request(req));
}

} // namespace vioavr::core
