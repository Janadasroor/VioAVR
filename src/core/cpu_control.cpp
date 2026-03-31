#include "vioavr/core/cpu_control.hpp"
#include "vioavr/core/avr_cpu.hpp"

namespace vioavr::core {

CpuControl::CpuControl(AvrCpu& cpu, const DeviceDescriptor& desc) noexcept 
    : cpu_(cpu), 
      ranges_({AddressRange{desc.spl_address, desc.sreg_address}}) 
{}

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
    // CPU reset is handled by AvrCpu itself
}

void CpuControl::tick(const u64 elapsed_cycles) noexcept
{
    (void)elapsed_cycles;
}

u8 CpuControl::read(const u16 address) noexcept
{
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
    if (address == cpu_.bus().device().spl_address) {
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
