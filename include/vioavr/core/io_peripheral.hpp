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
    none = 0x00
};

class MemoryBus;
class InterruptRequest;
class EventSystem;

class IoPeripheral {
public:
    virtual ~IoPeripheral() = default;

    virtual void set_memory_bus(MemoryBus* bus) noexcept { (void)bus; }
    virtual void set_event_system(EventSystem* evsys) noexcept { (void)evsys; }

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual std::span<const AddressRange> mapped_ranges() const noexcept = 0;

    [[nodiscard]] virtual ClockDomain clock_domain() const noexcept
    {
        return ClockDomain::io;
    }

    virtual void reset() noexcept = 0;
    virtual void tick(u64 elapsed_cycles) noexcept = 0;

    [[nodiscard]] virtual u8 read(u16 address) noexcept = 0;
    virtual void write(u16 address, u8 value) noexcept = 0;
    
    [[nodiscard]] virtual bool on_external_pin_change(u8 bit_index, PinLevel level) noexcept
    {
        (void)bit_index;
        (void)level;
        return false;
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

    virtual void on_reti() noexcept {}
};

}  // namespace vioavr::core
