#include "vioavr/core/usb8x.hpp"

namespace vioavr::core {

Usb8x::Usb8x(const Usb8xDescriptor& desc) noexcept
    : desc_(desc) {
    if (desc_.ctrla_address != 0 && desc_.pllcsr_address != 0) {
        ranges_[0] = {desc_.ctrla_address, desc_.pllcsr_address};
    }
}

std::span<const AddressRange> Usb8x::mapped_ranges() const noexcept {
    if (desc_.ctrla_address != 0) return ranges_;
    return {};
}

void Usb8x::reset() noexcept {
    ctrla_ = 0;
    ctrlb_ = 0;
    busstate_ = 0;
    addr_ = 0;
    fifowp_ = 0;
    fiforp_ = 0;
    epptr_ = 0;
    intctrla_ = 0;
    intctrlb_ = 0;
    intflagsa_ = 0;
    intflagsb_ = 0;
    pllcsr_ = 0;
}

void Usb8x::tick(u64) noexcept {}

u8 Usb8x::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.ctrlb_address) return ctrlb_;
    if (address == desc_.busstate_address) return busstate_;
    if (address == desc_.addr_address) return addr_;
    if (address == desc_.fifowp_address) return fifowp_;
    if (address == desc_.fiforp_address) return fiforp_;
    if (address == desc_.epptr_address) return epptr_;
    if (address == desc_.intctrla_address) return intctrla_;
    if (address == desc_.intctrlb_address) return intctrlb_;
    if (address == desc_.intflagsa_address) return intflagsa_;
    if (address == desc_.intflagsb_address) return intflagsb_;
    if (address == desc_.pllcsr_address) return pllcsr_;
    return 0;
}

void Usb8x::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) ctrla_ = value;
    else if (address == desc_.ctrlb_address) ctrlb_ = value;
    else if (address == desc_.busstate_address) busstate_ = value;
    else if (address == desc_.addr_address) addr_ = value;
    else if (address == desc_.fifowp_address) fifowp_ = value;
    else if (address == desc_.fiforp_address) fiforp_ = value;
    else if (address == desc_.epptr_address) epptr_ = value;
    else if (address == desc_.intctrla_address) intctrla_ = value;
    else if (address == desc_.intctrlb_address) intctrlb_ = value;
    else if (address == desc_.intflagsa_address) intflagsa_ = value;
    else if (address == desc_.intflagsb_address) intflagsb_ = value;
    else if (address == desc_.pllcsr_address) pllcsr_ = value;
}

} // namespace vioavr::core
