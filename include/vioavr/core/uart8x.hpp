#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include <array>
#include <string>

namespace vioavr::core {

class Uart8x final : public IoPeripheral {
public:
    explicit Uart8x(const Uart8xDescriptor& descriptor, PinMux& pin_mux) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override { return "UART8X"; }
    [[nodiscard]] const Uart8xDescriptor& descriptor() const noexcept { return desc_; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    void set_memory_bus(class MemoryBus* bus) noexcept override { bus_ = bus; }

    // Stream interface
    void inject_received_byte(u8 data, bool bit9 = false) noexcept;
    bool consume_transmitted_byte(u16& data) noexcept;

private:
    Uart8xDescriptor desc_;
    PinMux* pin_mux_;
    class MemoryBus* bus_ {nullptr};
    std::array<AddressRange, 8> ranges_;

    u8 ctrla_ {};
    u8 ctrlb_ {};
    u8 ctrlc_ {};
    u8 ctrld_ {};
    u16 baud_ {};
    u8 status_ {};
    u8 dbgctrl_ {};
    
    u8 tx_data_buffer_ {};
    u8 txdatah_ {};
    bool tx_data_busy_ {};

    struct FifoEntry {
        u8 data;
        u8 high;
    };
    FifoEntry rx_fifo_[2] {};
    u8 rx_fifo_write_idx_ {0};
    u8 rx_fifo_read_idx_ {0};
    u8 rx_fifo_count_ {0};

    // Bit-level simulation state
    bool tx_in_progress_ {};
    u16 tx_shift_reg_ {};
    u8 tx_bits_left_ {};
    u8 tx_total_bits_ {};
    double tx_bit_duration_ {};
    double tx_cycle_accumulator_ {};

    bool rx_in_progress_ {};
    u16 rx_shift_reg_ {};
    u8 rx_bits_left_ {};
    u8 rx_total_bits_ {};
    double rx_bit_duration_ {};
    double rx_cycle_accumulator_ {};

    static constexpr u8 STATUS_RXCIF = 0x80;
    static constexpr u8 STATUS_TXCIF = 0x40;
    static constexpr u8 STATUS_DREIF = 0x20;
    static constexpr u8 STATUS_RXSIF = 0x10;
    static constexpr u8 STATUS_IS_BUSY = 0x01;

    static constexpr u8 CTRLB_RXEN = 0x80;
    static constexpr u8 CTRLB_TXEN = 0x40;
    static constexpr u8 CTRLB_LBME = 0x08;
    static constexpr u8 CTRLB_RXMODE_MASK = 0x06;
    static constexpr u8 CTRLB_MPCM = 0x01;

    static constexpr u8 CTRLA_RXCIE = 0x80;
    static constexpr u8 CTRLA_TXCIE = 0x40;
    static constexpr u8 CTRLA_DREIE = 0x20;
    static constexpr u8 CTRLA_LBME = 0x01; // Note: In CTRLA not CTRL B on some?

    static constexpr u8 CTRLC_PMODE_MASK = 0x30;
    static constexpr u8 CTRLC_SBMODE = 0x08;
    static constexpr u8 RXDATAH_RXCIF = 0x80;
    static constexpr u8 RXDATAH_BUFOVF = 0x40;
    static constexpr u8 RXDATAH_FERR = 0x04;
    static constexpr u8 RXDATAH_PERR = 0x02;
    static constexpr u8 RXDATAH_DATA8 = 0x01;

    void update_pin_ownership() noexcept;
    void actually_push_to_fifo(u8 data, bool bit9) noexcept;
};

} // namespace vioavr::core
 
