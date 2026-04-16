#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <array>

namespace vioavr::core {

class Spi final : public IoPeripheral {
public:
    explicit Spi(std::string_view name, const SpiDescriptor& descriptor) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    [[nodiscard]] ClockDomain clock_domain() const noexcept override;

    // Host-side API for SPI simulation
    void inject_miso_byte(u8 value) noexcept;
    void trigger_slave_transfer() noexcept;
    [[nodiscard]] u8 last_transmitted_byte() const noexcept;
    [[nodiscard]] bool busy() const noexcept;

private:
    void complete_transfer() noexcept;

    std::string_view name_;
    SpiDescriptor desc_;
    std::array<AddressRange, 1> ranges_;

    u8 spcr_ {};
    u8 spsr_ {};
    u8 spdr_ {};
    u8 shift_register_ {};
    u8 miso_buffer_ {};
    
    u32 transfer_cycles_left_ {};
    bool interrupt_pending_ {};
};

}  // namespace vioavr::core
