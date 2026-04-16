#include "vioavr/core/xmem.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/logger.hpp"

namespace vioavr::core {

Xmem::Xmem(const DeviceDescriptor& desc) noexcept : desc_(desc)
{
    if (desc_.xmcra_address != 0U) {
        ranges_.push_back({desc_.xmcra_address, desc_.xmcra_address});
    }
    if (desc_.xmcrb_address != 0U) {
        ranges_.push_back({desc_.xmcrb_address, desc_.xmcrb_address});
    }
    // Also intercept SRE bit in MCUCR if xmcra is missing? 
    // ATmega128 has SRE in MCUCR, but XMCRA is 0x6D.
    // In ATmega2560, SRE is in XMCRA. We'll map the memory bounds natively.
    
    // Size external memory up to 64k (0xFFFF).
    // The internal SRAM ends at data_end_address().
    memory_.resize(0x10000); 
}

std::string_view Xmem::name() const noexcept
{
    return "XMEM";
}

std::span<const AddressRange> Xmem::mapped_ranges() const noexcept
{
    return ranges_;
}

void Xmem::reset() noexcept
{
    xmcra_ = 0U;
    xmcrb_ = 0U;
    enabled_ = false;
    // Memory contents are undefined on reset, but we can clear or preserve.
}

void Xmem::tick(u64 elapsed_cycles) noexcept
{
    (void)elapsed_cycles;
}

u8 Xmem::read(u16 address) noexcept
{
    if (address == desc_.xmcra_address) return xmcra_;
    if (address == desc_.xmcrb_address) return xmcrb_;
    return 0U;
}

void Xmem::write(u16 address, u8 value) noexcept
{
    if (address == desc_.xmcra_address) {
        xmcra_ = value;
        // Evaluate SRE. For ATmega2560, SRE is bit 7 of XMCRA.
        if ((value & 0x80U) != 0U) {
            enabled_ = true;
        } else {
            enabled_ = false;
        }
    } else if (address == desc_.xmcrb_address) {
        xmcrb_ = value;
    }
}

u8 Xmem::read_external(u16 address) noexcept
{
    if (!enabled_ || address >= memory_.size() || address <= desc_.data_end_address()) {
        return 0U; // Or bus conflict / invalid
    }
    return memory_[address];
}

void Xmem::write_external(u16 address, u8 value) noexcept
{
    if (!enabled_ || address >= memory_.size() || address <= desc_.data_end_address()) {
        return;
    }
    memory_[address] = value;
}

u8 Xmem::get_wait_states(u16 address) const noexcept
{
    if (!enabled_ || address <= desc_.data_end_address()) {
        return 0U; // Internal memory has no wait states
    }
    
    // This logic relies on XMCRA settings.
    // For ATmega128/2560:
    // SRW11:SRW0 sets wait states.
    // Wait state calculation depends on the sector boundaries (SRL2:0).
    // Simplified map for now:
    return 0U;
}

} // namespace vioavr::core
