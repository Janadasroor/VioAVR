#include "vioavr/core/uart.hpp"
#include <algorithm>
#include <vector>
#include <string>

namespace vioavr::core {

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
    ucsrc_ = 0x06U; // default format (8N1)
    ubrrh_ = 0U;
    ubrrl_ = 0U;
    tx_active_ = false;
    rx_active_ = false;
    tx_cycle_accumulator_ = 0;
    rx_cycle_accumulator_ = 0;
    tx_bit_duration_ = 160U;
    tx_output_queue_.clear();
    tx_buffer_full_ = false;
}

void Uart::tick(const u64 elapsed_cycles) noexcept
{
    // Start transmission if buffer is full and shift register is idle
    if (!tx_active_ && tx_buffer_full_) {
        tx_shift_reg_ = udr_tx_;
        tx_active_ = true;
        tx_buffer_full_ = false;
        tx_cycle_accumulator_ = 0;
        ucsra_ |= desc_.udre_mask;   // Buffer now empty
        ucsra_ &= ~desc_.txc_mask;  // New transmission clears TXC
    }

    if (tx_active_) {
        tx_cycle_accumulator_ += elapsed_cycles;
        u16 ubrr = (static_cast<u16>(ubrrh_) << 8) | ubrrl_;
        u32 bit_duration = ((ucsra_ & desc_.u2x_mask) ? 8U : 16U) * (ubrr + 1U);
        const u64 limit = bit_duration * 10;
        
        if (tx_cycle_accumulator_ >= limit) {
            // Byte finished shifting
            tx_output_queue_.push_back(static_cast<u8>(tx_shift_reg_));
            ucsra_ |= desc_.txc_mask;
            
            // Check for next byte in buffer
            if (tx_buffer_full_) {
                tx_shift_reg_ = udr_tx_;
                tx_buffer_full_ = false;
                ucsra_ |= desc_.udre_mask;
                ucsra_ &= ~desc_.txc_mask;
                tx_cycle_accumulator_ -= limit;
                // tx_active remains true
            } else {
                tx_active_ = false;
            }
        }
    }
}

u8 Uart::read(const u16 address) noexcept
{
    if (address == desc_.udr_address) {
        ucsra_ &= ~desc_.rxc_mask;
        return udr_rx_;
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
        udr_tx_ = value;
        tx_buffer_full_ = true;
        ucsra_ &= ~desc_.udre_mask; // Buffer full
    } else if (address == desc_.ucsra_address) {
        ucsra_ = static_cast<u8>((ucsra_ & ~desc_.u2x_mask) | (value & desc_.u2x_mask));
        if (value & desc_.txc_mask) ucsra_ &= ~desc_.txc_mask;
    } else if (address == desc_.ucsrb_address) {
        ucsrb_ = value;
    } else if (address == desc_.ucsrc_address) {
        ucsrc_ = value;
    } else if (address == desc_.ubrrl_address) {
        ubrrl_ = value;
    } else if (address == desc_.ubrrh_address) {
        ubrrh_ = value & 0x0FU;
    }
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
    }
    return true;
}

void Uart::inject_received_byte(const u8 data) noexcept
{
    udr_rx_ = data;
    ucsra_ |= desc_.rxc_mask;
}

bool Uart::consume_transmitted_byte(u8& data) noexcept
{
    if (tx_output_queue_.empty()) return false;
    data = tx_output_queue_.front();
    tx_output_queue_.pop_front();
    return true;
}

}  // namespace vioavr::core
