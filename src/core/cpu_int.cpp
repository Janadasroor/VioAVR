#include "vioavr/core/cpu_int.hpp"
#include <algorithm>
#include <vector>

namespace vioavr::core {

CpuInt::CpuInt(const CpuIntDescriptor& desc) : desc_(desc) {
    if (desc_.ctrla_address != 0U) {
        std::vector<u16> addrs = {
            desc_.ctrla_address, desc_.status_address,
            desc_.lvl0pri_address, desc_.lvl1vec_address
        };
        std::sort(addrs.begin(), addrs.end());
        
        // Compact into single range if contiguous (usually it is for CPUINT)
        ranges_[0].begin = addrs.front();
        ranges_[0].end = addrs.back();
    }
}

std::span<const AddressRange> CpuInt::mapped_ranges() const noexcept {
    if (ranges_[0].begin == 0) return {};
    return {ranges_.data(), 1};
}

u8 CpuInt::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.status_address) return status_;
    if (address == desc_.lvl0pri_address) return lvl0pri_;
    if (address == desc_.lvl1vec_address) return lvl1vec_;
    return 0x00;
}

void CpuInt::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value & 0x61U; // LVL0RR, CVT, IVSEL
    } else if (address == desc_.status_address) {
        // Read-only or cleared by HW, but mostly read-only executing flags
    } else if (address == desc_.lvl0pri_address) {
        lvl0pri_ = value;
    } else if (address == desc_.lvl1vec_address) {
        lvl1vec_ = value;
    }
}

void CpuInt::reset() noexcept {
    ctrla_ = 0;
    status_ = 0;
    lvl0pri_ = 0;
    lvl1vec_ = 0xFF; // Disabled
    last_ack_vector_ = 0;
}

} // namespace vioavr::core
