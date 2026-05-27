#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"

namespace vioavr::core {

class Usb8x final : public IoPeripheral {
public:
    explicit Usb8x(const Usb8xDescriptor& desc) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override { return "USB8x"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

private:
    Usb8xDescriptor desc_;
    std::array<AddressRange, 1> ranges_{};

    u8 ctrla_{0};
    u8 ctrlb_{0};
    u8 busstate_{0};
    u8 addr_{0};
    u8 fifowp_{0};
    u8 fiforp_{0};
    u8 epptr_{0};
    u8 intctrla_{0};
    u8 intctrlb_{0};
    u8 intflagsa_{0};
    u8 intflagsb_{0};
    u8 pllcsr_{0};
};

} // namespace vioavr::core
