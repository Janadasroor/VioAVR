#include "vioavr/core/xmem.hpp"
#include "vioavr/core/cpu_control.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/logger.hpp"

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
    
    // We don't intercept MCUCR address here, but we'll monitor it via cpu_control_
    // for chips like ATmega128.
    
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
    if (address == desc_.xmem.xmcra_address) return xmcra_;
    if (address == desc_.xmem.xmcrb_address) return xmcrb_;
    return 0U;
}

void Xmem::write(u16 address, u8 value) noexcept
{
    if (address == desc_.xmem.xmcra_address) {
        xmcra_ = value;
    } else if (address == desc_.xmem.xmcrb_address) {
        xmcrb_ = value;
    }
}

u8 Xmem::read_external(u16 address) noexcept
{
    if (!enabled() || address >= memory_.size() || address <= desc_.data_end_address()) {
        return 0U; // Or bus conflict / invalid
    }
    return memory_[address];
}

void Xmem::write_external(u16 address, u8 value) noexcept
{
    if (!enabled() || address >= memory_.size() || address <= desc_.data_end_address()) {
        return;
    }
    memory_[address] = value;
}

bool Xmem::enabled() const noexcept
{
    if ((xmcra_ & desc_.xmem.sre_mask) != 0U) return true;
    if (desc_.xmem.mcucr_address != 0U && (cpu_control_.mcucr() & desc_.xmem.sre_mask) != 0U) return true;
    return false;
}

u8 Xmem::get_wait_states(u16 address) const noexcept
{
    // Evaluate if enabled. 
    // SRE is in XMCRA bit 7 or MCUCR bit 7.
    bool sre = (xmcra_ & desc_.xmem.sre_mask) != 0U;
    if (!sre && desc_.xmem.mcucr_address != 0U) {
        sre = (cpu_control_.mcucr() & desc_.xmem.sre_mask) != 0U;
    }

    if (!sre || address <= desc_.data_end_address()) {
        return 0U; 
    }
    
    // SRW11:SRW0 sets wait states.
    // Wait bits depend on the chip. 
    // ATmega2560: XMCRA[3:0] (SRW11:10, SRW01:00)
    // ATmega128: MCUCR[6] is SRW10, XMCRA[3:2] is SRW11. 
    
    // For now, assume common wait bits are in XMCRA[3:0]
    return xmcra_ & 0x03U; 
}

} // namespace vioavr::core
