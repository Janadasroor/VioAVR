#include "vioavr/core/uart8x.hpp"
#include <algorithm>
#include <vector>

namespace vioavr::core {

Uart8x::Uart8x(const Uart8xDescriptor& descriptor, PinMux& pin_mux) noexcept
    : desc_(descriptor), pin_mux_(&pin_mux)
{
    const std::array<u16, 9> addrs = {
        desc_.ctrla_address, desc_.ctrlb_address, desc_.ctrlc_address,
        desc_.ctrld_address, desc_.status_address, desc_.baud_address,
        desc_.rxdata_address, desc_.txdata_address, desc_.dbgctrl_address
    };
    
    std::vector<u16> sorted;
    for (auto a : addrs) {
        if (a != 0) {
            sorted.push_back(a);
            if (a == desc_.baud_address || a == desc_.rxdata_address || a == desc_.txdata_address) {
                sorted.push_back(a + 1);
            }
        }
    }
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());

    size_t ri = 0;
    ranges_.fill({0, 0});
    for (size_t i = 0; i < sorted.size(); ++i) {
        if (ri > 0 && sorted[i] == ranges_[ri-1].end + 1) {
            ranges_[ri-1].end = sorted[i];
        } else {
            ranges_[ri++] = AddressRange{sorted[i], sorted[i]};
        }
    }
}

