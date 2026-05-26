#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <array>

namespace vioavr::core {

class RstCtrl : public IoPeripheral {
public:
    enum ResetCause : u8 {
        power_on    = 0x01U,
        brown_out   = 0x02U,
        external    = 0x04U,
        watchdog    = 0x08U,
        software    = 0x10U,
        updi        = 0x20U,
    };

    explicit RstCtrl(const RstctrlDescriptor& desc) noexcept : desc_(desc) {
        size_t idx = 0;
        if (desc_.rstfr_address != 0U)
            ranges_[idx++] = {desc_.rstfr_address, desc_.rstfr_address};
        if (desc_.swrr_address != 0U)
            ranges_[idx++] = {desc_.swrr_address, desc_.swrr_address};
    }

    [[nodiscard]] std::string_view name() const noexcept override { return "RSTCTRL"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override {
        size_t count = 0;
        while (count < ranges_.size() && ranges_[count].begin != 0U) ++count;
        return {ranges_.data(), count};
    }
    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

    void reset() noexcept override {
        rstfr_ = static_cast<u8>(ResetCause::power_on);
        swrr_  = 0U;
        reset_requested_ = false;
    }

    void tick(u64) noexcept override {}

    u8 read(u16 address) noexcept override {
        if (address == desc_.rstfr_address) return rstfr_;
        if (address == desc_.swrr_address)  return swrr_;
        return 0U;
    }

    void write(u16 address, u8 value) noexcept override {
        if (address == desc_.rstfr_address) {
            rstfr_ &= ~value;
        } else if (address == desc_.swrr_address && value != 0U) {
            swrr_ = value;
            rstfr_ |= static_cast<u8>(ResetCause::software);
            reset_requested_ = true;
        }
    }

    void set_reset_cause(u8 cause_flags) noexcept { rstfr_ = cause_flags; }
    [[nodiscard]] u8 reset_flags() const noexcept { return rstfr_; }
    [[nodiscard]] bool software_reset_requested() const noexcept { return reset_requested_; }
    void acknowledge_reset() noexcept { reset_requested_ = false; }

private:
    const RstctrlDescriptor desc_;
    std::array<AddressRange, 2> ranges_{};
    u8 rstfr_{static_cast<u8>(ResetCause::power_on)};
    u8 swrr_{0U};
    bool reset_requested_{false};
};

} // namespace vioavr::core
