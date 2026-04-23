#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <array>

namespace vioavr::core {

/// Sleep Controller (AVR8X)
/// Stores the sleep mode configuration (CTRLA). The actual sleep logic is
/// handled by the CPU control layer which reads this peripheral's state.
class SlpCtrl : public IoPeripheral {
public:
    // CTRLA bitmasks
    static constexpr u8 SEN_MASK  = 0x01U; ///< Sleep Enable
    static constexpr u8 SMODE_MASK = 0x0EU; ///< Sleep Mode bits [3:1]

    explicit SlpCtrl(const SlpctrlDescriptor& desc) noexcept : desc_(desc) {
        if (desc_.ctrla_address != 0U)
            ranges_[0] = {desc_.ctrla_address, desc_.ctrla_address};
    }

    [[nodiscard]] std::string_view name() const noexcept override { return "SLPCTRL"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override {
        return {ranges_.data(), (desc_.ctrla_address != 0U) ? 1U : 0U};
    }
    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

    void reset() noexcept override { ctrla_ = 0U; }
    void tick(u64) noexcept override {}

    u8 read(u16 address) noexcept override {
        if (address == desc_.ctrla_address) return ctrla_;
        return 0U;
    }

    void write(u16 address, u8 value) noexcept override {
        if (address == desc_.ctrla_address) ctrla_ = value;
    }

    /// True if the CPU SLEEP instruction should actually enter sleep.
    [[nodiscard]] bool sleep_enabled() const noexcept { return (ctrla_ & SEN_MASK) != 0U; }

    /// Returns the SMODE bits [2:0] (shift right by 1, masked to 3 bits).
    [[nodiscard]] u8 sleep_mode() const noexcept { return (ctrla_ & SMODE_MASK) >> 1U; }

private:
    const SlpctrlDescriptor desc_;
    std::array<AddressRange, 1> ranges_{};
    u8 ctrla_{0U};
};

} // namespace vioavr::core
