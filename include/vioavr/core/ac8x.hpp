#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <array>
#include <string>

namespace vioavr::core {

class MemoryBus;
class EventSystem;
class AnalogSignalBank;

/**
 * @brief Modern AVR8X Analog Comparator (AC)
 */
class Ac8x : public IoPeripheral {
public:
    explicit Ac8x(const Ac8xDescriptor& desc) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_event_system(EventSystem* evsys) noexcept override;
    void set_analog_signal_bank(AnalogSignalBank* bank) noexcept { signal_bank_ = bank; }

    [[nodiscard]] std::string_view name() const noexcept override { return "AC"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override { return {ranges_.data(), 1}; }

    void tick(u64 elapsed_cycles) noexcept override;
    void reset() noexcept override;

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

private:
    Ac8xDescriptor desc_;
    MemoryBus* bus_ {nullptr};
    EventSystem* evsys_ {nullptr};
    AnalogSignalBank* signal_bank_ {nullptr};

    std::array<AddressRange, 1> ranges_ {};

    // Registers
    u8 ctrla_ {0};
    u8 muxctrla_ {0};
    u8 dacctrla_ {0};
    u8 intctrl_ {0};
    u8 status_ {0};

    bool is_enabled() const noexcept { return (ctrla_ & 0x01U); }
};

} // namespace vioavr::core
