#include "vioavr/core/lin.hpp"
#include "vioavr/core/memory_bus.hpp"
#include <algorithm>

namespace vioavr::core {

LinUART::LinUART(const LinDescriptor& desc) noexcept : desc_(desc) {
    if (desc_.ctrla_address != 0) {
        ranges_[0] = {desc_.ctrla_address, static_cast<u16>(desc_.ctrla_address + 7)};
    }
}

std::span<const AddressRange> LinUART::mapped_ranges() const noexcept {
    return {ranges_.data(), desc_.ctrla_address != 0 ? 1U : 0U};
}

void LinUART::reset() noexcept {
    lincr_ = 0;
    linsir_ = 0;
    linenir_ = 0;
    linerr_ = 0;
    linbtr_ = 0;
    linidr_ = 0;
    lindat_ = 0;
    state_ = State::Idle;
    fifo_.clear();
    fifo_idx_ = 0;
    checksum_ = 0;
    bit_timer_ = 0;
    tx_bit_pos_ = 0;
    tx_shift_reg_ = 0;
    tx_active_ = false;
    rx_shift_reg_ = 0;
    rx_bits_left_ = 0;
    rx_active_ = false;
    rx_current_byte_ = 0;
    rx_data_valid_ = false;
    int_pending_ = false;
    if (tx_pin_address_)
        pin_mux_->release_pin_by_address(tx_pin_address_, tx_pin_bit_, PinOwner::uart);
}

void LinUART::tick(u64 elapsed_cycles) noexcept {
    if (power_reduction_enabled()) return;
    u64 baud = 9600;
    if (linbtr_ > 0) baud = 16'000'000 / (linbtr_ & 0x7F);
    u64 bit_duration = 16'000'000 / baud;
    if (bit_duration < 8) bit_duration = 8;

    if (tx_active_) {
        bit_timer_ += elapsed_cycles;
        if (bit_timer_ >= bit_duration) {
            bit_timer_ -= bit_duration;
            bool bit_val = (tx_shift_reg_ >> tx_bit_pos_) & 1U;
            if (tx_bit_pos_ == 0) {
                bit_val = false;
            } else if (tx_bit_pos_ == 9) {
                bit_val = true;
            }
            if (tx_pin_address_)
                pin_mux_->update_pin_by_address(tx_pin_address_, tx_pin_bit_,
                    PinOwner::uart, true, !bit_val);
            tx_bit_pos_++;
            if (tx_bit_pos_ >= 10) {
                tx_active_ = false;
                if (state_ == State::Idle && !fifo_.empty()) {
                    linsir_ &= ~desc_.linsir_lbusy_mask;
                    linsir_ |= desc_.linsir_ltxok_mask;
                }
            }
        }
    }

    if (!(lincr_ & 0x08)) return;
    u8 lcmd = lincr_ & desc_.lincr_lcmd_mask;
    if (lcmd == 0x01 && !tx_active_ && !fifo_.empty()) {
        u8 byte = fifo_[fifo_idx_];
        tx_shift_reg_ = (0x0200) | (static_cast<u16>(byte) << 1);
        tx_bit_pos_ = 0;
        tx_active_ = true;
        bit_timer_ = 0;
        linsir_ |= desc_.linsir_lbusy_mask;
    }
}

u8 LinUART::read(u16 address) noexcept {
    u16 offset = address - desc_.ctrla_address;
    switch (offset) {
        case 0: return lincr_;
        case 1: return linsir_;
        case 2: return linenir_;
        case 3: return linerr_;
        case 4: return linbtr_;
        case 6: return linidr_;
        case 7: return lindat_;
        default: return 0;
    }
}

void LinUART::write(u16 address, u8 value) noexcept {
    u16 offset = address - desc_.ctrla_address;
    switch (offset) {
        case 0: {
            lincr_ = value;
            if (lincr_ & desc_.lincr_lswres_mask) {
                reset();
                lincr_ &= ~desc_.lincr_lswres_mask;
            }
            u8 lcmd = lincr_ & desc_.lincr_lcmd_mask;
            if (lcmd == 0x01) {
                state_ = State::SyncBreak;
                linsir_ |= desc_.linsir_lbusy_mask;
                fifo_.clear();
                expected_data_len_ = get_frame_length();
            } else if (lcmd == 0x02) {
                state_ = State::SyncBreak;
                linsir_ |= desc_.linsir_lbusy_mask;
                fifo_.clear();
                expected_data_len_ = get_frame_length();
            }
            break;
        }
        case 1:
            linsir_ &= ~(value & (desc_.linsir_lerr_mask | desc_.linsir_lidok_mask |
                                  desc_.linsir_ltxok_mask | desc_.linsir_lrxok_mask));
            break;
        case 2: linenir_ = value; break;
        case 3: linerr_ = value; break;
        case 4: linbtr_ = value; break;
        case 6: {
            linidr_ = value;
            linsir_ &= ~desc_.linsir_lidst_mask;
            linsir_ |= (value & 0x3FU) << 4;
            break;
        }
        case 7: {
            lindat_ = value;
            if (state_ == State::Data) {
                fifo_.push_back(value);
                calculate_checksum(value);
                if (fifo_.size() >= expected_data_len_) {
                    state_ = State::Checksum;
                    linsir_ &= ~desc_.linsir_lbusy_mask;
                    linsir_ |= desc_.linsir_ltxok_mask;
                    state_ = State::Idle;
                }
            } else if (state_ == State::Idle) {
                fifo_.push_back(value);
            }
            break;
        }
    }
}

void LinUART::update_interrupt_pending() noexcept {
    if ((linsir_ & 0x0FU) & (linenir_ & 0x0FU)) {
        int_pending_ = true;
    } else {
        int_pending_ = false;
    }
}

bool LinUART::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (int_pending_ && desc_.vector_index != 0xFF) {
        request.vector_index = desc_.vector_index;
        return true;
    }
    return false;
}

