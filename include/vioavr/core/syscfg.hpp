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
        if (desc_.reves_address != 0U) {
            // REVID (read-only) and EXTBRK (writable) are contiguous
            ranges_[0] = {desc_.reves_address, static_cast<u16>(desc_.reves_address + 1U)};
        }
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
        if (address == desc_.reves_address + 1U) return syscfg_;
        return 0U;
    }

    void write(u16 address, u8 value) noexcept override {
        if (address == desc_.reves_address) {
            // REVID is read-only; writes are ignored
        } else if (address == desc_.reves_address + 1U) {
            syscfg_ = value & 0x03U; // Only EXTBRK (bit 0) and bit 1 are writable
        }
    }

private:
    const SyscfgDescriptor desc_;
    std::array<AddressRange, 1> ranges_{};
    u8 revid_;
    u8 syscfg_{0U};
};

} // namespace vioavr::core
