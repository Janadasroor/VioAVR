#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <array>
#include <functional>

namespace vioavr::core {

/// Clock Controller (AVR8X — CLKCTRL)
///
/// Emulates MCLKCTRLA (clock source select), MCLKCTRLB (prescaler),
/// MCLKLOCK, MCLKSTATUS, and all oscillator control registers.
///
/// The key runtime effect: when firmware writes MCLKCTRLB, the effective
/// CPU frequency changes. Consumers (e.g. UART baud rate calculator) must
/// call `cpu_frequency_hz()` rather than using a hard-coded constant.
///
/// CCP (Configuration Change Protection) write sequences are not enforced
/// in the emulator — writes to protected registers are accepted immediately.
class ClkCtrl : public IoPeripheral {
public:
    // MCLKCTRLA bits
    static constexpr u8 CLKSEL_MASK = 0x03U;
    static constexpr u8 CLKOUT_MASK = 0x80U;

    // MCLKCTRLB bits
    static constexpr u8 PEN_MASK  = 0x01U; ///< Prescaler enable
    static constexpr u8 PDIV_MASK = 0x1EU; ///< Prescaler division [4:1]

    // MCLKSTATUS bits
    static constexpr u8 SOSC_MASK    = 0x01U;
    static constexpr u8 OSC20MS_MASK = 0x10U;
    static constexpr u8 OSC32KS_MASK = 0x20U;

    // Clock source IDs (CLKSEL)
    enum class ClockSource : u8 {
        osc20m    = 0x00U, ///< 20 MHz internal RC
        osculp32k = 0x01U, ///< 32.768 kHz ULP
        xosc32k   = 0x02U, ///< 32.768 kHz crystal
        extclk    = 0x03U, ///< External clock pin
    };

    using FrequencyChangedCallback = std::function<void(u32 new_hz)>;

    explicit ClkCtrl(const ClkctrlDescriptor& desc, u32 base_freq_hz = 3'333'333U) noexcept
        : desc_(desc), base_freq_hz_(base_freq_hz)
    {
        if (desc_.ctrla_address != 0U) {
            // MCLKCTRLA..MCLKSTATUS are at base+0..base+3
            ranges_[0] = {desc_.ctrla_address, static_cast<u16>(desc_.ctrla_address + 3U)};
            // OSC20M registers at base+0x10..base+0x12
            if (desc_.osc20mctrla_address != 0U)
                ranges_[1] = {desc_.osc20mctrla_address, static_cast<u16>(desc_.osc20mctrla_address + 2U)};
            // OSC32K at base+0x18
            if (desc_.osc32kctrla_address != 0U)
                ranges_[2] = {desc_.osc32kctrla_address, desc_.osc32kctrla_address};
            // XOSC32K at base+0x1C
            if (desc_.xosc32kctrla_address != 0U)
                ranges_[3] = {desc_.xosc32kctrla_address, desc_.xosc32kctrla_address};
        }
    }

