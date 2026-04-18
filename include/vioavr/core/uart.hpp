#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <array>
#include <string>
#include <string_view>

namespace vioavr::core {

class Uart final : public IoPeripheral {
public:
    explicit Uart(std::string_view name, const UartDescriptor& descriptor) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    [[nodiscard]] ClockDomain clock_domain() const noexcept override;

    void inject_received_byte(u8 data) noexcept;
    bool consume_transmitted_byte(u8& data) noexcept;

private:
    std::string name_;
    UartDescriptor desc_;
    std::array<AddressRange, 4> ranges_;

    u8 udr_rx_ {};
    u8 udr_tx_ {};
    u8 ucsra_ {};
    u8 ucsrb_ {};
    u8 ucsrc_ {};
    u8 ubrrh_ {};
    u8 ubrrl_ {};
    bool tx_in_progress_ {};
    u64 tx_cycles_elapsed_ {};
    u64 tx_duration_ {1000}; // Default bit-time in cycles
};

}  // namespace vioavr::core
