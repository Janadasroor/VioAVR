#pragma once

#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <string>
#include <array>

namespace vioavr::core {

class Uart8x : public IoPeripheral {
public:
    explicit Uart8x(const Uart8xDescriptor& descriptor) noexcept;
    virtual ~Uart8x() override = default;

    virtual std::string_view name() const noexcept override { return "USART8X"; }
    virtual std::span<const AddressRange> mapped_ranges() const noexcept override;
    virtual ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

    virtual void reset() noexcept override;
    virtual void tick(u64 elapsed_cycles) noexcept override;
    virtual u8 read(u16 address) noexcept override;
    virtual void write(u16 address, u8 value) noexcept override;
    virtual bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    virtual bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    // External Interface
    void inject_received_byte(u8 data) noexcept;
    bool consume_transmitted_byte(u8& data) noexcept;

private:
    struct RxBufferEntry {
        u8 data;
        u8 high; // Contains error bits
    };

    const Uart8xDescriptor desc_;
    std::array<AddressRange, 4> ranges_{};

    u8 ctrla_{0U};
    u8 ctrlb_{0U};
    u8 ctrlc_{0U};
    u8 ctrld_{0U};
    u8 status_{0x20U}; // DREIF initially set
    u16 baud_{0U};
    u8 dbgctrl_{0U};

    std::array<RxBufferEntry, 2> rx_fifo_{};
    u8 rx_fifo_count_{0};
    u8 rx_fifo_read_idx_{0};
    u8 rx_fifo_write_idx_{0};

    bool tx_in_progress_{false};
    u64 tx_cycles_elapsed_{0};
    u64 tx_duration_{160U};

    // Bits in STATUS
    static constexpr u8 STATUS_RXCIF = 0x80U;
    static constexpr u8 STATUS_TXCIF = 0x40U;
    static constexpr u8 STATUS_DREIF = 0x20U;
    static constexpr u8 STATUS_RXSIF = 0x10U;

    // Bits in RXDATAH
    static constexpr u8 RXDATAH_RXCIF = 0x80U;
    static constexpr u8 RXDATAH_BUFOVF = 0x40U;
    static constexpr u8 RXDATAH_FERR = 0x04U;
    static constexpr u8 RXDATAH_PERR = 0x02U;
    static constexpr u8 RXDATAH_DATA8 = 0x01U;

    // Bits in CTRLA
    static constexpr u8 CTRLA_RXCIE = 0x80U;
    static constexpr u8 CTRLA_TXCIE = 0x40U;
    static constexpr u8 CTRLA_DREIE = 0x20U;

    // Bits in CTRLB
    static constexpr u8 CTRLB_RXEN = 0x80U;
    static constexpr u8 CTRLB_TXEN = 0x40U;
    static constexpr u8 CTRLB_RXMODE_MASK = 0x06U;
};

} // namespace vioavr::core
