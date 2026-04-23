#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <array>
#include <string_view>

namespace vioavr::core {

/// System Configuration (AVR8X — SYSCFG)
/// Exposes the chip silicon revision ID as a read-only register (REVID).
/// Also holds EXTBRK (external break enable for debug).
class SysCfg : public IoPeripheral {
public:
    explicit SysCfg(const SyscfgDescriptor& desc, u8 revision_id = 0x00U) noexcept
        : desc_(desc), revid_(revision_id)
    {
        if (desc_.reves_address != 0U)
            ranges_[0] = {desc_.reves_address, desc_.reves_address};
    }

    [[nodiscard]] std::string_view name() const noexcept override { return "SYSCFG"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override {
        return {ranges_.data(), (desc_.reves_address != 0U) ? 1U : 0U};
    }
    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

    void reset() noexcept override {}
    void tick(u64) noexcept override {}

    u8 read(u16 address) noexcept override {
        if (address == desc_.reves_address) return revid_;
        return 0U;
    }

    void write(u16 /*address*/, u8 /*value*/) noexcept override {
        // REVID is read-only; writes are ignored
    }

private:
    const SyscfgDescriptor desc_;
    std::array<AddressRange, 1> ranges_{};
    u8 revid_;
};

} // namespace vioavr::core
