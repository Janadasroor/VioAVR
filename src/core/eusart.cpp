#include "vioavr/core/eusart.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/uart.hpp"
#include <algorithm>

namespace vioavr::core {

namespace {
// Datasheet 18.6.4: the EUSART and the standard USART are one peripheral.
// RXC/TXC/UDRE/FE/DOR/UPE are the classic UCSRA bits (0xC0) regardless of
// mode; EUCSRA only carries the character size.
constexpr u8 kUcsraRxc = 0x80U;
constexpr u8 kUcsraTxc = 0x40U;
} // namespace

Eusart::Eusart(std::string_view name, const EusartDescriptor& desc) noexcept
    : name_(name), desc_(desc)
{
    std::vector<u16> addrs = {
        desc.eudr_address, static_cast<u16>(desc.eudr_address + 1),
        desc.eucsra_address, desc.eucsrb_address, desc.eucsrc_address,
        desc.mubrrl_address, desc.mubrrh_address
    };
    std::sort(addrs.begin(), addrs.end());
    
    size_t count = 0;
    for (u16 addr : addrs) {
        if (addr == 0) continue;
        if (count > 0 && addr == ranges_[count-1].end + 1) {
            ranges_[count-1].end = addr;
        } else if (count < ranges_.size()) {
            ranges_[count++] = {addr, addr};
        }
    }
}

std::string_view Eusart::name() const noexcept { return name_; }

std::span<const AddressRange> Eusart::mapped_ranges() const noexcept {
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

ClockDomain Eusart::clock_domain() const noexcept { return ClockDomain::io; }

void Eusart::reset() noexcept {
    eudr_ = 0;
    eucsra_ = desc_.eucsra_reset;
    eucsrb_ = desc_.eucsrb_reset;
    eucsrc_ = desc_.eucsrc_reset;
    mubrrl_ = 0;
    mubrrh_ = 0;
    tx_queue_.clear();
    rx_queue_.clear();
    rx_uart_state_ = RxUartState::idle;
    rx_active_ = false;
    rx_accumulator_ = 0;
    frame_error_count_ = 0;
    man_state_ = ManState::data;
    rx_size_16_or_17_ = false;
    rx_stop_bits_ = 1;
    eus_stop1_ = true;
    eus_stop2_ = true;
}

void Eusart::tick(u64 elapsed_cycles) noexcept {
    if (power_reduction_enabled()) return;
    
    bool enabled = (eucsrb_ & desc_.eus_en_mask);
    if (!enabled) {
        if (desc_.txd_pin_address) bus_->pin_mux()->release_pin_by_address(desc_.txd_pin_address, desc_.txd_pin_bit, PinOwner::uart);
        if (desc_.rxd_pin_address) bus_->pin_mux()->release_pin_by_address(desc_.rxd_pin_address, desc_.rxd_pin_bit, PinOwner::uart);
        tx_active_ = false;
        return;
    }

    u16 mubrr = (static_cast<u16>(mubrrh_) << 8) | mubrrl_;
    u64 cycles_per_bit = 16ULL * (mubrr + 1);
    bool manchester = (eucsrb_ & desc_.emch_mask);
    u64 cycles_per_half = cycles_per_bit / 2;

    auto update_pin = [&]() {
        if (desc_.txd_pin_address && tx_active_) {
            bus_->pin_mux()->claim_pin_by_address(desc_.txd_pin_address, desc_.txd_pin_bit, PinOwner::uart);
            
            bool current_bit_val;
            if (eucsrb_ & desc_.bodr_mask) { // BODR: MSB first
                current_bit_val = (tx_shift_reg_ >> (tx_bit_count_ - 1)) & 1;
            } else { // LSB first
                current_bit_val = tx_shift_reg_ & 1;
            }

            bool output_level;
            if (manchester) {
                if (current_bit_val) {
                    output_level = tx_half_bit_; // First half LOW, second half HIGH
                } else {
                    output_level = !tx_half_bit_; // First half HIGH, second half LOW
                }
            } else {
                output_level = current_bit_val;
            }
            
            bus_->pin_mux()->update_pin_by_address(desc_.txd_pin_address, desc_.txd_pin_bit, PinOwner::uart, true, output_level);
        } else if (desc_.txd_pin_address && !tx_active_) {
             // Idle level for UART/Manchester is usually HIGH
             bus_->pin_mux()->update_pin_by_address(desc_.txd_pin_address, desc_.txd_pin_bit, PinOwner::uart, true, true);
        }
    };

    auto get_char_size = [&](bool tx) -> u8 {
        u8 val = tx ? ((eucsra_ & desc_.utxs_mask) >> 4) : (eucsra_ & desc_.urxs_mask);
        switch (val) {
            case 0x00: return 5;
            case 0x01: return 6;
            case 0x02: return 7;
            case 0x03: return 8;
            case 0x07: return 9;
            case 0x08: return 13;
            case 0x09: return 14;
            case 0x0A: return 15;
            case 0x0B: return 16;
            // URxS/UTxS = 1110 selects 16 bits (16 or 17 in Manchester only,
            // where F1617 reports the actual length); = 1111 selects 17 bits.
            case 0x0E: return 16;
            case 0x0F: return 17;
            default: return 8;
        }
    };

    // --- TX Logic ---
    if (!tx_active_ && !tx_queue_.empty()) {
        tx_active_ = true;
        u32 data = tx_queue_.front();
        tx_queue_.pop_front();
        
        u8 data_bits = get_char_size(true);
        // Build frame: Start(1) + Data + Stop(s)
        // For Manchester, we simulate this by setting up the shift reg
        // and a bit counter that includes start/stop.
        tx_shift_reg_ = data;
        if (manchester) {
            // Manchester usually just sends the data bits if no explicit start/stop bit is defined,
            // but the AT90PWM316 says "Start Bit is a logic 1".
            // We'll prepend it.
            tx_shift_reg_ = (data << 1) | 0x01; 
            tx_bit_count_ = data_bits + 1; // +1 for Start
        } else {
            tx_bit_count_ = data_bits;
        }
        
        tx_half_bit_ = false;
        cycles_to_next_bit_ = manchester ? cycles_per_half : cycles_per_bit;
        update_pin();
    }

    if (tx_active_) {
        update_pin();

        tx_accumulator_ += elapsed_cycles;
        while (tx_active_ && tx_accumulator_ >= cycles_to_next_bit_) {
            tx_accumulator_ -= cycles_to_next_bit_;
            
            if (manchester && !tx_half_bit_) {
                tx_half_bit_ = true;
                cycles_to_next_bit_ = cycles_per_half;
            } else {
                tx_half_bit_ = false;
                if (!(eucsrb_ & desc_.bodr_mask)) tx_shift_reg_ >>= 1; // LSB first
                else tx_shift_reg_ <<= 1; // MSB first
                
                tx_bit_count_--;
                
                if (tx_bit_count_ == 0) {
                    tx_active_ = false;
                    // TX Complete lives in the shared UCSRA (18.6.4).
                    if (shared_usart_ != nullptr) {
                        shared_usart_->set_status_bits(kUcsraTxc, true);
                    } else {
                        eucsra_ |= desc_.txc_mask;
                    }
                    cycles_to_next_bit_ = 0;
                } else {
                    cycles_to_next_bit_ = manchester ? cycles_per_half : cycles_per_bit;
                }
            }
            update_pin();
        }
    }

    // --- RX Logic ---
    if (eucsrb_ & desc_.eus_en_mask) {
        bool current_rx = bus_->pin_mux()->get_state_by_address(desc_.rxd_pin_address, desc_.rxd_pin_bit).drive_level;
        
        if (manchester) {
            // Manchester RX path
            if (!rx_active_) {
                // Manchester '1' Start Bit: LOW then HIGH. Wait for falling edge.
                if (current_rx == false && rx_last_pin_level_ == true) {
                    rx_active_ = true;
                    rx_waiting_for_start_ = true;
                    rx_half_bit_ = false;
                    rx_cycles_to_sample_ = cycles_per_half / 2;
                    rx_shift_reg_ = 0;
                    rx_accumulator_ = 0; // start from a clean bit boundary
                }
            } else {
                rx_accumulator_ += elapsed_cycles;
                while (rx_active_ && rx_accumulator_ >= rx_cycles_to_sample_) {
                    rx_accumulator_ -= rx_cycles_to_sample_;
                    
                    if (rx_waiting_for_start_) {
                        if (!rx_half_bit_) {
                            if (current_rx == true) {
                                rx_active_ = false;
                            } else {
                                rx_half_bit_ = true;
                                rx_cycles_to_sample_ = cycles_per_half;
                            }
                        } else {
                            if (current_rx == false) {
                                rx_active_ = false;
                            } else {
                                rx_waiting_for_start_ = false;
                                rx_half_bit_ = false;
                                rx_bit_count_ = get_char_size(false);
                                // URxS = 1110 enables the 16-or-17 bit mode.
                                rx_size_16_or_17_ =
                                    ((eucsra_ & desc_.urxs_mask) == 0x0EU);
                                rx_stop_bits_ = (eucsrb_ & desc_.eusbs_mask) ? 2U : 1U;
                                man_state_ = ManState::data;
                                rx_cycles_to_sample_ = cycles_per_half;
                            }
                        }
                    } else if (man_state_ == ManState::data) {
                        if (!rx_half_bit_) {
                            rx_temp_sampled_ = current_rx;
                            rx_half_bit_ = true;
                            rx_cycles_to_sample_ = cycles_per_half;
                        } else {
                            const bool bit_val = decode_symbol(current_rx);
                            u8 total_bits = get_char_size(false);
                            if (!(eucsrb_ & desc_.bodr_mask)) {
                                if (bit_val) rx_shift_reg_ |= (1U << (total_bits - rx_bit_count_));
                            } else {
                                rx_shift_reg_ = (rx_shift_reg_ << 1) | (bit_val ? 1 : 0);
                            }

                            rx_bit_count_--;
                            rx_half_bit_ = false;

                            if (rx_bit_count_ == 0) {
                                // URxS = 1110 selects the 16-or-17 bit special
                                // mode: the next bit time decides (18.3.3).
                                if (rx_size_16_or_17_) {
                                    man_state_ = ManState::bit17;
                                } else {
                                    eucsrc_ = static_cast<u8>(eucsrc_ & ~desc_.f1617_mask);
                                    man_state_ = ManState::stop1;
                                }
                            }
                            rx_cycles_to_sample_ = cycles_per_half;
                        }
                    } else if (man_state_ == ManState::bit17) {
                        // Seventeenth bit time: a Manchester transition means a
                        // 17th data bit (F1617 = 1), otherwise it is already
                        // the first stop bit (F1617 = 0).
                        if (!rx_half_bit_) {
                            rx_temp_sampled_ = current_rx;
                            rx_half_bit_ = true;
                            rx_cycles_to_sample_ = cycles_per_half;
                        } else {
                            const bool transition = (rx_temp_sampled_ != current_rx);
                            rx_half_bit_ = false;
                            if (transition) {
                                if (current_rx) rx_shift_reg_ |= (1U << 16U);
                                eucsrc_ |= desc_.f1617_mask;
                                man_state_ = ManState::stop1;
                            } else {
                                eucsrc_ = static_cast<u8>(eucsrc_ & ~desc_.f1617_mask);
                                eus_stop1_ = current_rx;
                                man_state_ = (rx_stop_bits_ > 1U) ? ManState::stop2 : ManState::done;
                            }
                            rx_cycles_to_sample_ = cycles_per_half;
                        }
                    } else if (man_state_ == ManState::stop1 || man_state_ == ManState::stop2) {
                        if (!rx_half_bit_) {
                            rx_temp_sampled_ = current_rx;
                            rx_half_bit_ = true;
                            rx_cycles_to_sample_ = cycles_per_half;
                        } else {
                            rx_half_bit_ = false;
                            // A stop bit carries no Manchester transition; its
                            // value is the level of the bit time.
                            const bool stop_value = current_rx;
                            if (man_state_ == ManState::stop1) {
                                eus_stop1_ = stop_value;
                                if (rx_stop_bits_ > 1U) {
                                    man_state_ = ManState::stop2;
                                    rx_cycles_to_sample_ = cycles_per_half;
                                } else {
                                    man_state_ = ManState::done;
                                    rx_cycles_to_sample_ = cycles_per_half;
                                }
                            } else {
                                eus_stop2_ = stop_value;
                                man_state_ = ManState::done;
                                rx_cycles_to_sample_ = cycles_per_half;
                            }
                        }
                    } else { // ManState::done
                        // EUCSRC.STP[1:0] reports the stop bits of the frame
                        // (datasheet 18.6.5); Manchester frames only.
                        u8 stp = static_cast<u8>(eus_stop2_ ? 0x02U : 0x00U);
                        if (eus_stop1_) stp |= 0x01U;
                        eucsrc_ = static_cast<u8>((eucsrc_ & ~desc_.stp_mask) | stp);
                        store_received_word(rx_shift_reg_);
                        rx_active_ = false;
                        rx_cycles_to_sample_ = 0;
                    }
                }
            }
        } else {
            // Standard UART RX path
            switch (rx_uart_state_) {
                case RxUartState::idle:
                    // Wait for falling edge (start bit)
                    if (current_rx == false && rx_last_pin_level_ == true) {
                        rx_uart_state_ = RxUartState::start_bit;
                        rx_cycles_to_sample_ = cycles_per_bit + cycles_per_bit / 2; // 1.5 bit times to center of first data bit
                        rx_accumulator_ = 0;
                        rx_shift_reg_ = 0;
                        rx_bit_count_ = get_char_size(false);
                    }
                    break;

                case RxUartState::start_bit:
                    rx_accumulator_ += elapsed_cycles;
                    if (rx_accumulator_ >= rx_cycles_to_sample_) {
                        rx_accumulator_ = 0;
                        // Sample first data bit
                        bool bit_val = current_rx;
                        u8 total_bits = rx_bit_count_;
                        if (!(eucsrb_ & desc_.bodr_mask)) { // LSB first
                            if (bit_val) rx_shift_reg_ |= (1U << (total_bits - 1));
                        } else { // MSB first
                            rx_shift_reg_ = (rx_shift_reg_ << 1) | (bit_val ? 1 : 0);
                        }
                        rx_bit_count_--;
                        if (rx_bit_count_ > 0) {
                            rx_uart_state_ = RxUartState::data_bits;
                            rx_cycles_to_sample_ = cycles_per_bit;
                        } else {
                            // Only 1 data bit, skip to stop bit
                            rx_uart_state_ = RxUartState::stop_bit;
                            rx_cycles_to_sample_ = cycles_per_bit;
                        }
                    }
                    break;

                case RxUartState::data_bits:
                    rx_accumulator_ += elapsed_cycles;
                    while (rx_bit_count_ > 0 && rx_accumulator_ >= rx_cycles_to_sample_) {
                        rx_accumulator_ -= rx_cycles_to_sample_;
                        bool bit_val = current_rx;
                        u8 total_bits = get_char_size(false);
                        if (!(eucsrb_ & desc_.bodr_mask)) { // LSB first
                            if (bit_val) rx_shift_reg_ |= (1U << (rx_bit_count_ - 1));
                        } else { // MSB first
                            rx_shift_reg_ = (rx_shift_reg_ << 1) | (bit_val ? 1 : 0);
                        }
                        rx_bit_count_--;
                    }
                    if (rx_bit_count_ == 0) {
                        rx_uart_state_ = RxUartState::stop_bit;
                        rx_cycles_to_sample_ = cycles_per_bit;
                    }
                    break;

                case RxUartState::stop_bit:
                    rx_accumulator_ += elapsed_cycles;
                    if (rx_accumulator_ >= rx_cycles_to_sample_) {
                        rx_accumulator_ = 0;
                        bool stop_valid = current_rx;
                        if (!stop_valid) {
                            // The EUSART registers of this family carry no
                            // framing-error bit (EUCSRA = UTxS/URxS char size,
                            // EUCSRB = ctrl, EUCSRC = STP/F1617/FEM), so the
                            // error is counted instead of being written into a
                            // configuration bit.
                            ++frame_error_count_;
                        }
                        rx_uart_state_ = RxUartState::idle;
                        store_received_word(rx_shift_reg_);
                    }
                    break;
            }
        }
        rx_last_pin_level_ = current_rx;
    }
}

void Eusart::store_received_word(u32 word) noexcept {
    if (shared_usart_ != nullptr) {
        // UDR holds the low byte, EUDR the high byte; RXC goes to UCSRA.
        shared_usart_->set_rx_word(static_cast<u16>(word));
        rx_temp_ = static_cast<u8>((word >> 8U) & 0xFFU);
    } else {
        if (rx_queue_.size() >= 64U) return;
        rx_queue_.push_back(word);
        eucsra_ |= desc_.rxc_mask;
    }
}

u8 Eusart::read(u16 address) noexcept {
    if (address == desc_.eudr_address) {
        // EUDR is the HIGH byte of the received character (datasheet
        // 18.6.2); reading it does not consume the buffer.
        if (shared_usart_ != nullptr) {
            return static_cast<u8>((shared_usart_->rx_word() >> 8U) & 0xFFU);
        }
        if (rx_queue_.empty()) return 0;
        u32 val = rx_queue_.front();
        rx_queue_.pop_front();
        eucsra_ &= ~desc_.rxc_mask;
        if (!rx_queue_.empty()) eucsra_ |= desc_.rxc_mask;
        // FEM and F1617 stay valid only until the receive buffer is read
        // (datasheet 18.6.5).
        eucsrc_ &= static_cast<u8>(~(desc_.fem_mask | desc_.f1617_mask));
        
        rx_temp_ = (val >> 8);
        return static_cast<u8>(val & 0xFF);
    }
    if (shared_usart_ == nullptr && address == desc_.eudr_address + 1) {
        // Legacy path for devices whose data register really is 16 bits
        // wide; the AT90PWM216/316 EUSART has no register at EUDR+1.
        return static_cast<u8>(rx_temp_);
    }
    if (address == desc_.eucsra_address) return eucsra_;
    if (address == desc_.eucsrb_address) return eucsrb_;
    if (address == desc_.eucsrc_address) return eucsrc_;
    if (address == desc_.mubrrl_address) return mubrrl_;
    if (address == desc_.mubrrh_address) return mubrrh_;
    return 0;
}

void Eusart::on_receive_buffer_read() noexcept {
    // EUCSRC status bits are valid only until the shared data register is
    // read (datasheet 18.6.5).
    eucsrc_ &= static_cast<u8>(~(desc_.fem_mask | desc_.f1617_mask));
}

bool Eusart::on_transmit_data(u8 low_byte) noexcept {
    if (!is_extended_mode()) return false;
    const u32 full = (static_cast<u32>(tx_temp_) << 8U) | low_byte;
    tx_queue_.push_back(full);
    tx_temp_ = 0U;
    return true;
}

void Eusart::write(u16 address, u8 value) noexcept {
    if (address == desc_.eudr_address) { // High byte of the TX character
        if (shared_usart_ != nullptr) {
            // The low byte goes to the shared UDR and triggers transmission
            // (datasheet 18.6.2).
            tx_temp_ = value;
        } else {
            tx_temp_ = value;
        }
    } else if (shared_usart_ == nullptr && address == desc_.eudr_address + 1) {
        tx_temp_ = value;
    } else if (address == desc_.eucsra_address) {
        // TXC is write-1-to-clear; other bits are writable
        eucsra_ = (value & ~desc_.txc_mask) | (eucsra_ & desc_.txc_mask & ~value);
    } else if (address == desc_.eucsrb_address) {
        eucsrb_ = value;
        if ((eucsrb_ & desc_.eus_en_mask) && bus_ && bus_->pin_mux()) {
            bus_->pin_mux()->claim_pin_by_address(desc_.txd_pin_address, desc_.txd_pin_bit, PinOwner::uart);
            bus_->pin_mux()->claim_pin_by_address(desc_.rxd_pin_address, desc_.rxd_pin_bit, PinOwner::uart);
        } else if (bus_ && bus_->pin_mux()) {
            bus_->pin_mux()->release_pin_by_address(desc_.txd_pin_address, desc_.txd_pin_bit, PinOwner::uart);
            bus_->pin_mux()->release_pin_by_address(desc_.rxd_pin_address, desc_.rxd_pin_bit, PinOwner::uart);
        }
    } else if (address == desc_.eucsrc_address) {
        eucsrc_ = value;
    } else if (address == desc_.mubrrl_address) {
        mubrrl_ = value;
    } else if (address == desc_.mubrrh_address) {
        mubrrh_ = value;
    }
}

void Eusart::inject_received_byte(u32 data) noexcept {
    if (shared_usart_ != nullptr) {
        shared_usart_->set_rx_word(static_cast<u16>(data));
        rx_temp_ = static_cast<u8>((data >> 8U) & 0xFFU);
        return;
    }
    if (rx_queue_.size() >= 64) return;
    rx_queue_.push_back(data);
}

bool Eusart::consume_transmitted_byte(u32& data) noexcept {
    if (tx_queue_.empty()) return false;
    if (tx_active_) return false;
    data = tx_queue_.front();
    tx_queue_.pop_front();
    return true;
}

bool Eusart::power_reduction_enabled() const noexcept {
    if (!bus_ || desc_.pr_address == 0U || desc_.pr_bit == 0xFFU) return false;
    return (bus_->read_data(desc_.pr_address) & (1U << desc_.pr_bit)) != 0;
}

} // namespace vioavr::core
