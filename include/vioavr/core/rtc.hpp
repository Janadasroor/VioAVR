#pragma once

#include "vioavr/core/types.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"

namespace vioavr::core {

class MemoryBus;

/**
 * @brief Real-Time Counter (RTC) with PIT support.
 * Used for low-power timing and periodic interrupts.
 */
class Rtc : public IoPeripheral {
public:
    explicit Rtc(const RtcDescriptor& desc);

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] std::string_view name() const noexcept override { return "RTC"; }
    u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;
    
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_event_system(EventSystem* evsys) noexcept override { evsys_ = evsys; }

private:
    const RtcDescriptor desc_;
    MemoryBus* bus_ {nullptr};
    EventSystem* evsys_ {nullptr};

    // Main Counter Registers
    u8 ctrla_ {0x00};
    u8 status_ {0x00};
    u8 intctrl_ {0x00};
    u8 intflags_ {0x00};
    u8 temp_ {0x00};
    u8 dbgctrl_ {0x00};
    u8 clksel_ {0x00};
    
    u16 cnt_ {0x0000};
    u16 per_ {0x0000};
    u16 cmp_ {0x0000};

    // PIT Registers
    u8 pitctrla_ {0x00};
    u8 pitstatus_ {0x00};
    u8 pitintctrl_ {0x00};
    u8 pitintflags_ {0x00};

    std::array<AddressRange, 2> ranges_ {};

    // Internal logic
    u64 internal_ticks_ {0};
    u64 pit_ticks_ {0};
    
    void handle_pit_tick();
    void handle_rtc_tick();
    
    bool is_rtc_enabled() const noexcept { return ctrla_ & 0x01; }
    bool is_pit_enabled() const noexcept { return pitctrla_ & 0x01; }
    u16 get_prescaler() const noexcept;
    u16 get_pit_period() const noexcept;
};

} // namespace vioavr::core
