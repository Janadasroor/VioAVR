#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"

namespace vioavr::core {

/**
 * @brief LINUART (Local Interconnect Network UART) peripheral.
 */
class LinUART final : public IoPeripheral {
public:
    explicit LinUART(const LinDescriptor& desc) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override { return "LINUART"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

private:
    LinDescriptor desc_;
    std::array<AddressRange, 1> ranges_{};
    
    u8 lincr_{0};
    u8 linsir_{0};
    u8 linenir_{0};
    u8 linerr_{0};
    u8 linbtr_{0};
    u8 linidr_{0};
    u8 lindat_{0};
};

} // namespace vioavr::core
