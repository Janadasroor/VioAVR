#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <array>

namespace vioavr::core {

class MemoryBus;

class Mvio final : public IoPeripheral {
public:
    explicit Mvio(const MvioDescriptor& desc) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }

    [[nodiscard]] std::string_view name() const noexcept override { return "MVIO"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] bool wants_tick() const noexcept override { return true; }

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    void set_vddio2(double voltage) noexcept { vddio2_ = voltage; }

private:
    void update_interrupt_pending() noexcept;

    MvioDescriptor desc_;
    std::array<AddressRange, 1> ranges_{};
    MemoryBus* bus_{};

    u8 intctrl_{0};
    u8 intflags_{0};
    u8 status_{0};
    bool int_pending_{false};

    double vddio2_{0.0};
    bool prev_vddio2_ok_{false};
};

} // namespace vioavr::core
