#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/pin_mux.hpp"
#include <array>
#include <string>
#include <string_view>

namespace vioavr::core {

class MemoryBus;

class Ircom final : public IoPeripheral {
public:
    explicit Ircom(std::string_view name, const IrcomDescriptor& desc) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override { return name_; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] bool wants_tick() const noexcept override { return true; }

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] bool on_external_pin_change(u16 pin_address, u8 bit_index, PinLevel level) noexcept override;

    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    [[nodiscard]] bool modulator_state() const noexcept { return modulator_high_; }

    void set_pin_mux(PinMux* pm) noexcept { pin_mux_ = pm; }
    void set_output_pin(u8 port_idx, u8 bit_idx) noexcept {
        port_idx_ = port_idx;
        bit_idx_ = bit_idx;
    }
    void set_memory_bus(MemoryBus* bus) noexcept { bus_ = bus; }

    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

    // Test/observation helpers
    [[nodiscard]] u8 rx_data() const noexcept { return rx_shift_reg_; }
    [[nodiscard]] u8 rx_bit_count() const noexcept { return rx_bit_count_; }
    [[nodiscard]] u64 rx_pulse_counter() const noexcept { return rx_pulse_counter_; }
    [[nodiscard]] bool rx_busy() const noexcept { return rx_busy_; }

private:
    void drive_output() noexcept;
    void check_rx_pulse() noexcept;

    std::string name_;
    IrcomDescriptor desc_;
    std::array<AddressRange, 1> ranges_{};
    u8 num_ranges_{0};

    PinMux* pin_mux_{nullptr};
    MemoryBus* bus_{nullptr};
    u8 port_idx_{0xFFU};
    u8 bit_idx_{0xFFU};

    u8 ctrl_{};
    u8 txplctrl_{};
    u8 rxplctrl_{};

    u32 modulator_counter_{};
    bool modulator_high_{};
    bool last_driven_level_{false};

    // RX state
    u64 rx_current_cycle_{};
    u64 rx_last_cycle_{};
    bool rx_last_level_{false};
    bool rx_busy_{false};
    u64 rx_pulse_counter_{};
    u8 rx_shift_reg_{};
    u8 rx_bit_count_{};
};

} // namespace vioavr::core
