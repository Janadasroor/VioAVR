#include "vioavr/core/cpu_control.hpp"
#include "vioavr/core/avr_cpu.hpp"

namespace vioavr::core {

CpuControl::CpuControl(AvrCpu& cpu, const DeviceDescriptor& desc) noexcept 
    : cpu_(cpu) 
{
    ranges_.push_back(AddressRange{desc.spl_address, desc.sreg_address});
    if (desc.spmcsr_address != 0U && (desc.spmcsr_address < desc.spl_address || desc.spmcsr_address > desc.sreg_address)) {
        ranges_.push_back(AddressRange{desc.spmcsr_address, desc.spmcsr_address});
    }
}

std::string_view CpuControl::name() const noexcept
{
    return "CPU Control";
}

std::span<const AddressRange> CpuControl::mapped_ranges() const noexcept
{
    return ranges_;
}

void CpuControl::reset() noexcept
{
    spmcsr_ = 0U;
}

void CpuControl::tick(const u64 elapsed_cycles) noexcept
{
    (void)elapsed_cycles;
    // SPMEN bit is automatically cleared after 4 cycles (simplified here as immediate/next)
    // Actually, it stays set for 4 cycles. We'll handle this in execute_spm for simplicity, 
    // but we can clear it here if we track cycles.
}

u8 CpuControl::read(const u16 address) noexcept
{
    if (address == cpu_.bus().device().spmcsr_address) {
        return spmcsr_;
    }
    if (address == cpu_.bus().device().spl_address) {
        return static_cast<u8>(cpu_.stack_pointer() & 0xFFU);
    }
    if (address == cpu_.bus().device().sph_address) {
        return static_cast<u8>((cpu_.stack_pointer() >> 8U) & 0xFFU);
    }
    if (address == cpu_.bus().device().sreg_address) {
        return cpu_.sreg();
    }
    return 0U;
}

void CpuControl::write(const u16 address, const u8 value) noexcept
{
    if (address == cpu_.bus().device().spmcsr_address) {
        spmcsr_ = value;
    } else if (address == cpu_.bus().device().spl_address) {
        const u16 current = cpu_.stack_pointer();
        cpu_.set_stack_pointer(static_cast<u16>((current & 0xFF00U) | value));
    } else if (address == cpu_.bus().device().sph_address) {
        const u16 current = cpu_.stack_pointer();
        cpu_.set_stack_pointer(static_cast<u16>((static_cast<u16>(value) << 8U) | (current & 0x00FFU)));
    } else if (address == cpu_.bus().device().sreg_address) {
        cpu_.write_sreg(value);
    }
}

}  // namespace vioavr::core
