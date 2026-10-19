#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/pin_mux.hpp"
#include <array>
#include <string>
#include <string_view>
#include <deque>

namespace vioavr::core {

// Optional partner for devices where a USART and an EUSART are two modes of
// the same physical peripheral (AT90PWM216/316 datasheet 18.6.4). The Uart
// keeps owning UCSRA/UDR; the EUSART engine is told when the shared data
// register is read or written so that EUDR stays the high byte and the
// EUCSRC status flags expire on the buffer read.
class UartModePartner {
public:
    virtual ~UartModePartner() = default;
    // The receive buffer was read: EUCSRC status bits are no longer valid.
    virtual void on_receive_buffer_read() noexcept = 0;
    // A byte was written to the shared data register. Returns true when the
    // partner consumed it (EUSART mode), false to use the classic path.
    virtual bool on_transmit_data(u8 low_byte) noexcept = 0;
    // True when the partner runs the peripheral instead of this USART: the
    // two are one peripheral in two modes (AT90PWM216/316 datasheet 18.6.4,
    // "Set to enable the EUSART mode, clear to operate as standard USART"),
    // so only one of the two engines may touch the pins and the buffer.
    [[nodiscard]] virtual bool owns_peripheral() const noexcept { return false; }
};

class Uart final : public IoPeripheral {
public:
    explicit Uart(std::string_view name, const UartDescriptor& descriptor, PinMux& pin_mux) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_mode_partner(UartModePartner* partner) noexcept { partner_ = partner; }

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] const UartDescriptor& descriptor() const noexcept { return desc_; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;
    [[nodiscard]] bool supports_interrupt_mask() const noexcept override { return true; }

    [[nodiscard]] ClockDomain clock_domain() const noexcept override;

    // Inject a received frame. frame_error = stop bit sampled low (FE);
    // parity_error = received parity bit wrong (UPE, only when the USART
    // mode enables parity, UPM1:0 != 00). Flags latch with RXC at reception.
    bool inject_received_byte(u16 data, bool frame_error = false,
                              bool parity_error = false) noexcept;
    bool consume_transmitted_byte(u16& data) noexcept;
    [[nodiscard]] bool has_tx_byte() const noexcept { return !tx_output_queue_.empty(); }
    [[nodiscard]] u16 read_tx_byte() noexcept {
        u16 data{};
        if (!tx_output_queue_.empty()) {
            data = tx_output_queue_.front();
            tx_output_queue_.pop_front();
        }
        return data;
    }

    // ---- Shared USART registers ----
    // On devices where a USART and an EUSART are two modes of the same
    // physical peripheral (AT90PWM216/316 datasheet 18.6.4), the EUSART
    // drives these registers instead of its own: RXC/TXC/UDRE/FE/DOR/UPE
    // live in UCSRA, and the data register holds the low byte of the
    // received character while EUDR holds the high byte.
    [[nodiscard]] u8 status() const noexcept { return ucsra_; }
    void set_status_bits(u8 mask, bool set) noexcept {
        ucsra_ = set ? (ucsra_ | mask) : (ucsra_ & static_cast<u8>(~mask));
        update_interrupt_state();
    }
    // Hand a received character to the shared data register.
    void set_rx_word(u16 word) noexcept {
        udr_rx_ = word; // widened: the EUSART shares this register
        ucsra_ |= desc_.rxc_mask;
        update_interrupt_state();
    }
    [[nodiscard]] u16 rx_word() const noexcept { return udr_rx_; }

private:
    std::string name_;
    UartDescriptor desc_;
    PinMux* pin_mux_;
    MemoryBus* bus_ {};
    UartModePartner* partner_ {nullptr};
    std::array<AddressRange, 4> ranges_;

    u16 udr_rx_ {}; // UDR is 8 bits, but the EUSART shares it as the low byte
    u8 udr_tx_ {};
    u8 ucsra_ {};
    u8 ucsrb_ {};
    u8 ucsrc_ {};
    u8 ubrrh_ {};
    u8 ubrrl_ {};

    // Bit-level simulation
    bool tx_active_ {};
    u16 tx_shift_reg_ {};
    u8 tx_bits_left_ {};
    u64 tx_cycle_accumulator_ {};
    u64 tx_bit_duration_ {1000};

    bool rx_active_ {};
    u16 rx_shift_reg_ {};
    u8 rx_bits_left_ {};
    u64 rx_cycle_accumulator_ {};
    bool rx_frame_error_ {};
    bool rx_parity_error_ {};
    bool tx_buffer_full_ {};

    // RXD pin sampling (RXEN active). The pin is read, never driven: the
    // USART owns no output on RXD.
    enum class RxPinState : u8 { idle, start, data, parity, stop };
    RxPinState rx_pin_state_ {RxPinState::idle};
    u8 rx_bit_index_ {0};
    u8 rx_parity_bits_ {0};
    u8 rx_stop_bits_ {1};
    u64 rx_sample_in_ {0};
    bool rx_pin_last_ {true};

    std::deque<u16> tx_output_queue_;

    [[nodiscard]] bool power_reduction_enabled() const noexcept;
    void update_interrupt_state() noexcept;
};

}  // namespace vioavr::core
 
