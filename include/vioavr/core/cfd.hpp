#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <array>

namespace vioavr::core {

class MemoryBus;

class Cfd final : public IoPeripheral {
public:
    explicit Cfd(const CfdDescriptor& desc) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }

    [[nodiscard]] std::string_view name() const noexcept override { return "CFD"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] bool wants_tick() const noexcept override { return true; }

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    void on_edge() noexcept;

private:
    void update_interrupt_pending() noexcept;

    CfdDescriptor desc_;
    std::array<AddressRange, 1> ranges_{};
    MemoryBus* bus_{};

    u8 xfdcsr_{0};
    u8 clkdiv_{0};
    u8 intflags_{0};
    u8 intctrl_{0};
    bool int_pending_{false};

    u64 edge_counter_{0};
};

} // namespace vioavr::core
