#include "vioavr/core/ptc.hpp"
#include "vioavr/core/logger.hpp"

namespace vioavr::core {

// Register offsets from base address
enum PtcReg : u8 {
    REG_CTRLA    = 0x00,
    REG_CTRLB    = 0x01,
    REG_CTRLC    = 0x02,
    REG_CTRLD    = 0x03,
    REG_EVCTRL   = 0x04,
    REG_INTCTRL  = 0x05,
    REG_INTFLAGS = 0x06,
    REG_DBGCTRL  = 0x07,
    REG_SYNCBUSY = 0x08,
    REG_DATAL    = 0x09,
    REG_DATAH    = 0x0A,
    REG_INTSTAT  = 0x0B,
    REG_WCOMPCTRL = 0x0D,
};

// CTRLA bitfields
constexpr u8 CTRLA_SWRST    = 0x80;
constexpr u8 CTRLA_MODE_gm  = 0x0C;
constexpr u8 CTRLA_MODE_gp  = 2;
constexpr u8 CTRLA_RUNSTDBY = 0x02;
constexpr u8 CTRLA_ENABLE   = 0x01;

// INTCTRL/INTFLAGS bitfields
constexpr u8 INT_WCOMP = 0x02;
constexpr u8 INT_EOC   = 0x01;

// Typical conversion time in cycles (~120 µs at 16 MHz = 1920 cycles)
constexpr u64 CONVERSION_TIME_CYCLES = 1920;

Ptc::Ptc(const PtcDescriptor& desc) noexcept : desc_(desc) {
    if (desc_.base_address != 0) {
        ranges_[0] = {desc_.base_address, static_cast<u16>(desc_.base_address + 0x0D)};
    }
}

std::span<const AddressRange> Ptc::mapped_ranges() const noexcept {
    return {ranges_.data(), desc_.base_address != 0 ? 1U : 0U};
}

void Ptc::reset() noexcept {
    ctrla_ = 0;
    ctrlb_ = 0;
    ctrlc_ = 0;
    ctrld_ = 0;
    evctrl_ = 0;
    intctrl_ = 0;
    intflags_ = 0;
    dbgctrl_ = 0;
    wcompctrl_ = 0;
    result_ = 0;
    intstat_ = 0;
    converting_ = false;
    conversion_start_cycle_ = 0;
    int_pending_ = false;
}

u8 Ptc::reg_offset(u16 address) const noexcept {
    return static_cast<u8>(address - desc_.base_address);
}

u8 Ptc::read(u16 address) noexcept {
    u8 offset = reg_offset(address);
    if (offset > 0x0D) return 0;

    switch (offset) {
    case REG_CTRLA:    return ctrla_;
    case REG_CTRLB:    return ctrlb_;
    case REG_CTRLC:    return ctrlc_;
    case REG_CTRLD:    return ctrld_;
    case REG_EVCTRL:   return evctrl_;
    case REG_INTCTRL:  return intctrl_;
    case REG_INTFLAGS: return intflags_;
    case REG_DBGCTRL:  return dbgctrl_;
    case REG_SYNCBUSY: return (ctrla_ & CTRLA_SWRST) ? 0x01 : 0;
    case REG_DATAL:    return result_ & 0xFF;
    case REG_DATAH:    return (result_ >> 8) & 0xFF;
    case REG_INTSTAT:  return intstat_;
    case REG_WCOMPCTRL: return wcompctrl_;
    default: return 0;
    }
}

void Ptc::write(u16 address, u8 value) noexcept {
    u8 offset = reg_offset(address);
    if (offset > 0x0D) return;

    switch (offset) {
    case REG_CTRLA:
        if (value & CTRLA_SWRST) {
            reset();
            return;
        }
        ctrla_ = value & 0x8F;
        if (ctrla_ & CTRLA_ENABLE) {
            converting_ = true;
            conversion_cycles_ = 0;
        } else {
            converting_ = false;
            int_pending_ = false;
        }
        break;
    case REG_CTRLB:    ctrlb_ = value; break;
    case REG_CTRLC:    ctrlc_ = value & 0x03; break;
    case REG_CTRLD:    ctrld_ = value & 0x03; break;
    case REG_EVCTRL:   evctrl_ = value & 0x03; break;
    case REG_INTCTRL:  intctrl_ = value & 0x03; update_interrupt_pending(); break;
    case REG_INTFLAGS:
        intflags_ &= ~value; // Write-1-to-clear
        if (!intflags_) int_pending_ = false;
        update_interrupt_pending();
        break;
    case REG_DBGCTRL:  dbgctrl_ = value & 0x01; break;
    case REG_WCOMPCTRL: wcompctrl_ = value; break;
    default: break;
    }
}

void Ptc::tick(u64 elapsed_cycles) noexcept {
    if (!(ctrla_ & CTRLA_ENABLE) || desc_.base_address == 0) return;

    if (converting_) {
        conversion_cycles_ += elapsed_cycles;
        if (conversion_cycles_ >= CONVERSION_TIME_CYCLES) {
            converting_ = false;
            result_ = capacitance_;
            intflags_ |= INT_EOC;
            intstat_ |= INT_EOC;
            update_interrupt_pending();
            Logger::debug("PTC conversion complete, result=0x" + Logger::hex(result_));
        }
    }
}

bool Ptc::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (!int_pending_) return false;
    if (intflags_ & INT_EOC) {
        request = InterruptRequest{.vector_index = desc_.eoc_vector_index, .source_id = 0U};
        return true;
    }
    if (intflags_ & INT_WCOMP) {
        request = InterruptRequest{.vector_index = desc_.wcomp_vector_index, .source_id = 0U};
        return true;
    }
    return false;
}

bool Ptc::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (!int_pending_) return false;
    if (intflags_ & INT_EOC) {
        request = InterruptRequest{.vector_index = desc_.eoc_vector_index, .source_id = 0U};
        intflags_ &= ~INT_EOC;
        intstat_ &= ~INT_EOC;
        if (!intflags_) int_pending_ = false;
        update_interrupt_pending();
        return true;
    }
    if (intflags_ & INT_WCOMP) {
        request = InterruptRequest{.vector_index = desc_.wcomp_vector_index, .source_id = 0U};
        intflags_ &= ~INT_WCOMP;
        intstat_ &= ~INT_WCOMP;
        if (!intflags_) int_pending_ = false;
        update_interrupt_pending();
        return true;
    }
    return false;
}

void Ptc::update_interrupt_pending() noexcept {
    bool any_enabled = false;
    if ((intctrl_ & INT_EOC) && (intflags_ & INT_EOC)) any_enabled = true;
    if ((intctrl_ & INT_WCOMP) && (intflags_ & INT_WCOMP)) any_enabled = true;
    int_pending_ = any_enabled;
}

} // namespace vioavr::core
