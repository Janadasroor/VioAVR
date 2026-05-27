#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"

namespace vioavr::core {

class Mvio final : public IoPeripheral {
public:
    explicit Mvio(const MvioDescriptor& desc) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override { return "MVIO"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

private:
    MvioDescriptor desc_;
    std::array<AddressRange, 1> ranges_{};

    u8 intctrl_{0};
    u8 intflags_{0};
    u8 status_{0};
};

} // namespace vioavr::core
