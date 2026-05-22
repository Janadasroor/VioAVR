#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"

namespace vioavr::core {

class Cfd final : public IoPeripheral {
public:
    explicit Cfd(const CfdDescriptor& desc) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override { return "CFD"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    void trigger_failure() noexcept;

private:
    void update_interrupt_pending() noexcept;

    CfdDescriptor desc_;
    std::array<AddressRange, 1> ranges_{};

    u8 xfdcsr_{0};
};

} // namespace vioavr::core
