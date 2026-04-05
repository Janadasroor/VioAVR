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
    // Add SMCR if not already covered (ATmega328P: SMCR=0x53)
    if (desc.smcr_address != 0U) {
        bool covered = false;
        for (const auto& r : ranges_) {
            if (desc.smcr_address >= r.begin && desc.smcr_address <= r.end) {
                covered = true;
                break;
            }
        }
        if (!covered) {
            ranges_.push_back(AddressRange{desc.smcr_address, desc.smcr_address});
        }
    }
    // Add MCUSR if not already covered (ATmega328P: MCUSR=0x54)
    if (desc.mcusr_address != 0U) {
        bool covered = false;
        for (const auto& r : ranges_) {
            if (desc.mcusr_address >= r.begin && desc.mcusr_address <= r.end) {
                covered = true;
                break;
            }
        }
        if (!covered) {
            ranges_.push_back(AddressRange{desc.mcusr_address, desc.mcusr_address});
        }
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
    smcr_ = 0U;
    // On power-on reset, set PORF flag
    // This is the initial reset - subsequent resets (via AvrCpu::reset()) should preserve existing flags
    mcusr_ = MCUSR_PORF;
    sleep_mode_ = SleepMode::idle;
    sleep_enabled_ = false;
    sleeping_ = false;
}

void CpuControl::set_reset_cause(ResetCause cause) noexcept
{
    switch (cause) {
    case ResetCause::power_on:    mcusr_ |= MCUSR_PORF;  break;
    case ResetCause::external:    mcusr_ |= MCUSR_EXTRF; break;
    case ResetCause::brown_out:   mcusr_ |= MCUSR_BORF;  break;
    case ResetCause::watchdog:    mcusr_ |= MCUSR_WDRF;  break;
    }
}

void CpuControl::tick(const u64 elapsed_cycles) noexcept
{
    (void)elapsed_cycles;
}

u8 CpuControl::read(const u16 address) noexcept
{
    if (address == cpu_.bus().device().mcusr_address) {
        return mcusr_;
    }
    if (address == cpu_.bus().device().smcr_address) {
        return smcr_;
    }
    if (address == cpu_.bus().device().spmcsr_address) {
        u8 val = spmcsr_;
        if (cpu_.bus().flash_rww_busy()) {
            val |= 0x40U; // RWWSB bit
        }
        return val;
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
    if (address == cpu_.bus().device().mcusr_address) {
        return mcusr_;
    }
    return 0U;
}

void CpuControl::write(const u16 address, const u8 value) noexcept
{
    if (address == cpu_.bus().device().mcusr_address) {
        // MCUSR is cleared by writing 0 to it
        // Only flag bits can be cleared; other bits are read-only
        if (value == 0U) {
            mcusr_ = 0U;
        } else {
            // Writing non-zero doesn't set flags, only clears them
            // Flags are set by hardware based on reset cause
            mcusr_ &= ~value;
        }
    } else if (address == cpu_.bus().device().smcr_address) {
        smcr_ = value;
        sleep_enabled_ = (value & 0x01U) != 0U;
        sleep_mode_ = static_cast<SleepMode>((value >> 1U) & 0x07U);
    } else if (address == cpu_.bus().device().spmcsr_address) {
        // Shield read-only status bits (like RWWSB at bit 6)
        spmcsr_ = value & 0xBFU;
    } else if (address == cpu_.bus().device().spl_address) {
        const u16 current = cpu_.stack_pointer();
        cpu_.set_stack_pointer(static_cast<u16>((current & 0xFF00U) | value));
    } else if (address == cpu_.bus().device().sph_address) {
        const u16 current = cpu_.stack_pointer();
        cpu_.set_stack_pointer(static_cast<u16>((static_cast<u16>(value) << 8U) | (current & 0x00FFU)));
    } else if (address == cpu_.bus().device().mcusr_address) {
        // MCUSR is cleared by writing 0 to the bits (Actually, usually it's bit-clearing logic, 
        // but simple assignment is common in simple emulators)
        mcusr_ = value;
    } else if (address == cpu_.bus().device().sreg_address) {
        cpu_.write_sreg(value);
    }
}

}  // namespace vioavr::core
