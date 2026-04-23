#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <array>

namespace vioavr::core {

/// Reset Controller (AVR8X)
/// Tracks the cause of the last reset in RSTFR (Reset Flag Register).
/// Firmware reads RSTFR on startup to diagnose the reset source.
class RstCtrl : public IoPeripheral {
public:
    /// ResetCause mirrors the RSTFR bitfield layout on ATmega4809.
    enum ResetCause : u8 {
        power_on    = 0x01U, ///< PORF  — Power-On Reset
        brown_out   = 0x02U, ///< BORF  — Brown-Out Reset
        external    = 0x04U, ///< EXTRF — External Reset (RESET pin)
        watchdog    = 0x08U, ///< WDRF  — Watchdog Reset
        software    = 0x10U, ///< SWRF  — Software Reset
        updi        = 0x20U, ///< UPDIRF — UPDI Reset
    };

    explicit RstCtrl(const RstctrlDescriptor& desc) noexcept : desc_(desc) {
        if (desc_.rstfr_address != 0U)
            ranges_[0] = {desc_.rstfr_address, desc_.rstfr_address};
    }

    [[nodiscard]] std::string_view name() const noexcept override { return "RSTCTRL"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override {
        return {ranges_.data(), (desc_.rstfr_address != 0U) ? 1U : 0U};
    }
    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

    void reset() noexcept override {
        // On POR the PORF bit is set. Firmware clears bits by writing 1 to them.
        rstfr_ = static_cast<u8>(ResetCause::power_on);
        swrr_  = 0U;
    }

    void tick(u64) noexcept override {}

    u8 read(u16 address) noexcept override {
        if (address == desc_.rstfr_address) return rstfr_;
        return 0U;
    }

    void write(u16 address, u8 value) noexcept override {
        if (address == desc_.rstfr_address) {
            // Clear-by-writing-1 semantics
            rstfr_ &= ~value;
        }
        // SWRR (software reset request) would trigger CPU reset — omitted for now
    }

    /// Set the reset cause flags (called by the emulator on a system reset event).
    void set_reset_cause(u8 cause_flags) noexcept { rstfr_ = cause_flags; }
    [[nodiscard]] u8 reset_flags() const noexcept { return rstfr_; }

private:
    const RstctrlDescriptor desc_;
    std::array<AddressRange, 1> ranges_{};
    u8 rstfr_{static_cast<u8>(ResetCause::power_on)};
    u8 swrr_{0U};
};

} // namespace vioavr::core
