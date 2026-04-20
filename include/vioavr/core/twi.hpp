#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <vector>
#include <array>
#include <string>

namespace vioavr::core {

class Twi final : public IoPeripheral {
public:
    explicit Twi(std::string_view name, const TwiDescriptor& descriptor) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    // Host-side API for TWI simulation
    void set_rx_buffer(const std::vector<u8>& data) noexcept;
    [[nodiscard]] const std::vector<u8>& tx_buffer() const noexcept;
    [[nodiscard]] bool busy() const noexcept;
    [[nodiscard]] bool check_slave_address(u8 address) const noexcept;

private:
    void complete_step() noexcept;
    void handle_master_step() noexcept;
    void handle_slave_step() noexcept;

    std::string name_;
    TwiDescriptor desc_;
    std::array<AddressRange, 1> ranges_;

    enum class Mode {
        idle,
        master_tx,
        master_rx,
        slave_tx,
        slave_rx
    };

    u8 twbr_ {};
    u8 twsr_ {};
    u8 twar_ {};
    u8 twdr_ {};
    u8 twcr_ {};
    u8 twamr_ {};
    
    Mode mode_ {Mode::idle};
    u32 step_cycles_left_ {};
    bool interrupt_pending_ {};
    bool sla_matched_ {false};
    bool general_call_matched_ {false};
    
    std::vector<u8> rx_buffer_ {};
    std::size_t rx_idx_ {0};
    std::vector<u8> tx_buffer_ {};
};

}  // namespace vioavr::core
