#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <array>
#include <functional>

namespace vioavr::core {

/// XMEGA Clock Controller — CLK.CTRL / PSCTRL / LOCK / RTCCTRL
///
/// Emulates clock source selection (SCLKSEL), prescaler A (PSADIV),
/// prescaler B/C (PSBCDIV), lock, and RTC clock source selection.
/// The effective CPU frequency is computed from the selected source
/// divided by PSADIV, and a callback notifies consumers on changes.
///
/// CCP write sequences are not enforced (same as AVR8X ClkCtrl).
class XmegaClkCtrl : public IoPeripheral {
public:
    // CLK.CTRL bits
    static constexpr u8 SCLKSEL_MASK = 0x07U;

    // CLK.PSCTRL bits
    static constexpr u8 PSADIV_MASK  = 0x07U;
    static constexpr u8 PSBCDIV_MASK = 0x18U;

    enum class ClockSource : u8 {
        rc2mhz  = 0x00,
        rc32mhz = 0x01,
        rc32khz = 0x02,
        xosc    = 0x03,
        pll     = 0x04,
    };

    static constexpr u32 SRC_RC2MHZ  =   2'000'000U;
    static constexpr u32 SRC_RC32MHZ =  32'000'000U;
    static constexpr u32 SRC_RC32KHZ =      32'768U;

    static constexpr u32 psadiv_table[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
    static constexpr u8 psbcdiv_clkper2[4] = { 1, 2, 4, 2 };
    static constexpr u8 psbcdiv_clkper4[4] = { 1, 1, 1, 2 };

    using FrequencyChangedCallback = std::function<void(u32 new_hz)>;

    explicit XmegaClkCtrl(u16 ctrl_addr, u16 psctrl_addr, u16 lock_addr, u16 rtcctrl_addr,
                          u32 xosc_freq_hz = 0U, u32 pll_freq_hz = 0U) noexcept
        : ctrl_addr_(ctrl_addr), psctrl_addr_(psctrl_addr),
          lock_addr_(lock_addr), rtcctrl_addr_(rtcctrl_addr),
          xosc_freq_hz_(xosc_freq_hz), pll_freq_hz_(pll_freq_hz)
    {
        build_ranges();
        recompute_freq();
    }

    [[nodiscard]] std::string_view name() const noexcept override { return "XMEGA CLKCTRL"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override {
        return {ranges_.data(), range_count_};
    }
    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

    void reset() noexcept override {
        clk_ctrl_  = 0x01U;
        psctrl_    = 0x00U;
        lock_      = 0x00U;
        rtcctrl_   = 0x00U;
        recompute_freq();
    }

    void tick(u64) noexcept override {}

    u8 read(u16 address) noexcept override {
        if (address == ctrl_addr_)    return clk_ctrl_;
        if (address == psctrl_addr_)  return psctrl_;
        if (address == lock_addr_)    return lock_;
        if (address == rtcctrl_addr_) return rtcctrl_;
        return 0U;
    }

    void write(u16 address, u8 value) noexcept override {
        bool locked = (lock_ & 0x01U) != 0U;
        if (address == ctrl_addr_ && !locked) {
            clk_ctrl_ = value & SCLKSEL_MASK;
            recompute_freq();
        } else if (address == psctrl_addr_ && !locked) {
            psctrl_ = value & (PSADIV_MASK | PSBCDIV_MASK);
            recompute_freq();
        } else if (address == lock_addr_) {
            lock_ = value & 0x01U;
        } else if (address == rtcctrl_addr_) {
            rtcctrl_ = value;
        }
    }

    [[nodiscard]] u32 cpu_frequency_hz() const noexcept { return effective_freq_hz_; }

    [[nodiscard]] u32 source_frequency_hz() const noexcept {
        switch (static_cast<ClockSource>(clk_ctrl_ & SCLKSEL_MASK)) {
            case ClockSource::rc2mhz:  return SRC_RC2MHZ;
            case ClockSource::rc32mhz: return SRC_RC32MHZ;
            case ClockSource::rc32khz: return SRC_RC32KHZ;
            case ClockSource::xosc:    return xosc_freq_hz_ > 0U ? xosc_freq_hz_ : 32'000'000U;
            case ClockSource::pll:     return pll_freq_hz_ > 0U ? pll_freq_hz_ : 48'000'000U;
            default:                   return SRC_RC32MHZ;
        }
    }

    [[nodiscard]] u32 per2_frequency_hz() const noexcept {
        u8 idx = (psctrl_ & PSBCDIV_MASK) >> 3U;
        return effective_freq_hz_ / psbcdiv_clkper2[idx & 3];
    }

    [[nodiscard]] u32 per4_frequency_hz() const noexcept {
        u8 idx = (psctrl_ & PSBCDIV_MASK) >> 3U;
        return per2_frequency_hz() / psbcdiv_clkper4[idx & 3];
    }

    void set_frequency_changed_callback(FrequencyChangedCallback cb) noexcept {
        on_freq_changed_ = std::move(cb);
    }

    void set_xosc_frequency(u32 hz) noexcept {
        xosc_freq_hz_ = hz;
        recompute_freq();
    }

    void set_pll_frequency(u32 hz) noexcept {
        pll_freq_hz_ = hz;
        recompute_freq();
    }

private:
    void recompute_freq() noexcept {
        u32 src = source_frequency_hz();
        u32 old = effective_freq_hz_;
        u8 psadiv = psctrl_ & PSADIV_MASK;
        effective_freq_hz_ = src / psadiv_table[psadiv];
        if (effective_freq_hz_ != old && on_freq_changed_)
            on_freq_changed_(effective_freq_hz_);
    }

    void build_ranges() noexcept {
        // Two non-contiguous ranges: [CTRL..PSCTRL], [LOCK..RTCCTRL]
        ranges_[0] = {ctrl_addr_, psctrl_addr_};
        range_count_ = 1;
        ranges_[1] = {lock_addr_, rtcctrl_addr_};
        range_count_ = 2;
    }

    const u16 ctrl_addr_;
    const u16 psctrl_addr_;
    const u16 lock_addr_;
    const u16 rtcctrl_addr_;
    std::array<AddressRange, 2> ranges_{};
    u8 range_count_{0};

    u8 clk_ctrl_  {0x01U};
    u8 psctrl_    {0x00U};
    u8 lock_      {0x00U};
    u8 rtcctrl_   {0x00U};

    u32 xosc_freq_hz_ {0U};
    u32 pll_freq_hz_  {0U};
    u32 effective_freq_hz_ {32'000'000U};

    FrequencyChangedCallback on_freq_changed_;
};

} // namespace vioavr::core