#include "vioavr/core/eusart.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"
#include <algorithm>

namespace vioavr::core {

Eusart::Eusart(std::string_view name, const EusartDescriptor& desc) noexcept
    : name_(name), desc_(desc)
{
    std::vector<u16> addrs = {
        desc.eudr_address, desc.eudr_address + 1,
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

    auto get_char_size = [&](bool tx) -> u8 {
        u8 val = tx ? ((eucsra_ & desc_.utxs_mask) >> 4) : (eucsra_ & 0x0F);
        bool f1617 = (eucsrc_ & desc_.f1617_mask);
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
            case 0x0F: return f1617 ? 17 : 8; // 17 if F1617 set
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

        u64 tx_elapsed = elapsed_cycles;
        while (tx_active_ && tx_elapsed >= cycles_to_next_bit_) {
            tx_elapsed -= cycles_to_next_bit_;
            
            if (manchester && !tx_half_bit_) {
                tx_half_bit_ = true;
                cycles_to_next_bit_ = cycles_per_half;
            } else {
                tx_half_bit_ = false;
                if (!(eucsrb_ & 0x01)) tx_shift_reg_ >>= 1; // LSB first
                else tx_shift_reg_ <<= 1; // MSB first
                
                tx_bit_count_--;
                
                if (tx_bit_count_ == 0) {
                    tx_active_ = false;
                    eucsra_ |= 0x80; // TX Complete
                    cycles_to_next_bit_ = 0;
                } else {
                    cycles_to_next_bit_ = manchester ? cycles_per_half : cycles_per_bit;
                }
            }
            update_pin();
        }
        if (tx_active_) cycles_to_next_bit_ -= tx_elapsed;
    }

    // --- RX Logic ---
    if (eucsrb_ & desc_.eus_en_mask) {
        bool current_rx = bus_->pin_mux()->get_state_by_address(desc_.rxd_pin_address, desc_.rxd_pin_bit).drive_level;
        
        if (!rx_active_) {
            // Wait for Start bit edge (Manchester '1' is LOW/HIGH, so falling edge at start)
            if (current_rx == false && rx_last_pin_level_ == true) {
                rx_active_ = true;
                rx_bit_count_ = get_char_size(false); // Data bits to follow start bit
                rx_shift_reg_ = 0;
                // Skip the rest of the start bit (1 bit period) and sample first data bit at 3/4
                rx_cycles_to_sample_ = cycles_per_bit + (cycles_per_bit * 3) / 4;
            }
        } else {
            u64 rx_elapsed = elapsed_cycles;
            while (rx_active_ && rx_elapsed >= rx_cycles_to_sample_) {
                rx_elapsed -= rx_cycles_to_sample_;
                
                // Sample bit
                bool sampled_val = current_rx;
                u8 total_bits = get_char_size(false);
                
                if (!(eucsrb_ & 0x01)) { // LSB First
                    if (sampled_val) rx_shift_reg_ |= (1U << (total_bits - rx_bit_count_));
                } else { // MSB First
                    rx_shift_reg_ = (rx_shift_reg_ << 1) | (sampled_val ? 1 : 0);
                }
                
                rx_bit_count_--;
                if (rx_bit_count_ == 0) {
                    rx_active_ = false;
                    rx_queue_.push_back(rx_shift_reg_);
                    rx_cycles_to_sample_ = 0;
                } else {
                    rx_cycles_to_sample_ = cycles_per_bit;
                }
            }
            if (rx_active_) rx_cycles_to_sample_ -= rx_elapsed;
        }
        rx_last_pin_level_ = current_rx;
    }
}

u8 Eusart::read(u16 address) noexcept {
    if (address == desc_.eudr_address) { // Low byte
        if (rx_queue_.empty()) return 0;
        u32 val = rx_queue_.front();
        rx_queue_.pop_front();
        
        rx_temp_ = (val >> 8);
        return static_cast<u8>(val & 0xFF);
    }
    if (address == desc_.eudr_address + 1) { // High byte
        return static_cast<u8>(rx_temp_);
    }
    if (address == desc_.eucsra_address) return eucsra_;
    if (address == desc_.eucsrb_address) return eucsrb_;
    if (address == desc_.eucsrc_address) return eucsrc_;
    if (address == desc_.mubrrl_address) return mubrrl_;
    if (address == desc_.mubrrh_address) return mubrrh_;
    return 0;
}

void Eusart::write(u16 address, u8 value) noexcept {
    if (address == desc_.eudr_address + 1) { // High byte first
        tx_temp_ = value;
    } else if (address == desc_.eudr_address) { // Low byte triggers
        u32 full = (static_cast<u32>(tx_temp_) << 8) | value;
        tx_queue_.push_back(full);
        tx_temp_ = 0; // Reset for next use
    } else if (address == desc_.eucsra_address) {
        eucsra_ = value;
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
