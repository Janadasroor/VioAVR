#include "vioavr/core/uart.hpp"
#include "vioavr/core/memory_bus.hpp"
#include <algorithm>
#include <vector>
#include <string>

namespace vioavr::core {

namespace {
constexpr u8 kFeMask = 0x10U;
constexpr u8 kDorMask = 0x08U;
constexpr u8 kUpeMask = 0x04U;
}

Uart::Uart(std::string_view name, const UartDescriptor& descriptor, PinMux& pin_mux) noexcept
    : name_(std::string(name)), desc_(descriptor), pin_mux_(&pin_mux)
{
    reset();
    const std::array<u16, 6> addrs = {
        desc_.udr_address, desc_.ucsra_address, 
        desc_.ucsrb_address, desc_.ucsrc_address,
        desc_.ubrrl_address, desc_.ubrrh_address
    };
    std::vector<u16> sorted_addrs;
    for (auto a : addrs) if (a != 0) sorted_addrs.push_back(a);
    std::sort(sorted_addrs.begin(), sorted_addrs.end());
    
    size_t ri = 0;
    ranges_.fill({0, 0});
    for (size_t i = 0; i < sorted_addrs.size(); ++i) {
        if (ri > 0 && sorted_addrs[i] == ranges_[ri-1].end + 1) {
             ranges_[ri-1].end = sorted_addrs[i];
        } else {
             ranges_[ri++] = AddressRange{sorted_addrs[i], sorted_addrs[i]};
        }
    }
}

std::string_view Uart::name() const noexcept
{
    return name_;
}

std::span<const AddressRange> Uart::mapped_ranges() const noexcept
{
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

ClockDomain Uart::clock_domain() const noexcept
{
    return ClockDomain::io;
}

void Uart::reset() noexcept
{
    udr_rx_ = 0U;
    udr_tx_ = 0U;
    ucsra_ = desc_.udre_mask; // UDRE initially set
    ucsrb_ = 0U;
    ucsrc_ = desc_.ucsrc_reset;
    ubrrh_ = 0U;
    ubrrl_ = 0U;
    tx_active_ = false;
    rx_active_ = false;
    rx_pin_state_ = RxPinState::idle;
    rx_bit_index_ = 0;
    rx_parity_bits_ = 0;
    rx_stop_bits_ = 1;
    rx_sample_in_ = 0;
    rx_pin_last_ = true;
    tx_cycle_accumulator_ = 0;
    rx_cycle_accumulator_ = 0;
    tx_bit_duration_ = 160U;
    tx_output_queue_.clear();
    tx_buffer_full_ = false;
    rx_shift_reg_ = 0;
    rx_bits_left_ = 0;
    rx_frame_error_ = false;
    rx_parity_error_ = false;
    if (desc_.txd_pin_address)
        pin_mux_->release_pin_by_address(desc_.txd_pin_address, desc_.txd_pin_bit, PinOwner::uart);
    update_interrupt_state();
}

void Uart::tick(const u64 elapsed_cycles) noexcept
{
    if (power_reduction_enabled()) return;

    const u8 old_ucsra = ucsra_;

    // Start transmission if buffer is full and shift register is idle
    if (!tx_active_ && tx_buffer_full_ && (ucsrb_ & desc_.txen_mask)) {
        tx_shift_reg_ = ((ucsrb_ & 0x01U) << 8) | udr_tx_;
        tx_active_ = true;
        tx_buffer_full_ = false;
        tx_cycle_accumulator_ = 0;
        ucsra_ |= desc_.udre_mask;   // Buffer now empty
        ucsra_ &= ~desc_.txc_mask;  // New transmission clears TXC
        // Drive start bit LOW
        if (desc_.txd_pin_address)
            pin_mux_->update_pin_by_address(desc_.txd_pin_address, desc_.txd_pin_bit,
                PinOwner::uart, true, false);
    }

    if (tx_active_) {
        u64 old_accum = tx_cycle_accumulator_;
        tx_cycle_accumulator_ += elapsed_cycles;
        u16 ubrr = (static_cast<u16>(ubrrh_) << 8) | ubrrl_;
        u32 bit_duration = ((ucsra_ & desc_.u2x_mask) ? 8U : 16U) * (ubrr + 1U);
        u8 data_bits = (ucsrb_ & 0x04U) ? 9 : ((ucsrc_ >> 1) & 0x03U) + 5;
        u8 parity_bits = ((ucsrc_ & 0x30U) != 0) ? 1 : 0;
        u8 stop_bits = (ucsrc_ & 0x08U) ? 2 : 1;
        const u64 limit = bit_duration * (1ULL + data_bits + parity_bits + stop_bits);
        
        // Drive TX pin for current bit position
        if (desc_.txd_pin_address && tx_cycle_accumulator_ < limit) {
            u64 bit_pos = tx_cycle_accumulator_ / bit_duration;
            if (bit_pos < 1ULL + data_bits + parity_bits + stop_bits) {
                bool level = true; // stop/idle HIGH
                if (bit_pos == 0) {
                    level = false; // start bit
                } else if (bit_pos <= data_bits) {
                    level = (tx_shift_reg_ >> (bit_pos - 1)) & 1;
                }
                pin_mux_->update_pin_by_address(desc_.txd_pin_address, desc_.txd_pin_bit,
                    PinOwner::uart, true, level);
            }
        }
        
        if (tx_cycle_accumulator_ >= limit) {
            // Byte finished shifting — drive idle HIGH
            if (desc_.txd_pin_address)
                pin_mux_->update_pin_by_address(desc_.txd_pin_address, desc_.txd_pin_bit,
                    PinOwner::uart, true, true);
            
            tx_output_queue_.push_back(static_cast<u16>(tx_shift_reg_));
            ucsra_ |= desc_.txc_mask;
            
            // Check for next byte in buffer
            if (tx_buffer_full_) {
                tx_shift_reg_ = ((ucsrb_ & 0x01U) << 8) | udr_tx_;
                tx_buffer_full_ = false;
                ucsra_ |= desc_.udre_mask;
                ucsra_ &= ~desc_.txc_mask;
                tx_cycle_accumulator_ -= limit;
                // Drive start bit of next byte
                if (desc_.txd_pin_address)
                    pin_mux_->update_pin_by_address(desc_.txd_pin_address, desc_.txd_pin_bit,
                        PinOwner::uart, true, false);
                // tx_active remains true
            } else {
                tx_active_ = false;
            }
        }
    }

    // Abort RX if receiver disabled mid-reception
    if ((ucsrb_ & desc_.rxen_mask) == 0U) { rx_active_ = false; rx_pin_state_ = RxPinState::idle; }

    // RX pin sampling: start bit, data bits (LSB first), optional parity,
    // then one or two stop bits (datasheet USART "Receiver" sections).
    if ((ucsrb_ & desc_.rxen_mask) != 0U && desc_.rxd_pin_address != 0U) {
        const PinState rx_pin =
            pin_mux_->get_state_by_address(desc_.rxd_pin_address, desc_.rxd_pin_bit);
        // An undriven input is idle-high: only a pin somebody actually drives
        // carries a serial line (otherwise a floating pin reads as a start
        // bit and the receiver would decode noise).
        const bool line_driven = rx_pin.is_output;
        const bool level = line_driven ? rx_pin.drive_level : true;
        u16 ubrr = static_cast<u16>((ubrrh_ << 8) | ubrrl_);
        const u32 bit_cycles =
            static_cast<u32>((ucsra_ & desc_.u2x_mask) ? 8U : 16U) * (ubrr + 1U);
        const u8 data_bits = (ucsrb_ & 0x04U) ? 9U : static_cast<u8>(((ucsrc_ >> 1) & 0x03U) + 5U);
        const u8 parity_bits = (ucsrc_ & 0x30U) != 0U ? 1U : 0U;
        const u8 stop_bits = (ucsrc_ & 0x08U) ? 2U : 1U;

        switch (rx_pin_state_) {
        case RxPinState::idle:
            // A high-to-low transition is the start bit.
            if (!level && rx_pin_last_) {
                rx_pin_state_ = RxPinState::start;
                rx_shift_reg_ = 0;
                rx_bit_index_ = 0;
                rx_parity_bits_ = parity_bits;
                rx_stop_bits_ = stop_bits;
                // Sample the first data bit at 1.5 bit times.
                rx_sample_in_ = bit_cycles + (bit_cycles / 2U);
            }
            break;

        case RxPinState::start:
        case RxPinState::data:
        case RxPinState::parity:
        case RxPinState::stop:
            rx_sample_in_ -= (rx_sample_in_ > elapsed_cycles) ? elapsed_cycles : rx_sample_in_;
            while (rx_pin_state_ != RxPinState::idle && rx_sample_in_ == 0U) {
                if (rx_pin_state_ == RxPinState::start ||
                    rx_pin_state_ == RxPinState::data) {
                    if (level && rx_bit_index_ < 16U) {
                        rx_shift_reg_ |= static_cast<u16>(1U << rx_bit_index_);
                    }
                    ++rx_bit_index_;
                    if (rx_bit_index_ >= data_bits) {
                        if (rx_parity_bits_ != 0U) {
                            rx_pin_state_ = RxPinState::parity;
                        } else {
                            rx_pin_state_ = RxPinState::stop;
                        }
                    } else {
                        rx_pin_state_ = RxPinState::data;
                    }
                    rx_sample_in_ = bit_cycles;
                } else if (rx_pin_state_ == RxPinState::parity) {
                    u8 ones = 0;
                    for (u8 i = 0; i < data_bits && i < 16U; ++i) {
                        ones = static_cast<u8>(ones + ((rx_shift_reg_ >> i) & 1U));
                    }
                    const bool parity_bit = level;
                    const bool even = (ucsrc_ & 0x30U) == 0x20U; // UPM = 10
                    const u8 total = static_cast<u8>(ones + (parity_bit ? 1U : 0U));
                    // Even parity: the count must be even; odd: odd.
                    const bool wrong = even ? ((total % 2U) != 0U) : ((total % 2U) == 0U);
                    if (wrong) ucsra_ |= kUpeMask;
                    rx_pin_state_ = RxPinState::stop;
                    rx_sample_in_ = bit_cycles;
                } else { // stop bit(s)
                    if (!level) ucsra_ |= kFeMask;
                    if (rx_stop_bits_ > 1U) {
                        rx_stop_bits_--;
                        rx_sample_in_ = bit_cycles;
                    } else {
                        // A second byte while RXC is still set loses data.
                        if ((ucsra_ & desc_.rxc_mask) != 0U) {
                            ucsra_ |= kDorMask;
                        } else {
                            udr_rx_ = rx_shift_reg_;
                            ucsra_ |= desc_.rxc_mask;
                        }
                        rx_pin_state_ = RxPinState::idle;
                        rx_sample_in_ = 0;
                    }
                }
            }
            break;
        }
        rx_pin_last_ = level;
    }

    // RX timing
    if (rx_active_) {
        rx_cycle_accumulator_ += elapsed_cycles;
        u16 ubrr = (static_cast<u16>(ubrrh_) << 8) | ubrrl_;
        u32 bit_duration = ((ucsra_ & desc_.u2x_mask) ? 8U : 16U) * (ubrr + 1U);
        u8 data_bits = (ucsrb_ & 0x04U) ? 9 : ((ucsrc_ >> 1) & 0x03U) + 5;
        u8 parity_bits = ((ucsrc_ & 0x30U) != 0) ? 1 : 0;
        u8 stop_bits = (ucsrc_ & 0x08U) ? 2 : 1;
        const u64 limit = bit_duration * (1ULL + data_bits + parity_bits + stop_bits);
        if (rx_cycle_accumulator_ >= limit) {
            rx_active_ = false;
            ucsra_ |= desc_.rxc_mask;
            // Error flags latch together with RXC. UPE applies only when the
            // mode enables parity (UPM1:0 != 00); without a parity bit there
            // is nothing to check.
            if (rx_frame_error_) ucsra_ |= kFeMask;
            if (rx_parity_error_ && (ucsrc_ & 0x30U) != 0U) ucsra_ |= kUpeMask;
            rx_frame_error_ = false;
            rx_parity_error_ = false;
        }
    }

    if (ucsra_ != old_ucsra) {
        update_interrupt_state();
    }
}

u8 Uart::read(const u16 address) noexcept
{
    if (address == desc_.udr_address) {
        ucsra_ &= ~(desc_.rxc_mask | kDorMask | kFeMask | kUpeMask);
        update_interrupt_state();
        if (partner_ != nullptr) partner_->on_receive_buffer_read();
        return static_cast<u8>(udr_rx_ & 0xFFU);
    }
    if (address == desc_.ucsra_address) return ucsra_;
    if (address == desc_.ucsrb_address) return ucsrb_;
    if (address == desc_.ucsrc_address) return ucsrc_;
    if (address == desc_.ubrrl_address) return ubrrl_;
    if (address == desc_.ubrrh_address) return ubrrh_;
    return 0U;
}

void Uart::write(const u16 address, const u8 value) noexcept
{
    const u8 old_ucsra = ucsra_;
    if (address == desc_.udr_address) {
        if (partner_ != nullptr && partner_->on_transmit_data(value)) return;
        udr_tx_ = value;
        tx_buffer_full_ = true;
        ucsra_ &= ~desc_.udre_mask; // Buffer full
    } else if (address == desc_.ucsra_address) {
        ucsra_ = static_cast<u8>((ucsra_ & ~(desc_.u2x_mask | desc_.mpcm_mask)) | (value & (desc_.u2x_mask | desc_.mpcm_mask)));
        if (value & desc_.txc_mask) ucsra_ &= ~desc_.txc_mask;
    } else if (address == desc_.ucsrb_address) {
        u8 old = ucsrb_;
        ucsrb_ = value;
        if (desc_.txd_pin_address) {
            if ((value & desc_.txen_mask) && !(old & desc_.txen_mask)) {
                pin_mux_->claim_pin_by_address(desc_.txd_pin_address, desc_.txd_pin_bit, PinOwner::uart);
                pin_mux_->update_pin_by_address(desc_.txd_pin_address, desc_.txd_pin_bit,
                    PinOwner::uart, true, true);
            } else if (!(value & desc_.txen_mask) && (old & desc_.txen_mask)) {
                pin_mux_->release_pin_by_address(desc_.txd_pin_address, desc_.txd_pin_bit, PinOwner::uart);
            }
        }
    } else if (address == desc_.ucsrc_address) {
        ucsrc_ = value;
    } else if (address == desc_.ubrrl_address) {
        ubrrl_ = value;
    } else if (address == desc_.ubrrh_address) {
        ubrrh_ = value & 0x0FU;
    }
    update_interrupt_state();
}

bool Uart::pending_interrupt_request(InterruptRequest& request) const noexcept
{
    const bool rxc = (ucsra_ & desc_.rxc_mask) && (ucsrb_ & desc_.rxcie_mask);
    const bool udre = (ucsra_ & desc_.udre_mask) && (ucsrb_ & desc_.udrie_mask);
    const bool txc = (ucsra_ & desc_.txc_mask) && (ucsrb_ & desc_.txcie_mask);

    if (rxc || udre || txc) {
        if (rxc) {
            request = {desc_.rx_vector_index, 0U};
            return true;
        }
        if (udre) {
            request = {desc_.udre_vector_index, 0U};
            return true;
        }
        if (txc) {
            request = {desc_.tx_vector_index, 0U};
            return true;
        }
    }
    return false;
}

bool Uart::consume_interrupt_request(InterruptRequest& request) noexcept
{
    // For legacy UART, flags are handled by hardware as follows:
    // RXC: cleared by reading UDR
    // UDRE: cleared by writing UDR
    // TXC: cleared by jump to vector OR by writing 1 to the bit

    if (request.vector_index == desc_.tx_vector_index) {
        ucsra_ &= ~desc_.txc_mask;
        update_interrupt_state();
    }
    return true;
}

bool Uart::inject_received_byte(const u16 data, const bool frame_error,
                                const bool parity_error) noexcept
{
    if ((ucsrb_ & desc_.rxen_mask) == 0U) return false;

    // MPCM: in 9-bit mode, data frames (9th bit = 0) are ignored.
    const bool use_9bit = (ucsrb_ & 0x04U) != 0U;
    const bool bit9 = ((data >> 8U) & 0x01U) != 0U;
    if ((ucsra_ & desc_.mpcm_mask) != 0U && use_9bit && !bit9) return false;

    if ((ucsra_ & desc_.rxc_mask) != 0U || rx_active_) {
        ucsra_ |= kDorMask;
        return false;
    }

    rx_shift_reg_ = data;
    udr_rx_ = data & 0xFFU;
    ucsrb_ = static_cast<u8>((ucsrb_ & ~0x02U) | ((data >> 7) & 0x02U));
    rx_frame_error_ = frame_error;
    rx_parity_error_ = parity_error;
    rx_cycle_accumulator_ = 0;
    rx_active_ = true;
    update_interrupt_state();
    return true;
}

bool Uart::consume_transmitted_byte(u16& data) noexcept
{
    if (tx_output_queue_.empty()) return false;
    data = tx_output_queue_.front();
    tx_output_queue_.pop_front();
    return true;
}

bool Uart::power_reduction_enabled() const noexcept
{
    if (!bus_ || desc_.pr_address == 0U || desc_.pr_bit == 0xFFU) {
        return false;
    }
    return (bus_->read_data(desc_.pr_address) & (1U << desc_.pr_bit)) != 0;
}

void Uart::update_interrupt_state() noexcept
{
    const bool rxc = (ucsra_ & desc_.rxc_mask) && (ucsrb_ & desc_.rxcie_mask);
    const bool udre = (ucsra_ & desc_.udre_mask) && (ucsrb_ & desc_.udrie_mask);
    const bool txc = (ucsra_ & desc_.txc_mask) && (ucsrb_ & desc_.txcie_mask);
    set_interrupt_pending(rxc || udre || txc);
}

} // namespace vioavr::core
