#include "vioavr/core/dac8x.hpp"

namespace vioavr::core {

Dac8x::Dac8x(const Dac8xDescriptor& desc) noexcept
    : desc_(desc) {
    if (desc_.ctrla_address != 0) {
        ranges_[0] = {desc_.ctrla_address, desc_.data_address};
    }
}

std::span<const AddressRange> Dac8x::mapped_ranges() const noexcept {
    if (desc_.ctrla_address != 0) return ranges_;
    return {};
}

void Dac8x::reset() noexcept {
    ctrla_ = 0;
    data_ = 0;
}

void Dac8x::tick(u64) noexcept {}

u8 Dac8x::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.data_address) return data_;
    return 0;
}

void Dac8x::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) ctrla_ = value;
    else if (address == desc_.data_address) data_ = value;
}

} // namespace vioavr::core
