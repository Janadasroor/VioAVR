#include "vioavr/core/nvm_ctrl.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/logger.hpp"
#include <algorithm>

namespace vioavr::core {

NvmCtrl::NvmCtrl(const NvmCtrlDescriptor& desc) : desc_(desc) {
    if (desc_.ctrla_address != 0U) {
        // Collect all registers and group into ranges
        std::vector<u16> addrs = {
            desc_.ctrla_address, desc_.ctrlb_address, desc_.status_address,
            desc_.intctrl_address, desc_.intflags_address,
            desc_.data_address, static_cast<u16>(desc_.data_address + 1),
            desc_.addr_address, static_cast<u16>(desc_.addr_address + 1)
        };
        std::sort(addrs.begin(), addrs.end());
        
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

std::span<const AddressRange> NvmCtrl::mapped_ranges() const noexcept {
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

u8 NvmCtrl::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.ctrlb_address) return ctrlb_;
    if (address == desc_.status_address) return status_;
    if (address == desc_.intctrl_address) return intctrl_;
    if (address == desc_.intflags_address) return intflags_;
    
    if (address == desc_.data_address) return static_cast<u8>(data_ & 0xFFU);
    if (address == desc_.data_address + 1) return static_cast<u8>((data_ >> 8U) & 0xFFU);
    
    if (address == desc_.addr_address) return static_cast<u8>(addr_ & 0xFFU);
    if (address == desc_.addr_address + 1) return static_cast<u8>((addr_ >> 8U) & 0xFFU);

    return 0x00;
}

void NvmCtrl::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value & 0x7FU; // Only CMD bits are writable in CTRLA for now
        if (ctrla_ != 0 && bus_) {
            bus_->execute_nvm_command(ctrla_, addr_, data_);
        }
    } else if (address == desc_.ctrlb_address) {
        ctrlb_ = value;
    } else if (address == desc_.status_address) {
        // Status bits are usually read-only or cleared by HW
    } else if (address == desc_.intctrl_address) {
        intctrl_ = value;
    } else if (address == desc_.intflags_address) {
        intflags_ &= ~value; // Clear on write 1
    } else if (address == desc_.data_address) {
        data_ = (data_ & 0xFF00U) | value;
    } else if (address == desc_.data_address + 1) {
        data_ = (data_ & 0x00FFU) | (static_cast<u16>(value) << 8U);
    } else if (address == desc_.addr_address) {
        addr_ = (addr_ & 0xFF00U) | value;
    } else if (address == desc_.addr_address + 1) {
        addr_ = (addr_ & 0x00FFU) | (static_cast<u16>(value) << 8U);
    }
}

void NvmCtrl::reset() noexcept {
    ctrla_ = 0;
    ctrlb_ = 0;
    status_ = 0;
    intctrl_ = 0;
    intflags_ = 0;
    addr_ = 0;
    data_ = 0;
}

} // namespace vioavr::core
