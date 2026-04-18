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
    explicit Tcb(const TcbDescriptor& desc);

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] std::string_view name() const noexcept override { return "TCB"; }
    u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;
    
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }

private:
    const TcbDescriptor desc_;
    MemoryBus* bus_ {nullptr};

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

    std::array<AddressRange, 2> ranges_ {};

    // Internal logic
    void handle_matches();
    void perform_tick();
    bool is_enabled() const noexcept { return ctrla_ & 0x01; }
    u8 get_mode() const noexcept { return ctrlb_ & 0x07; }
};

} // namespace vioavr::core
