#pragma once

#include "vioavr/core/io_peripheral.hpp"
#include <array>

namespace vioavr::core {

class PllController final : public IoPeripheral {
public:
    explicit PllController(u16 address);

    [[nodiscard]] std::string_view name() const noexcept override { return "PLL"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

private:
    u16 address_;
    std::array<AddressRange, 1> ranges_{};
    u8 pllcsr_{0};
    u64 lock_delay_remaining_{0};
    static constexpr u64 LOCK_DELAY_CYCLES = 100; // Aligned with fidelity tests
};

} // namespace vioavr::core
