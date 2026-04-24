#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <vector>

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

    // Simulation interface
    void simulate_rx_break() noexcept;
    void simulate_rx_byte(u8 data) noexcept;
    [[nodiscard]] std::vector<u8> get_tx_data() const noexcept;

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

    enum class State {
        Idle,
        SyncBreak,
        SyncField,
        Identifier,
        Data,
        Checksum
    } state_{State::Idle};

    std::vector<u8> fifo_;
    u8 fifo_idx_{0};
    u8 expected_data_len_{0};
    u8 checksum_{0};

    void update_state() noexcept;
    void calculate_checksum(u8 data) noexcept;
    u8 get_frame_length() const noexcept;
};

} // namespace vioavr::core