std::span<const AddressRange> Uart8x::mapped_ranges() const noexcept {
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

void Uart8x::reset() noexcept {
    ctrla_ = 0x00U;
    ctrlb_ = 0x00U;
    ctrlc_ = 0x03U; // 8N1
    ctrld_ = 0x00U;
    status_ = STATUS_DREIF;
    baud_ = 0U;
    dbgctrl_ = 0x00U;
    txdatah_ = 0x00U;
    rx_fifo_count_ = 0;
    rx_fifo_read_idx_ = 0;
    rx_fifo_write_idx_ = 0;
    tx_in_progress_ = false;
    tx_data_busy_ = false;
    tx_bits_left_ = 0;
    tx_total_bits_ = 0;
    tx_bit_duration_ = 0.0;
    tx_cycle_accumulator_ = 0.0;
    rx_in_progress_ = false;
    rx_bits_left_ = 0;
    rx_total_bits_ = 0;
    rx_bit_duration_ = 0.0;
    rx_cycle_accumulator_ = 0.0;
}

void Uart8x::tick(u64 elapsed_cycles) noexcept {
    // Determine bit duration once if not cached
    u8 samples_per_bit = 16;
    u8 rxmode = (ctrlb_ & CTRLB_RXMODE_MASK) >> 1;
    if (rxmode == 0x01) samples_per_bit = 8;
    
    double bit_duration = 16.0;
    if (baud_ > 64) {
        bit_duration = (static_cast<double>(baud_) * samples_per_bit) / 64.0;
    }

    for (u64 i = 0; i < elapsed_cycles; ++i) {
        // 1. Transmitter
        if (!tx_in_progress_ && tx_data_busy_) {
            tx_in_progress_ = true;
            tx_data_busy_ = false;
            tx_cycle_accumulator_ = 0.0;
            tx_bit_duration_ = bit_duration;
            status_ |= STATUS_DREIF;
            
            // Construct frame
            u8 char_size = 5 + (ctrlc_ & 0x03U);
            if ((ctrlc_ & 0x07U) == 0x07U) char_size = 9; // 9-bit
            
            u16 data = (static_cast<u16>(txdatah_ & 0x01U) << 8U) | tx_data_buffer_;
            tx_shift_reg_ = data;
            
            u8 data_bits = char_size;
            u8 parity_bits = (ctrlc_ & CTRLC_PMODE_MASK) ? 1 : 0;
            u8 stop_bits = (ctrlc_ & CTRLC_SBMODE) ? 2 : 1;
            tx_total_bits_ = 1 + data_bits + parity_bits + stop_bits;
            tx_bits_left_ = tx_total_bits_;
        }

        if (tx_in_progress_) {
            tx_cycle_accumulator_ += 1.0;
            if (tx_cycle_accumulator_ >= tx_bit_duration_) {
                tx_cycle_accumulator_ -= tx_bit_duration_;
                tx_bits_left_--;
                
                if (tx_bits_left_ == 0) {
                    tx_in_progress_ = false;
                    status_ |= STATUS_TXCIF;
                    
                    // Loop-back handling
                    if (ctrla_ & CTRLA_LBME) {
                        // Push transmitted value back to receiver
                        actually_push_to_fifo(static_cast<u8>(tx_shift_reg_), (tx_shift_reg_ >> 8) & 0x01);
                    }
                }
            }
        }

        // 2. Receiver (bit-level simulation)
        if (ctrlb_ & CTRLB_RXEN) {
            u8 samples_per_bit = (ctrlb_ & CTRLB_RXMODE_MASK) == 0x02 ? 8 : 16;
            double bit_duration = (baud_ > 64) ? (static_cast<double>(baud_) * samples_per_bit) / 64.0 : 16.0;

            bool rx_level_bit = pin_mux_->get_state_by_address(desc_.rxd_pin_address, desc_.rxd_pin_bit).drive_level;
            PinLevel rx_level = rx_level_bit ? PinLevel::high : PinLevel::low;

            if (!rx_in_progress_ && rx_level == PinLevel::low) {
                rx_in_progress_ = true;
                rx_cycle_accumulator_ = bit_duration / 2.0;
                rx_bits_left_ = 10; // Start + 8 data + 1 stop
                rx_shift_reg_ = 0;
                status_ |= STATUS_RXSIF;
            }

            if (rx_in_progress_) {
                rx_cycle_accumulator_ += 1.0;
                if (rx_cycle_accumulator_ + 0.5 >= bit_duration) {
                    rx_cycle_accumulator_ -= bit_duration;
                    rx_bits_left_--;
                    if (rx_bits_left_ > 0 && rx_bits_left_ < 9) { // Bits 0-7
                        if (rx_level == PinLevel::high) {
                            rx_shift_reg_ |= (1 << (8 - rx_bits_left_));
                        }
                    } else if (rx_bits_left_ == 0) {
                        actually_push_to_fifo(static_cast<u8>(rx_shift_reg_), false);
                        rx_in_progress_ = false;
                        status_ &= ~STATUS_RXSIF;
                    }
                }
            }
        }
    }
}

u8 Uart8x::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.ctrlb_address) return ctrlb_;
    if (address == desc_.ctrlc_address) return ctrlc_;
    if (address == desc_.ctrld_address) return ctrld_;
    if (address == desc_.status_address) return status_;
    if (address == desc_.baud_address) return static_cast<u8>(baud_ & 0xFFU);
    if (address == desc_.baud_address + 1) return static_cast<u8>(baud_ >> 8U);
    if (address == desc_.rxdata_address) {
        if (rx_fifo_count_ == 0) return 0;
        u8 data = rx_fifo_[rx_fifo_read_idx_].data;
        rx_fifo_read_idx_ = (rx_fifo_read_idx_ + 1) % 2;
        rx_fifo_count_--;
        if (rx_fifo_count_ == 0) status_ &= ~STATUS_RXCIF;
        return data;
    }
    if (address == desc_.rxdata_address + 1) {
        if (rx_fifo_count_ == 0) return 0;
        u8 high = rx_fifo_[rx_fifo_read_idx_].high;
        if (status_ & STATUS_RXCIF) high |= RXDATAH_RXCIF;
        return high;
    }
    if (address == desc_.dbgctrl_address) return dbgctrl_;
    return 0U;
}

