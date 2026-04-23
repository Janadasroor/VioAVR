#include "vioavr/core/lin.hpp"

namespace vioavr::core {

LinUART::LinUART(const LinDescriptor& desc) noexcept : desc_(desc) {
    if (desc_.ctrla_address != 0) {
        // LIN usually spans 8 bytes (0xC8 to 0xCF)
        ranges_[0] = {desc_.ctrla_address, static_cast<u16>(desc_.ctrla_address + 7)};
    }
}

std::span<const AddressRange> LinUART::mapped_ranges() const noexcept {
    return {ranges_.data(), desc_.ctrla_address != 0 ? 1U : 0U};
}

void LinUART::reset() noexcept {
    lincr_ = 0;
    linsir_ = 0;
    linenir_ = 0;
    linerr_ = 0;
    linbtr_ = 0;
    linidr_ = 0;
    lindat_ = 0;
}

void LinUART::tick(u64) noexcept {}

u8 LinUART::read(u16 address) noexcept {
    u16 offset = address - desc_.ctrla_address;
    switch (offset) {
        case 0: return lincr_;
        case 1: return linsir_;
        case 2: return linenir_;
        case 3: return linerr_;
        case 4: return linbtr_;
        case 6: return linidr_;
        case 7: return lindat_;
        default: return 0;
    }
}

void LinUART::write(u16 address, u8 value) noexcept {
    u16 offset = address - desc_.ctrla_address;
    switch (offset) {
        case 0: lincr_ = value; break;
        case 1: linsir_ = value; break;
        case 2: linenir_ = value; break;
        case 3: linerr_ = value; break;
        case 4: linbtr_ = value; break;
        case 6: linidr_ = value; break;
        case 7: lindat_ = value; break;
    }
}

bool LinUART::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if ((linsir_ & 0x0FU) & (linenir_ & 0x0FU)) {
        request.vector_index = desc_.vector_index;
        return true;
    }
    return false;
}

bool LinUART::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (pending_interrupt_request(request)) {
        // Bits in LINSIR are typically cleared by software writing 1 to them
        // or by hardware state changes. 
        return true;
    }
    return false;
}

} // namespace vioavr::core
