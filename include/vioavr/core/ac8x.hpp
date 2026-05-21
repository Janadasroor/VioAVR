#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <string>
#include <array>
#include <span>

namespace vioavr::core {


class MemoryBus;
class EventSystem;
class AnalogSignalBank;

/**
 * @brief Modern AVR8X Analog Comparator (AC)
 */
class Ac8x : public IoPeripheral {
public:
    explicit Ac8x(std::string name, const Ac8xDescriptor& desc) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_event_system(EventSystem* evsys) noexcept override;
    void set_analog_signal_bank(AnalogSignalBank* bank) noexcept { signal_bank_ = bank; }
    void set_vdd(double vdd) noexcept { vdd_ = vdd; }
    void set_vref(double vref) noexcept { vref_ = vref; }

    [[nodiscard]] std::string_view name() const noexcept override { return name_; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override { return {ranges_.data(), 1}; }

    void tick(u64 elapsed_cycles) noexcept override;
    void reset() noexcept override;

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    [[nodiscard]] bool get_state() const noexcept { return (status_ & 0x10U) != 0; }

private:
    std::string name_;
    Ac8xDescriptor desc_;
    MemoryBus* bus_ {nullptr};
    EventSystem* evsys_ {nullptr};
    AnalogSignalBank* signal_bank_ {nullptr};
    double vdd_ {5.0};
    double vref_ {5.0};

    std::array<AddressRange, 1> ranges_ {};

    // Registers
    u8 ctrla_ {0};
    u8 muxctrla_ {0};
    u8 dacctrla_ {0xFF};
    u8 intctrl_ {0};
    u8 status_ {0};

    static constexpr u64 PROPAGATION_DELAY = 0;

    bool is_enabled() const noexcept { return (ctrla_ & (0x01U | 0x80U)); }
    void update_interrupt_state() noexcept;
    void evaluate() noexcept;

    bool pending_state_ {false};
    u64 settle_counter_ {0};
};

} // namespace vioavr::core
