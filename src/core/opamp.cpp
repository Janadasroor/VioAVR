#include "vioavr/core/opamp.hpp"

namespace vioavr::core {

Opamp::Opamp(const OpampDescriptor& desc) noexcept : desc_(desc) {
    if (desc_.ctrla_address != 0) {
        // OPAMP usually spans a few bytes
        ranges_[0] = {desc_.ctrla_address, static_cast<u16>(desc_.ctrla_address + 3)};
    }
}

std::span<const AddressRange> Opamp::mapped_ranges() const noexcept {
    return {ranges_.data(), desc_.ctrla_address != 0 ? 1U : 0U};
}

void Opamp::reset() noexcept {
    ctrla_ = 0;
    ctrlb_ = 0;
    resctrl_ = 0;
    muxctrl_ = 0;
}

void Opamp::tick(u64) noexcept {}

u8 Opamp::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.ctrlb_address) return ctrlb_;
    if (address == desc_.resctrl_address) return resctrl_;
    if (address == desc_.muxctrl_address) return muxctrl_;
    return 0;
}

void Opamp::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) ctrla_ = value;
    else if (address == desc_.ctrlb_address) ctrlb_ = value;
    else if (address == desc_.resctrl_address) resctrl_ = value;
    else if (address == desc_.muxctrl_address) muxctrl_ = value;
}

} // namespace vioavr::core
