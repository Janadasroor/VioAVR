#include "vioavr/core/eusart.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"
#include <algorithm>

namespace vioavr::core {

Eusart::Eusart(std::string_view name, const EusartDescriptor& desc) noexcept
    : name_(name), desc_(desc)
{
    std::vector<u16> addrs = {
        desc.eudr_address, desc.eucsra_address, desc.eucsrb_address,
        desc.eucsrc_address, desc.mubrrl_address, desc.mubrrh_address
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
    eucsra_ = 0;
    eucsrb_ = 0;
    eucsrc_ = 0;
    mubrrl_ = 0;
    mubrrh_ = 0;
    tx_queue_.clear();
    rx_queue_.clear();
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
            if (eucsrb_ & 0x01) { // BODR: MSB first
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

    if (!tx_active_ && !tx_queue_.empty()) {
        tx_active_ = true;
        u32 data = tx_queue_.front();
        tx_queue_.pop_front();
        
        u8 utxs = (eucsra_ & desc_.utxs_mask) >> 4;
        u8 char_size = 8;
        switch (utxs) {
            case 0x00: char_size = 5; break;
            case 0x01: char_size = 6; break;
            case 0x02: char_size = 7; break;
            case 0x03: char_size = 8; break;
            case 0x07: char_size = 9; break;
            case 0x08: char_size = 13; break;
            case 0x09: char_size = 14; break;
            case 0x0A: char_size = 15; break;
            case 0x0B: char_size = 16; break;
            case 0x0F: char_size = 17; break;
            default: char_size = 8; break;
        }

        tx_shift_reg_ = data;
        tx_bit_count_ = char_size;
        tx_half_bit_ = false;
        cycles_to_next_bit_ = manchester ? cycles_per_half : cycles_per_bit;
        update_pin();
    }

    if (tx_active_) {
        update_pin(); // Ensure pin is correct at start of interval

        if (elapsed_cycles >= cycles_to_next_bit_) {
            u64 remaining = elapsed_cycles - cycles_to_next_bit_;
            
            if (manchester && !tx_half_bit_) {
                tx_half_bit_ = true;
                cycles_to_next_bit_ = cycles_per_half;
            } else {
                tx_half_bit_ = false;
                if (!(eucsrb_ & 0x01)) tx_shift_reg_ >>= 1;
                tx_bit_count_--;
                
                if (tx_bit_count_ == 0) {
                    tx_active_ = false;
                    eucsra_ |= 0x80;
                    cycles_to_next_bit_ = 0;
                } else {
                    cycles_to_next_bit_ = manchester ? cycles_per_half : cycles_per_bit;
                }
            }

            update_pin(); // Update pin immediately after state change

            if (remaining > 0 && tx_active_) {
                tick(remaining);
            }
        } else {
            cycles_to_next_bit_ -= elapsed_cycles;
        }
    }
}

u8 Eusart::read(u16 address) noexcept {
    if (address == desc_.eudr_address) {
        if (rx_queue_.empty()) return 0;
        u8 val = rx_queue_.front();
        rx_queue_.pop_front();
        return val;
    }
    if (address == desc_.eucsra_address) return eucsra_;
    if (address == desc_.eucsrb_address) return eucsrb_;
    if (address == desc_.eucsrc_address) return eucsrc_;
    if (address == desc_.mubrrl_address) return mubrrl_;
    if (address == desc_.mubrrh_address) return mubrrh_;
    return 0;
}

void Eusart::write(u16 address, u8 value) noexcept {
    if (address == desc_.eudr_address) {
        tx_queue_.push_back(value);
    } else if (address == desc_.eucsra_address) {
        eucsra_ = value;
    } else if (address == desc_.eucsrb_address) {
        eucsrb_ = value;
    } else if (address == desc_.eucsrc_address) {
        eucsrc_ = value;
    } else if (address == desc_.mubrrl_address) {
        mubrrl_ = value;
    } else if (address == desc_.mubrrh_address) {
        mubrrh_ = value;
    }
}

void Eusart::inject_received_byte(u8 data) noexcept {
    rx_queue_.push_back(data);
}

bool Eusart::consume_transmitted_byte(u8& data) noexcept {
    if (tx_queue_.empty()) return false;
    data = tx_queue_.front();
    tx_queue_.pop_front();
    return true;
}

bool Eusart::power_reduction_enabled() const noexcept {
    if (!bus_ || desc_.pr_address == 0U || desc_.pr_bit == 0xFFU) return false;
    return (bus_->read_data(desc_.pr_address) & (1U << desc_.pr_bit)) != 0;
}

} // namespace vioavr::core
