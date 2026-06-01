#include "vioavr/core/cfd.hpp"
#include "vioavr/core/memory_bus.hpp"

namespace vioavr::core {

Cfd::Cfd(const CfdDescriptor& desc) noexcept : desc_(desc) {
    if (desc_.xfdcsr_address != 0) {
        ranges_[0] = {desc_.xfdcsr_address, static_cast<u16>(desc_.xfdcsr_address + 3)};
    }
}

std::span<const AddressRange> Cfd::mapped_ranges() const noexcept {
    return {ranges_.data(), desc_.xfdcsr_address != 0 ? 1U : 0U};
}

void Cfd::reset() noexcept {
    xfdcsr_ = 0;
    clkdiv_ = 0;
    intflags_ = 0;
    intctrl_ = 0;
    int_pending_ = false;
    edge_counter_ = 0;
}

void Cfd::on_edge() noexcept {
    edge_counter_ = 0;
    intflags_ &= ~0x02;
    update_interrupt_pending();
}

void Cfd::tick(u64) noexcept {
    if (!(xfdcsr_ & 0x01)) return;
    u64 limit = 1024; // default: failure after ~1024 cycles
    if (xfdcsr_ & 0x40) limit = 4096;
    else if (xfdcsr_ & 0x20) limit = 256;
    else if (xfdcsr_ & 0x10) limit = 128;
    edge_counter_++;
    if (edge_counter_ >= limit) {
        intflags_ |= 0x02;
        update_interrupt_pending();
    }
}

u8 Cfd::read(u16 address) noexcept {
    u16 offset = address - desc_.xfdcsr_address;
    switch (offset) {
        case 0: return xfdcsr_;
        case 1: return clkdiv_;
        case 2: return intflags_;
        case 3: return intctrl_;
        default: return 0;
    }
}

void Cfd::write(u16 address, u8 value) noexcept {
    u16 offset = address - desc_.xfdcsr_address;
    switch (offset) {
        case 0: xfdcsr_ = (xfdcsr_ & 0x02) | (value & 0xFD); break;
        case 1: clkdiv_ = value; break;
        case 2:
            intflags_ &= ~(value & 0x03);
            if (!(intflags_ & 0x03)) int_pending_ = false;
            break;
        case 3: intctrl_ = value & 0x03; break;
    }
}

void Cfd::update_interrupt_pending() noexcept {
    int_pending_ = (intflags_ & intctrl_) != 0;
}

bool Cfd::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (int_pending_ && desc_.vector_index != 0xFF) {
        request.vector_index = desc_.vector_index;
        return true;
    }
    return false;
}

bool Cfd::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (int_pending_ && desc_.vector_index != 0xFF) {
        request.vector_index = desc_.vector_index;
        int_pending_ = false;
        return true;
    }
    return false;
}

} // namespace vioavr::core