bool LinUART::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (int_pending_ && desc_.vector_index != 0xFF) {
        request.vector_index = desc_.vector_index;
        int_pending_ = false;
        return true;
    }
    return false;
}

void LinUART::simulate_rx_break() noexcept {
    if (state_ == State::SyncBreak || state_ == State::Idle) {
        state_ = State::SyncField;
    }
}

void LinUART::simulate_rx_byte(u8 data) noexcept {
    switch (state_) {
        case State::SyncField:
            if (data == 0x55) state_ = State::Identifier;
            else { linerr_ |= 0x01; linsir_ |= desc_.linsir_lerr_mask; state_ = State::Idle; }
            break;
        case State::Identifier:
            linidr_ = data;
            linsir_ |= desc_.linsir_lidok_mask;
            expected_data_len_ = get_frame_length();
            state_ = State::Data;
            fifo_.clear();
            checksum_ = 0;
            break;
        case State::Data:
            fifo_.push_back(data);
            calculate_checksum(data);
            lindat_ = data;
            if (fifo_.size() >= expected_data_len_) {
                state_ = State::Checksum;
            }
            break;
        case State::Checksum:
            if (data == ((0xFFU - checksum_) & 0xFFU)) {
                linsir_ |= desc_.linsir_lrxok_mask;
            } else {
                linerr_ |= 0x08;
                linsir_ |= desc_.linsir_lerr_mask;
            }
            state_ = State::Idle;
            linsir_ &= ~desc_.linsir_lbusy_mask;
            break;
        default:
            break;
    }
    update_interrupt_pending();
}

std::vector<u8> LinUART::get_tx_data() const noexcept {
    return fifo_;
}

void LinUART::calculate_checksum(u8 data) noexcept {
    u16 sum = checksum_ + data;
    if (sum > 255) sum = (sum & 0xFFU) + 1;
    checksum_ = static_cast<u8>(sum);
}

u8 LinUART::get_frame_length() const noexcept {
    u8 conf = (lincr_ & desc_.lincr_lconf_mask) >> 4;
    switch (conf) {
        case 0: return 2;
        case 1: return 4;
        case 2: return 8;
        case 3: {
            u8 id = linidr_ & 0x3FU;
            if (id <= 31) return 2;
            if (id <= 47) return 4;
            return 8;
        }
        default: return 0;
    }
}

void LinUART::evaluate_interrupts() noexcept {
    update_interrupt_pending();
}

bool LinUART::power_reduction_enabled() const noexcept {
    if (!bus_ || desc_.pr_address == 0U || desc_.pr_bit == 0xFFU) return false;
    return (bus_->read_data(desc_.pr_address) & (1U << desc_.pr_bit)) != 0;
}

} // namespace vioavr::core
