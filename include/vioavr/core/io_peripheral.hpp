#pragma once

#include "vioavr/core/types.hpp"

#include <span>
#include <string_view>

namespace vioavr::core {

enum class ClockDomain : u8 {
    cpu = 0x01,
    io = 0x02,
    adc = 0x04,
    async_timer = 0x08,
    watchdog = 0x10,
    none = 0x00,
    all = 0xFF
};

class MemoryBus;
class InterruptRequest;
class EventSystem;

class IoPeripheral {
public:
    virtual ~IoPeripheral() = default;

    virtual void set_memory_bus(MemoryBus* bus) noexcept { bus_ = bus; }
    virtual void set_event_system(EventSystem* evsys) noexcept { (void)evsys; }

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual std::span<const AddressRange> mapped_ranges() const noexcept = 0;

    [[nodiscard]] virtual ClockDomain clock_domain() const noexcept
    {
        return ClockDomain::io;
    }

    virtual void reset() noexcept = 0;
    virtual void tick(u64 elapsed_cycles) noexcept = 0;
    [[nodiscard]] virtual bool wants_tick() const noexcept { return true; }

    virtual void sync() noexcept {}
    virtual void on_power_state_change() noexcept {}

    [[nodiscard]] virtual u8 read(u16 address) noexcept = 0;
    virtual void write(u16 address, u8 value) noexcept = 0;
    
    [[nodiscard]] virtual bool on_external_pin_change(u16 pin_address, u8 bit_index, PinLevel level) noexcept
    {
        (void)pin_address;
        (void)bit_index;
        (void)level;
        return false;
    }

    virtual void on_analog_pin_change(u16 pin_address, u8 bit_index, double voltage) noexcept
    {
        (void)pin_address;
        (void)bit_index;
        (void)voltage;
    }

    [[nodiscard]] virtual bool consume_pin_change(PinStateChange& change) noexcept
    {
        (void)change;
        return false;
    }
    [[nodiscard]] virtual bool pending_interrupt_request(InterruptRequest& request) const noexcept
    {
        (void)request;
        return false;
    }
    [[nodiscard]] virtual bool consume_interrupt_request(InterruptRequest& request) noexcept
    {
        (void)request;
        return false;
    }
    
    [[nodiscard]] bool interrupt_pending() const noexcept { return interrupt_pending_; }
    void set_interrupt_pending(bool pending) noexcept;

    virtual void on_reti() noexcept {}
    
    [[nodiscard]] virtual bool check_twi_address(u8 address) const noexcept
    {
        (void)address;
        return false;
    }

    [[nodiscard]] virtual bool supports_interrupt_mask() const noexcept { return false; }

protected:
    class MemoryBus* bus_ {nullptr};
    u8 bus_id_ {0xFFU};

private:
    bool interrupt_pending_ {false};
    friend class MemoryBus;
};

}  // namespace vioavr::core
