#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/pin_mux.hpp"
#include <vector>

namespace vioavr::core {

class MemoryBus;

class LinUART final : public IoPeripheral {
public:
    explicit LinUART(const LinDescriptor& desc) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_pin_mux(PinMux* pm) noexcept { pin_mux_ = pm; }
    void set_tx_pin(u16 address, u8 bit) noexcept { tx_pin_address_ = address; tx_pin_bit_ = bit; }

    [[nodiscard]] std::string_view name() const noexcept override { return "LINUART"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] bool wants_tick() const noexcept override { return true; }

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    // Simulation interface
    void simulate_rx_break() noexcept;
    void simulate_rx_byte(u8 data) noexcept;
    [[nodiscard]] std::vector<u8> get_tx_data() const noexcept;

private:
    enum class State {
        Idle,
        SyncBreak,
        SyncField,
        Identifier,
        Data,
        Checksum
    } state_{State::Idle};

    void update_state() noexcept;
    void calculate_checksum(u8 data) noexcept;
    u8 get_frame_length() const noexcept;
    void evaluate_interrupts() noexcept;
    [[nodiscard]] bool power_reduction_enabled() const noexcept;
    void update_interrupt_pending() noexcept;

    LinDescriptor desc_;
    MemoryBus* bus_{};
    PinMux* pin_mux_{};
    u16 tx_pin_address_{0};
    u8 tx_pin_bit_{0xFF};
    std::array<AddressRange, 1> ranges_{};
    bool int_pending_{false};

    u8 lincr_{0};
    u8 linsir_{0};
    u8 linenir_{0};
    u8 linerr_{0};
    u8 linbtr_{0};
    u8 linidr_{0};
    u8 lindat_{0};

    std::vector<u8> fifo_;
    u8 fifo_idx_{0};
    u8 expected_data_len_{0};
    u8 checksum_{0};

    u64 bit_timer_{0};
    u8 tx_bit_pos_{0};
    u16 tx_shift_reg_{0};
    bool tx_active_{false};
    u16 rx_shift_reg_{0};
    u8 rx_bits_left_{0};
    bool rx_active_{false};
    u8 rx_current_byte_{0};
    bool rx_data_valid_{false};
};

} // namespace vioavr::core
