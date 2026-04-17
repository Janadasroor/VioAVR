#include "vioavr/core/xmem.hpp"
#include "vioavr/core/cpu_control.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/logger.hpp"
#include <algorithm>

namespace vioavr::core {

Xmem::Xmem(const DeviceDescriptor& desc, CpuControl& cpu_control) noexcept 
    : desc_(desc), cpu_control_(cpu_control)
{
    if (desc_.xmem.xmcra_address != 0U) {
        ranges_.push_back({desc_.xmem.xmcra_address, desc_.xmem.xmcra_address});
    }
    if (desc_.xmem.xmcrb_address != 0U) {
        ranges_.push_back({desc_.xmem.xmcrb_address, desc_.xmem.xmcrb_address});
    }
    
    // Internal SRAM ends at data_end_address()
    // External SRAM can go up to 64KB (0xFFFF)
    memory_.resize(0x10000); 
}

std::string_view Xmem::name() const noexcept { return "XMEM"; }
std::span<const AddressRange> Xmem::mapped_ranges() const noexcept { return ranges_; }

void Xmem::reset() noexcept {
    xmcra_ = 0U;
    xmcrb_ = 0U;
    enabled_ = false;
    // Memory contents usually preserved or random, we'll keep existing contents
}

void Xmem::tick(u64 elapsed_cycles) noexcept { (void)elapsed_cycles; }

u8 Xmem::read(u16 address) noexcept {
    if (address == desc_.xmem.xmcra_address) return xmcra_;
    if (address == desc_.xmem.xmcrb_address) return xmcrb_;
    return 0U;
}

void Xmem::write(u16 address, u8 value) noexcept {
    if (address == desc_.xmem.xmcra_address) {
        xmcra_ = value;
    } else if (address == desc_.xmem.xmcrb_address) {
        xmcrb_ = value;
    }
}

u8 Xmem::read_external(u16 address) noexcept {
    if (!enabled() || address >= memory_.size() || address <= desc_.data_end_address()) {
        return 0U; 
    }
    return memory_[address];
}

void Xmem::write_external(u16 address, u8 value) noexcept {
    if (!enabled() || address >= memory_.size() || address <= desc_.data_end_address()) {
        return;
    }
    memory_[address] = value;
}

bool Xmem::enabled() const noexcept {
    if ((xmcra_ & desc_.xmem.sre_mask) != 0U) return true;
    if (desc_.xmem.mcucr_address != 0U && (cpu_control_.mcucr() & desc_.xmem.sre_mask) != 0U) return true;
    return false;
}

u8 Xmem::get_wait_states(u16 address) const noexcept {
    if (!enabled() || address <= desc_.data_end_address()) return 0U;

    // Sector partitioning logic
    // SRE is enabled. Check if address is in LS or US.
    
    u8 srl_raw = (xmcra_ & desc_.xmem.srl_mask);
    u8 srl = 0;
    if (desc_.xmem.srl_mask > 0) {
        u8 mask = desc_.xmem.srl_mask;
        while (!(mask & 1)) { mask >>= 1; srl_raw >>= 1; }
        srl = srl_raw;
    }

    u16 boundary = 0;
    if (srl > 0) {
        boundary = (srl * 0x2000) - 1;
    }

    u8 wait_bits = 0;
    if (address <= boundary) {
        // Lower Sector
        u8 val = (xmcra_ & desc_.xmem.srw0_mask);
        u8 mask = desc_.xmem.srw0_mask;
        if (mask > 0) {
            while (!(mask & 1)) { mask >>= 1; val >>= 1; }
        }
        wait_bits = val;
    } else {
        // Upper Sector
        u8 val = (xmcra_ & desc_.xmem.srw1_mask);
        u8 mask = desc_.xmem.srw1_mask;
        if (mask > 0) {
            while (!(mask & 1)) { mask >>= 1; val >>= 1; }
        }
        wait_bits = val;
    }

    switch (wait_bits) {
        case 0b01: return 1U;
        case 0b10: return 2U;
        case 0b11: return 3U; // 2 wait + 1 hold
        default:   return 0U;
    }
}

} // namespace vioavr::core