    [[nodiscard]] std::string_view name() const noexcept override { return "CLKCTRL"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override {
        size_t count = 0;
        while (count < ranges_.size() && ranges_[count].begin != 0U) ++count;
        return {ranges_.data(), count};
    }
    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

    void reset() noexcept override {
        // ATmega4809 reset defaults:
        // MCLKCTRLA = 0x00 (OSC20M), MCLKCTRLB = 0x11 (prescaler ON, /6 → ~3.33MHz)
        mclkctrla_    = 0x00U;
        mclkctrlb_    = 0x11U;
        mclklock_     = 0x00U;
        osc20mctrla_  = 0x00U;
        osc20mcaliba_ = 0x00U;
        osc20mcalibb_ = 0x00U;
        osc32kctrla_  = 0x00U;
        xosc32kctrla_ = 0x00U;
        update_status();
    }

    void tick(u64) noexcept override {}

    u8 read(u16 address) noexcept override {
        if (address == desc_.ctrla_address)         return mclkctrla_;
        if (address == desc_.ctrlb_address)         return mclkctrlb_;
        if (address == desc_.mclklock_address)      return mclklock_;
        if (address == desc_.mclkstatus_address)    return mclkstatus_;
        if (address == desc_.osc20mctrla_address)   return osc20mctrla_;
        if (address == desc_.osc20mcalib_address)   return osc20mcaliba_;
        if (address == static_cast<u16>(desc_.osc20mcalib_address + 1U)) return osc20mcalibb_;
        if (address == desc_.osc32kctrla_address)   return osc32kctrla_;
        if (address == desc_.xosc32kctrla_address)  return xosc32kctrla_;
        return 0U;
    }

    void write(u16 address, u8 value) noexcept override {
        // LOCKEN prevents writes to MCLKCTRLA/B once set
        bool locked = (mclklock_ & 0x01U) != 0U;

        if (address == desc_.ctrla_address && !locked)      { mclkctrla_ = value;    update_status(); recompute_freq(); }
        else if (address == desc_.ctrlb_address && !locked) { mclkctrlb_ = value;    recompute_freq(); }
        else if (address == desc_.mclklock_address)         { mclklock_  = value & 0x01U; }
        // mclkstatus_ is read-only
        else if (address == desc_.osc20mctrla_address)      { osc20mctrla_  = value; }
        else if (address == desc_.osc20mcalib_address)      { osc20mcaliba_ = value & 0x7FU; }
        else if (address == static_cast<u16>(desc_.osc20mcalib_address + 1U)) { osc20mcalibb_ = value; }
        else if (address == desc_.osc32kctrla_address)      { osc32kctrla_  = value; }
        else if (address == desc_.xosc32kctrla_address)     { xosc32kctrla_ = value; }
    }

    /// Returns the effective CPU clock frequency in Hz after prescaler.
    [[nodiscard]] u32 cpu_frequency_hz() const noexcept { return effective_freq_hz_; }

    /// Returns the base (source) frequency before prescaler.
    [[nodiscard]] u32 source_frequency_hz() const noexcept {
        switch (static_cast<ClockSource>(mclkctrla_ & CLKSEL_MASK)) {
            case ClockSource::osc20m:    return base_freq_hz_;
            case ClockSource::osculp32k: return 32'768U;
            case ClockSource::xosc32k:  return 32'768U;
            case ClockSource::extclk:   return ext_clock_hz_;
            default:                    return base_freq_hz_;
        }
    }

    /// Set callback invoked whenever the computed CPU frequency changes.
    void set_frequency_changed_callback(FrequencyChangedCallback cb) noexcept {
        on_freq_changed_ = std::move(cb);
    }

    /// Override the base OSC20M frequency (default 3.333 MHz = 20 MHz / 6).
    void set_base_frequency(u32 hz) noexcept { base_freq_hz_ = hz; recompute_freq(); }

    /// Set the external clock pin frequency used when CLKSEL=EXTCLK.
    void set_ext_clock_hz(u32 hz) noexcept { ext_clock_hz_ = hz; recompute_freq(); }

    [[nodiscard]] ClockSource clock_source() const noexcept {
        return static_cast<ClockSource>(mclkctrla_ & CLKSEL_MASK);
    }

private:
    /// PDIV encoding → prescaler divisor (ATmega4809 datasheet Table 10-2)
    static constexpr u32 pdiv_table[16] = {
        2, 4, 8, 16, 32, 64, 1, 1, 6, 10, 12, 24, 48, 1, 1, 1
    };

    void update_status() noexcept {
        // In the emulator all oscillators are always "ready"
        mclkstatus_ = 0x00U;
        switch (static_cast<ClockSource>(mclkctrla_ & CLKSEL_MASK)) {
            case ClockSource::osc20m:    mclkstatus_ |= OSC20MS_MASK; break;
            case ClockSource::osculp32k: mclkstatus_ |= OSC32KS_MASK; break;
            case ClockSource::xosc32k:  mclkstatus_ |= 0x40U; break;
            case ClockSource::extclk:   mclkstatus_ |= 0x80U; break;
        }
    }

    void recompute_freq() noexcept {
        u32 src = source_frequency_hz();
        u32 old = effective_freq_hz_;

        if (mclkctrlb_ & PEN_MASK) {
            u8 pdiv_idx = (mclkctrlb_ & PDIV_MASK) >> 1U;
            u32 divisor = pdiv_table[pdiv_idx & 0x0FU];
            effective_freq_hz_ = src / divisor;
        } else {
            effective_freq_hz_ = src;
        }

        if (effective_freq_hz_ != old && on_freq_changed_)
            on_freq_changed_(effective_freq_hz_);
    }

    const ClkctrlDescriptor desc_;
    std::array<AddressRange, 4> ranges_{};

    u8 mclkctrla_{0x00U};
    u8 mclkctrlb_{0x11U};   ///< PEN=1, PDIV=0x08 → /6
    u8 mclklock_{0x00U};
    u8 mclkstatus_{0x10U};  ///< OSC20MS ready
    u8 osc20mctrla_{0x00U};
    u8 osc20mcaliba_{0x00U};
    u8 osc20mcalibb_{0x00U};
    u8 osc32kctrla_{0x00U};
    u8 xosc32kctrla_{0x00U};

    u32 base_freq_hz_{3'333'333U}; ///< Default: 20 MHz / 6 = 3.333 MHz
    u32 ext_clock_hz_{0U};
    u32 effective_freq_hz_{3'333'333U};

    FrequencyChangedCallback on_freq_changed_;
};

} // namespace vioavr::core