void Uart8x::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) ctrla_ = value;
    else if (address == desc_.ctrlb_address) ctrlb_ = value;
    else if (address == desc_.ctrlc_address) ctrlc_ = value;
    else if (address == desc_.ctrld_address) ctrld_ = value;
    else if (address == desc_.status_address) {
        if (value & STATUS_TXCIF) status_ &= ~STATUS_TXCIF;
    }
    else if (address == desc_.baud_address) baud_ = (baud_ & 0xFF00U) | value;
    else if (address == desc_.baud_address + 1) baud_ = (baud_ & 0x00FFU) | (static_cast<u16>(value) << 8U);
    else if (address == desc_.txdata_address + 1) {
        txdatah_ = value;
    }
    else if (address == desc_.txdata_address) {
        if (ctrlb_ & CTRLB_TXEN) {
            tx_data_buffer_ = value;
            tx_data_busy_ = true;
            status_ &= ~STATUS_DREIF;
            status_ &= ~STATUS_TXCIF;
        }
    }
    else if (address == desc_.dbgctrl_address) dbgctrl_ = value;
}

bool Uart8x::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if ((status_ & STATUS_RXCIF) && (ctrla_ & CTRLA_RXCIE)) {
        request = {desc_.rx_vector_index, 0U};
        return true;
    }
    if ((status_ & STATUS_DREIF) && (ctrla_ & CTRLA_DREIE)) {
        request = {desc_.dre_vector_index, 0U};
        return true;
    }
    if ((status_ & STATUS_TXCIF) && (ctrla_ & CTRLA_TXCIE)) {
        request = {desc_.tx_vector_index, 0U};
        return true;
    }
    return false;
}

bool Uart8x::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (!pending_interrupt_request(request)) return false;
    if (request.vector_index == desc_.tx_vector_index) status_ &= ~STATUS_TXCIF;
    return true;
}

void Uart8x::inject_received_byte(u8 data, bool bit9) noexcept {
    if (!(ctrlb_ & CTRLB_RXEN)) return;
    if (rx_in_progress_) return;

    rx_in_progress_ = true;
    rx_shift_reg_ = data | (static_cast<u16>(bit9) << 8);
    status_ |= STATUS_RXSIF;

    u8 samples_per_bit = (ctrlb_ & CTRLB_RXMODE_MASK) == 0x02 ? 8 : 16;
    rx_bit_duration_ = (baud_ > 64) ? (static_cast<double>(baud_) * samples_per_bit) / 64.0 : 16.0;
    
    // Total bits for rx
    u8 char_size = 5 + (ctrlc_ & 0x03U);
    if ((ctrlc_ & 0x07U) == 0x07U) char_size = 9;
    rx_total_bits_ = 1 + char_size + ((ctrlc_ & CTRLC_PMODE_MASK) ? 1 : 0) + ((ctrlc_ & CTRLC_SBMODE) ? 2 : 1);
    rx_bits_left_ = rx_total_bits_;
    rx_cycle_accumulator_ = 0.0;
}

void Uart8x::actually_push_to_fifo(u8 data, bool bit9) noexcept {
    bool mpc_en = (ctrlb_ & CTRLB_MPCM);
    if (mpc_en && !bit9) return;

    if (rx_fifo_count_ < 2) {
        u8 high = bit9 ? RXDATAH_DATA8 : 0;
        rx_fifo_[rx_fifo_write_idx_] = {data, high};
        rx_fifo_write_idx_ = (rx_fifo_write_idx_ + 1) % 2;
        rx_fifo_count_++;
        status_ |= STATUS_RXCIF;
    } else {
        rx_fifo_[(rx_fifo_write_idx_ + 1) % 2].high |= RXDATAH_BUFOVF;
    }
}

bool Uart8x::consume_transmitted_byte(u16& data) noexcept {
    if (status_ & STATUS_TXCIF) {
        data = tx_shift_reg_;
        return true;
    }
    return false;
}

} // namespace vioavr::core
