#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <array>

namespace vioavr::core {

class GpioPort;

class PinChangeInterrupt final : public IoPeripheral {
public:
    PinChangeInterrupt(std::string_view name, const PinChangeInterruptDescriptor& desc, GpioPort& port) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    void notify_pin_change(u8 mask) noexcept;

private:
    std::string_view name_;
    PinChangeInterruptDescriptor desc_;
    std::array<AddressRange, 3> ranges_;
    GpioPort& port_;

    u8 pcicr_ {};
    u8 pcifr_ {};
    u8 pcmsk_ {};
    bool interrupt_pending_ {};
    u8 last_pin_state_ {};
};

}  // namespace vioavr::core
