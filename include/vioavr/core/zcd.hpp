#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"

namespace vioavr::core {

/**
 * @brief Zero Cross Detector (ZCD) peripheral.
 */
class Zcd final : public IoPeripheral {
public:
    explicit Zcd(const ZcdDescriptor& desc) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override { return "ZCD"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    [[nodiscard]] bool on_external_pin_change(u16 pin_address, u8 bit_index, PinLevel level) noexcept override;

private:
    ZcdDescriptor desc_;
    std::array<AddressRange, 1> ranges_{};
    
    u8 ctrla_{0};
    bool pin_state_{false};
    bool int_pending_{false};
};

} // namespace vioavr::core
