#pragma once

#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <string>
#include <array>

namespace vioavr::core {

class PortMux;

class Spi8x : public IoPeripheral {
public:
    explicit Spi8x(const Spi8xDescriptor& descriptor) noexcept;
    virtual ~Spi8x() override = default;

    virtual std::string_view name() const noexcept override { return "SPI8X"; }
    virtual std::span<const AddressRange> mapped_ranges() const noexcept override;
    virtual ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

    virtual void reset() noexcept override;
    virtual void tick(u64 elapsed_cycles) noexcept override;
    virtual u8 read(u16 address) noexcept override;
    virtual void write(u16 address, u8 value) noexcept override;
    virtual bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    virtual bool consume_interrupt_request(InterruptRequest& request) noexcept override;
    
    void set_event_system(class EventSystem* evsys) noexcept;
    void set_port_mux(class PortMux* pm) noexcept { port_mux_ = pm; }

private:
    const Spi8xDescriptor desc_;
    class EventSystem* evsys_ {nullptr};
    class PortMux* port_mux_ {nullptr};
    std::array<AddressRange, 2> ranges_{};

    u8 ctrla_{0U};
    u8 ctrlb_{0U};
    u8 intctrl_{0U};
    u8 intflags_{0U};
    u8 data_{0U};

    bool transfer_in_progress_{false};
    u64 transfer_cycles_elapsed_{0};
    u64 transfer_duration_{16U}; // Default 8 bits @ /2

    u8 tx_buffer_{0U};
    u8 shift_register_{0U};
    bool tx_buffer_full_{false};

    static constexpr u8 CTRLA_ENABLE = 0x01U;
    static constexpr u8 CTRLA_MASTER = 0x20U;
    static constexpr u8 INTCTRL_IE = 0x01U;
    static constexpr u8 INTFLAGS_IF = 0x80U;
    static constexpr u8 INTFLAGS_WRCOL = 0x40U;
};

} // namespace vioavr::core
