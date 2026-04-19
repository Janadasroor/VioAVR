#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/pin_mux.hpp"
#include <array>
#include <string>
#include <string_view>

namespace vioavr::core {

class Uart final : public IoPeripheral {
public:
    explicit Uart(std::string_view name, const UartDescriptor& descriptor, PinMux& pin_mux) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] const UartDescriptor& descriptor() const noexcept { return desc_; }
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
    PinMux* pin_mux_;
    std::array<AddressRange, 4> ranges_;

    u8 udr_rx_ {};
    u8 udr_tx_ {};
    u8 ucsra_ {};
    u8 ucsrb_ {};
    u8 ucsrc_ {};
    u8 ubrrh_ {};
    u8 ubrrl_ {};

    // Bit-level simulation
    bool tx_active_ {};
    u16 tx_shift_reg_ {};
    u8 tx_bits_left_ {};
    u64 tx_cycle_accumulator_ {};
    u64 tx_bit_duration_ {1000};

    bool rx_active_ {};
    u16 rx_shift_reg_ {};
    u8 rx_bits_left_ {};
    u64 rx_cycle_accumulator_ {};

    void update_pin_ownership() noexcept;
};

}  // namespace vioavr::core
 
