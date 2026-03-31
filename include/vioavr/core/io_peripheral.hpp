#pragma once

#include "vioavr/core/types.hpp"

#include <span>
#include <string_view>

namespace vioavr::core {

class IoPeripheral {
public:
    virtual ~IoPeripheral() = default;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual std::span<const AddressRange> mapped_ranges() const noexcept = 0;

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
};

}  // namespace vioavr::core
