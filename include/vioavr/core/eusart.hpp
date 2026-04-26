#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <deque>
#include <string>
#include <vector>

namespace vioavr::core {

class Eusart : public IoPeripheral {
public:
    Eusart(std::string_view name, const EusartDescriptor& desc) noexcept;
    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;
    [[nodiscard]] ClockDomain clock_domain() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    // External interaction
    void inject_received_byte(u8 data) noexcept;
    bool consume_transmitted_byte(u8& data) noexcept;

private:
    std::string name_;
    EusartDescriptor desc_;
    MemoryBus* bus_ {nullptr};
    std::array<AddressRange, 4> ranges_{};

    u8 eudr_{};
    u8 eucsra_{};

    // EUSART Timing
    u64 cycles_to_next_bit_ {0};
    bool tx_active_ {false};
    u8 tx_bit_count_ {0};
    u16 tx_shift_reg_ {0};

    u8 eucsrb_{};
    u8 eucsrc_{};
    u8 mubrrl_{};
    u8 mubrrh_{};

    std::deque<u8> tx_queue_;
    std::deque<u8> rx_queue_;

    bool power_reduction_enabled() const noexcept;
};

} // namespace vioavr::core
