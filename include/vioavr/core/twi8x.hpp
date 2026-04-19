#pragma once

#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <string>
#include <array>

namespace vioavr::core {

class Twi8x : public IoPeripheral {
public:
    explicit Twi8x(const Twi8xDescriptor& descriptor) noexcept;
    virtual ~Twi8x() override = default;

    virtual std::string_view name() const noexcept override { return "TWI8X"; }
    virtual std::span<const AddressRange> mapped_ranges() const noexcept override;
    virtual ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

    virtual void reset() noexcept override;
    virtual void tick(u64 elapsed_cycles) noexcept override;
    virtual u8 read(u16 address) noexcept override;
    virtual void write(u16 address, u8 value) noexcept override;
    virtual bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    virtual bool consume_interrupt_request(InterruptRequest& request) noexcept override;

private:
    const Twi8xDescriptor desc_;
    std::array<AddressRange, 4> ranges_{};

    // Host
    u8 mctrla_{0U};
    u8 mctrlb_{0U};
    u8 mstatus_{0U};
    u8 mbaud_{0U};
    u8 maddr_{0U};
    u8 mdata_{0U};

    // Client
    u8 sctrla_{0U};
    u8 sctrlb_{0U};
    u8 sstatus_{0U};
    u8 saddr_{0U};
    u8 sdata_{0U};
    u8 saddrmask_{0U};

    u8 dbgctrl_{0U};
    
    u64 cycle_counter_{0};
    u64 bit_duration_{0};
    u8 bits_left_{0};

    // MSTATUS Bits
    static constexpr u8 MSTATUS_RIF = 0x80U;
    static constexpr u8 MSTATUS_WIF = 0x40U;
    static constexpr u8 MSTATUS_CLKHOLD = 0x20U;
    static constexpr u8 MSTATUS_RXACK = 0x10U;
    static constexpr u8 MSTATUS_ARBLOST = 0x08U;
    static constexpr u8 MSTATUS_BUSERR = 0x04U;
    static constexpr u8 MSTATUS_BUSSTATE_MASK = 0x03U;

    // SSTATUS Bits
    static constexpr u8 SSTATUS_DIF = 0x80U;
    static constexpr u8 SSTATUS_APIF = 0x40U;
    static constexpr u8 SSTATUS_CLKHOLD = 0x20U;
    static constexpr u8 SSTATUS_RXACK = 0x10U;
    static constexpr u8 SSTATUS_COLL = 0x08U;
    static constexpr u8 SSTATUS_BUSERR = 0x04U;
    static constexpr u8 SSTATUS_DIR = 0x02U;
    static constexpr u8 SSTATUS_AP = 0x01U;
};

} // namespace vioavr::core
