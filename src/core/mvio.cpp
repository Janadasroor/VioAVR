#include "vioavr/core/mvio.hpp"

namespace vioavr::core {

Mvio::Mvio(const MvioDescriptor& desc) noexcept
    : desc_(desc) {
    if (desc_.intctrl_address != 0 && desc_.status_address != 0) {
        ranges_[0] = {desc_.intctrl_address, desc_.status_address};
    }
}

std::span<const AddressRange> Mvio::mapped_ranges() const noexcept {
    if (desc_.intctrl_address != 0) return ranges_;
    return {};
}

void Mvio::reset() noexcept {
    intctrl_ = 0;
    intflags_ = 0;
    status_ = 0;
}

void Mvio::tick(u64) noexcept {}

u8 Mvio::read(u16 address) noexcept {
    if (address == desc_.intctrl_address) return intctrl_;
    if (address == desc_.intflags_address) return intflags_;
    if (address == desc_.status_address) return status_;
    return 0;
}

void Mvio::write(u16 address, u8 value) noexcept {
    if (address == desc_.intctrl_address) intctrl_ = value;
    else if (address == desc_.intflags_address) intflags_ = value;
    else if (address == desc_.status_address) status_ = value;
}

} // namespace vioavr::core
