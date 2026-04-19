#pragma once

#include "vioavr/core/types.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"

namespace vioavr::core {

class MemoryBus;

/**
 * @brief 16-bit Timer/Counter Type B (TCB)
 * Optimized for input capture and PWM in modern AVR devices.
 */
class Tcb : public IoPeripheral {
public:
    explicit Tcb(std::string name, const TcbDescriptor& desc);

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] std::string_view name() const noexcept override { return name_; }
    u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;
    
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;
    
    [[nodiscard]] bool get_wo_level() const noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_event_system(EventSystem* evsys) noexcept override;

private:
    std::string name_;
    const TcbDescriptor desc_;
    MemoryBus* bus_ {nullptr};
    EventSystem* evsys_ {nullptr};

    void on_event() noexcept;

    // Registers
    u8 ctrla_ {0x00};
    u8 ctrlb_ {0x00};
    u8 evctrl_ {0x00};
    u8 intctrl_ {0x00};
    u8 intflags_ {0x00};
    u8 status_ {0x00};
    u8 dbgctrl_ {0x00};
    u8 temp_ {0x00};

    u16 cnt_ {0x0000};
    u16 ccmp_ {0x0000};

    std::array<AddressRange, 8> ranges_ {};

    // Internal logic
    void handle_matches();
    void perform_tick(bool clock_event = true);
    bool is_enabled() const noexcept { return ctrla_ & 0x01; }
    u8 get_mode() const noexcept { return ctrlb_ & 0x07; }
    u8 get_clksel() const noexcept { return (ctrla_ >> 1) & 0x03; }

    u8 prescaler_counter_ {0};
};

} // namespace vioavr::core
