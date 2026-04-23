#pragma once

#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/types.hpp"
#include <string>

namespace vioavr::core {

enum class TwiPhase : u8 {
    idle,
    address,
    write_data,
    read_data
};

enum class TwiSlavePhase : u8 {
    idle = 0,
    addr = 1,
    ack_setup = 2,
    ack_pulse = 3,
    ack_hold = 4,
    rx_data = 5,
    tx_data = 6
};

class Twi8x : public IoPeripheral {
public:
    Twi8x(const Twi8xDescriptor& desc) noexcept;
    ~Twi8x() override = default;

    std::string_view name() const noexcept override { return "TWI8X"; }
    std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    void set_port_mux(class PortMux* port_mux) noexcept;
    void set_event_system(class EventSystem* evsys) noexcept;

    void inject_bus_start() noexcept;
    void inject_bus_address(u8 address) noexcept;
    void inject_bus_data(u8 data) noexcept;
    void inject_bus_stop() noexcept;

private:
    void tick_master_core() noexcept;

    const Twi8xDescriptor desc_;
    class EventSystem* evsys_ {nullptr};
    class PortMux* port_mux_ {nullptr};
    std::array<AddressRange, 4> ranges_{};

    // Common registers
    u8 dbgctrl_{0};

    // Master registers
    u8 mctrla_{0};
    u8 mctrlb_{0};
    u8 mstatus_{0};
    u8 mbaud_{0};
    u8 maddr_{0};
    u8 mdata_{0};

    // Slave registers
    u8 sctrla_{0};
    u8 sctrlb_{0};
    u8 sstatus_{0};
    u8 saddr_{0};
    u8 sdata_{0};
    u8 saddrmask_{0};

    // Master internal state
    u64 cycle_counter_{0};
    u64 bit_duration_{0};
    u8 bits_left_{0};
    u8 shift_register_{0};
    u8 data_read_accumulator_{0};
    TwiPhase host_phase_{TwiPhase::idle};

    // Signal monitoring
    PinLevel prev_scl_{PinLevel::high};
    PinLevel prev_sda_{PinLevel::high};
    PinLevel last_intended_sda_{PinLevel::high};
    u8 slave_bits_left_{0};
    u8 slave_shift_register_{0};
    TwiSlavePhase slave_phase_{TwiSlavePhase::idle};

    // Constants
    static constexpr u8 MSTATUS_RIF = 0x80U;
    static constexpr u8 MSTATUS_WIF = 0x40U;
    static constexpr u8 MSTATUS_CLKHOLD = 0x20U;
    static constexpr u8 MSTATUS_RXACK = 0x10U;
    static constexpr u8 MSTATUS_ARBLOST = 0x08U;
    static constexpr u8 MSTATUS_BUSERR = 0x04U;
    static constexpr u8 MSTATUS_BUSSTATE_MASK = 0x03U;

    static constexpr u8 MCTRLA_SMEN = 0x02U;
    static constexpr u8 MCTRLA_ENABLE = 0x01U;

    static constexpr u8 MCTRLB_MCMD_MASK = 0x03U;
    static constexpr u8 MCTRLB_MCMD_NOOP = 0x00U;
    static constexpr u8 MCTRLB_MCMD_REPSTART = 0x01U;
    static constexpr u8 MCTRLB_MCMD_RECVTRANS = 0x02U;
    static constexpr u8 MCTRLB_MCMD_STOP = 0x03U;
    static constexpr u8 MCTRLB_ACKACT = 0x04U;

    static constexpr u8 SSTATUS_DIF = 0x80U;
    static constexpr u8 SSTATUS_APIF = 0x40U;
    static constexpr u8 SSTATUS_CLKHOLD = 0x20U;
    static constexpr u8 SSTATUS_RXACK = 0x10U;
    static constexpr u8 SSTATUS_COLL = 0x08U;
    static constexpr u8 SSTATUS_BUSERR = 0x04U;
    static constexpr u8 SSTATUS_DIR = 0x02U;
    static constexpr u8 SSTATUS_AP = 0x01U;

    static constexpr u8 SCTRLB_ACKACT = 0x04U;
    static constexpr u8 SCTRLB_SCMD_MASK = 0x03U;
    static constexpr u8 SCTRLB_SCMD_COMPTRANS = 0x02U;
};

} // namespace vioavr::core
