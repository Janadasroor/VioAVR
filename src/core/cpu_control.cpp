#include "vioavr/core/cpu_control.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/pin_mux.hpp"

namespace vioavr::core {

CpuControl::CpuControl(AvrCpu& cpu, const DeviceDescriptor& desc) noexcept
    : cpu_(cpu), 
      base_frequency_hz_(desc.cpu_frequency_hz),
      internal_rc_active_(desc.internal_rc_active)
{
    ranges_.push_back(AddressRange{desc.spl_address, desc.sreg_address});
    if (desc.smcr_address != 0U) {
        ranges_.push_back(AddressRange{desc.smcr_address, desc.smcr_address});
    }
    if (desc.mcucr_address != 0U) {
        bool covered = false;
        for (const auto& r : ranges_) {
            if (desc.mcucr_address >= r.begin && desc.mcucr_address <= r.end) {
                covered = true;
                break;
            }
        }
        if (!covered) {
            ranges_.push_back(AddressRange{desc.mcucr_address, desc.mcucr_address});
        }
    }
    if (desc.rampz_address != 0U) ranges_.push_back({desc.rampz_address, desc.rampz_address});
    if (desc.eind_address != 0U) ranges_.push_back({desc.eind_address, desc.eind_address});
    if (desc.ccp_address != 0U) ranges_.push_back({desc.ccp_address, desc.ccp_address});
    // Add PRR/PRR0/PRR1
    if (desc.prr_address != 0U) ranges_.push_back({desc.prr_address, desc.prr_address});
    if (desc.prr0_address != 0U) ranges_.push_back({desc.prr0_address, desc.prr0_address});
    if (desc.prr1_address != 0U) ranges_.push_back({desc.prr1_address, desc.prr1_address});
    if (desc.clkpr_address != 0U) ranges_.push_back({desc.clkpr_address, desc.clkpr_address});
    if (desc.osccal_address != 0U) ranges_.push_back({desc.osccal_address, desc.osccal_address});
    // PLLCSR is managed by Timer10/Timer4
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
    mcucr_ = 0U;
    sleep_mode_ = SleepMode::idle;
    sleep_enabled_ = false;
    sleeping_ = false;
    ccp_ = 0x00U;
    ccp_expiry_ = 0;
    clkpr_ = 0x00U; // Default is 1:1 or 1:8 depending on fuse CKDIV8
    clkpr_expiry_ = 0;
    osccal_ = 0x80U;
}

void CpuControl::set_reset_cause(ResetCause cause) noexcept
{
    mcusr_ &= 0xF0U; // Clear PORF, EXTRF, BORF, WDRF
    switch (cause) {
        case ResetCause::power_on: mcusr_ |= MCUSR_PORF; break;
        case ResetCause::external: mcusr_ |= MCUSR_EXTRF; break;
        case ResetCause::brown_out: mcusr_ |= MCUSR_BORF; break;
        case ResetCause::watchdog: mcusr_ |= MCUSR_WDRF; break;
    }
}
 
u64 CpuControl::effective_frequency() const noexcept
{
    u64 freq = base_frequency_hz_;
    
    // 1. Apply OSCCAL scaling if internal RC is active
    if (internal_rc_active_) {
        // Simple linear approximation: 
        // 0x00 -> 50% frequency
        // 0x80 -> 100% frequency
        // 0xFF -> ~150% frequency
        float scale = 0.5f + (static_cast<float>(osccal_) / 128.0f) * 0.5f;
        freq = static_cast<u64>(static_cast<float>(freq) * scale);
    }
    
    // 2. Apply CLKPR prescaler
    // Divider is 2^(CLKPS)
    u8 prescaler = clkpr_ & 0x0FU;
    if (prescaler > 8) prescaler = 8; // Max divider is 256
    freq >>= prescaler;
    
    return freq;
}

void CpuControl::tick(const u64 elapsed_cycles) noexcept
{
    (void)elapsed_cycles;
}

u8 CpuControl::read(const u16 address) noexcept
{
    const auto& d = cpu_.bus().device();
    if (address == d.mcusr_address) return mcusr_;
    if (address == d.mcucr_address) return mcucr_;
    if (address == d.smcr_address) return smcr_;
    if (address == d.prr_address) return prr_;
    if (address == d.prr0_address) return prr0_;
    if (address == d.prr1_address) return prr1_;
    if (address == d.pllcsr_address) return pllcsr_;
    if (address == d.clkpr_address) return clkpr_;
    if (address == d.osccal_address) return osccal_;
    
    if (address == d.spl_address) return static_cast<u8>(cpu_.stack_pointer() & 0xFFU);
    if (address == d.sph_address) return static_cast<u8>((cpu_.stack_pointer() >> 8U) & 0xFFU);
    if (address == d.sreg_address) return cpu_.sreg();
    if (address == d.rampz_address) return cpu_.rampz();
    if (address == d.eind_address) return cpu_.eind();
    if (address == d.ccp_address) return ccp_;
    return 0U;
}

void CpuControl::write(const u16 address, const u8 value) noexcept
{
    const auto& d = cpu_.bus().device();
    if (address == d.mcusr_address) {
        mcusr_ &= ~value; // Flags are cleared by writing 1 (Accuracy)
    } else if (address == d.mcucr_address) {
        mcucr_ = value;
        if (d.mcucr_pud_mask != 0) {
            if (auto* mux = cpu_.bus().pin_mux()) {
                mux->update_pullup_suppressed((value & d.mcucr_pud_mask) != 0);
            }
        }
    } else if (address == d.smcr_address) {
        smcr_ = value;
        sleep_enabled_ = (value & d.smcr_se_mask) != 0U;
        // Accurate bit shift for SM bits
        u8 sm_mask = d.smcr_sm_mask;
        u8 sm_shift = 0;
        if (sm_mask > 0) {
            while (!(sm_mask & 0x01)) { sm_mask >>= 1; sm_shift++; }
        }
        sleep_mode_ = static_cast<SleepMode>((value & d.smcr_sm_mask) >> sm_shift);
    } else if (address == d.prr_address) {
        prr_ = value;
    } else if (address == d.prr0_address) {
        prr0_ = value;
    } else if (address == d.prr1_address) {
        prr1_ = value;
    } else if (address == d.pllcsr_address) {
        pllcsr_ = value;
    } else if (address == d.clkpr_address) {
        if (value == 0x80U) {
            // Write to CLKPCE
            clkpr_expiry_ = cpu_.cycles() + 4;
        } else if (cpu_.cycles() < clkpr_expiry_) {
            // Update CLKPS if CE is not set in this write
            if (!(value & 0x80U)) {
                clkpr_ = value & 0x0FU;
                clkpr_expiry_ = 0;
            }
        }
    } else if (address == d.osccal_address) {
        osccal_ = value;
    } else if (address == d.spl_address) {
        const u16 sp = cpu_.stack_pointer();
        cpu_.set_stack_pointer(static_cast<u16>((sp & 0xFF00U) | value));
    } else if (address == d.sph_address) {
        const u16 sp = cpu_.stack_pointer();
        cpu_.set_stack_pointer(static_cast<u16>((static_cast<u16>(value) << 8U) | (sp & 0x00FFU)));
    } else if (address == d.sreg_address) {
        cpu_.write_sreg(value);
    } else if (address == d.rampz_address) {
        cpu_.set_rampz(value);
    } else if (address == d.eind_address) {
        cpu_.set_eind(value);
    } else if (address == d.ccp_address) {
        unlock_ccp(value);
    }
}

bool CpuControl::is_ccp_unlocked() const noexcept {
    return cpu_.cycles() < ccp_expiry_;
}

void CpuControl::unlock_ccp(u8 signature) noexcept {
    // 0xD8: IOREG, 0x9D: SPM
    if (signature == 0xD8 || signature == 0x9D) {
        ccp_ = signature;
        ccp_expiry_ = cpu_.cycles() + 4;
    }
}

u8 CpuControl::clock_prescaler() const noexcept {
    u8 ps = clkpr_ & 0x0FU;
    if (ps > 8) return 1; // Reserved
    return static_cast<u8>(1U << ps);
}

}  // namespace vioavr::core
