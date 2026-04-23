#include "vioavr/core/zcd.hpp"

namespace vioavr::core {

Zcd::Zcd(const ZcdDescriptor& desc) noexcept : desc_(desc) {
    if (desc_.ctrla_address != 0) {
        ranges_[0] = {desc_.ctrla_address, desc_.ctrla_address};
    }
}

std::span<const AddressRange> Zcd::mapped_ranges() const noexcept {
    return {ranges_.data(), desc_.ctrla_address != 0 ? 1U : 0U};
}

void Zcd::reset() noexcept {
    ctrla_ = 0;
    int_pending_ = false;
}

void Zcd::tick(u64) noexcept {}

u8 Zcd::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) {
        u8 val = ctrla_ & 0x7FU;
        if (pin_state_) val |= 0x80U; // STATE bit
        return val;
    }
    return 0;
}

void Zcd::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value & 0x7FU; // Bit 7 (STATE) is read-only
    }
}

bool Zcd::on_external_pin_change(u16 pin_address, u8 bit_index, PinLevel level) noexcept {
    if (pin_address != desc_.pin_address) return false;
    if (desc_.pin_bit != 0xFFU && bit_index != desc_.pin_bit) return false;
    
    bool new_state = (level == PinLevel::high);
    if (new_state == pin_state_) return false;
    
    // We crossed zero!
    pin_state_ = new_state;
    
    if (!(ctrla_ & 0x01U)) return true; // ENABLE bit is off, but pin state updated
    
    // ZCD typically triggers on crossing.
    int_pending_ = true;
    return true;
}

bool Zcd::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (int_pending_ && (ctrla_ & 0x01U)) {
        request.vector_index = desc_.vector_index;
        return true;
    }
    return false;
}

bool Zcd::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (pending_interrupt_request(request)) {
        int_pending_ = false;
        return true;
    }
    return false;
}

} // namespace vioavr::core
