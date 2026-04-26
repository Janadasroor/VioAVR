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
    
    bool enabled = (eucsrb_ & 0x10);
    if (!enabled) {
        // Release pins if they were claimed
        if (desc_.txd_pin_address) bus_->pin_mux()->release_pin_by_address(desc_.txd_pin_address, desc_.txd_pin_bit, PinOwner::uart);
        if (desc_.rxd_pin_address) bus_->pin_mux()->release_pin_by_address(desc_.rxd_pin_address, desc_.rxd_pin_bit, PinOwner::uart);
        return;
    }

    // Claim pins
    if (desc_.txd_pin_address) {
        bus_->pin_mux()->claim_pin_by_address(desc_.txd_pin_address, desc_.txd_pin_bit, PinOwner::uart);
        // TXD is an output from peripheral's perspective when transmitting
        bus_->pin_mux()->update_pin_by_address(desc_.txd_pin_address, desc_.txd_pin_bit, PinOwner::uart, true, !tx_active_ || (tx_shift_reg_ & 1));
    }
    if (desc_.rxd_pin_address) {
        bus_->pin_mux()->claim_pin_by_address(desc_.rxd_pin_address, desc_.rxd_pin_bit, PinOwner::uart);
    }

    u16 mubrr = (static_cast<u16>(mubrrh_) << 8) | mubrrl_;
    u64 cycles_per_bit = 16ULL * (mubrr + 1);
    if (eucsrb_ & desc_.emch_mask) {
        // Manchester mode often uses higher oversampling or different clocking
        // For simulation, we'll assume it's roughly the same bit period for now.
    }

    if (!tx_active_ && !tx_queue_.empty()) {
        tx_active_ = true;
        tx_shift_reg_ = tx_queue_.front();
        tx_queue_.pop_front();
        tx_bit_count_ = 10; // Start + 8 + Stop
        cycles_to_next_bit_ = cycles_per_bit;
        
        // Clear EOT in EUCSRA if needed (Bit 7 is UTxOK/EOT)
        // eucsra_ &= ~0x80; 
    }

    if (tx_active_) {
        if (elapsed_cycles >= cycles_to_next_bit_) {
            u64 remaining = elapsed_cycles - cycles_to_next_bit_;
            tx_bit_count_--;
            if (tx_bit_count_ == 0) {
                tx_active_ = false;
                // Set EOT (bit 7)
                eucsra_ |= 0x80;
                cycles_to_next_bit_ = 0;
            } else {
                cycles_to_next_bit_ = cycles_per_bit;
                if (remaining > 0) {
                    // Recurse or handle multiple bits in one tick
                    tick(remaining);
                }
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
