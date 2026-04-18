#include "vioavr/core/uart.hpp"
#include <algorithm>
#include <vector>
#include <string>

namespace vioavr::core {

Uart::Uart(std::string_view name, const UartDescriptor& descriptor) noexcept
    : name_(std::string(name)), desc_(descriptor)
{
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
    tx_in_progress_ = false;
    tx_cycles_elapsed_ = 0;
    tx_duration_ = 160U; // Default bit-time in cycles (for UBRR=0)
}

void Uart::tick(const u64 elapsed_cycles) noexcept
{
    if (tx_in_progress_) {
        tx_cycles_elapsed_ += elapsed_cycles;
        
        if (tx_cycles_elapsed_ >= tx_duration_) {
            tx_in_progress_ = false;
            ucsra_ |= (desc_.udre_mask | desc_.txc_mask);
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
    if (address == desc_.udr_address) {
        udr_tx_ = value;
        ucsra_ &= ~(desc_.udre_mask | desc_.txc_mask);
        tx_in_progress_ = true;
        tx_cycles_elapsed_ = 0;
        
        const u16 ubrr = static_cast<u16>((static_cast<u16>(ubrrh_) << 8U) | ubrrl_);
        const u8 u2x = (ucsra_ & desc_.u2x_mask) ? 1U : 0U;
        tx_duration_ = (u2x ? 8 : 16) * (ubrr + 1U) * 10U;
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
    if ((ucsra_ & desc_.rxc_mask) && (ucsrb_ & desc_.rxcie_mask)) {
        request = {desc_.rx_vector_index, 0U};
        return true;
    }
    if ((ucsra_ & desc_.udre_mask) && (ucsrb_ & desc_.udrie_mask)) {
        request = {desc_.udre_vector_index, 0U};
        return true;
    }
    if ((ucsra_ & desc_.txc_mask) && (ucsrb_ & desc_.txcie_mask)) {
        request = {desc_.tx_vector_index, 0U};
        return true;
    }
    return false;
}

bool Uart::consume_interrupt_request(InterruptRequest& request) noexcept
{
    if (!pending_interrupt_request(request)) {
        return false;
    }
    
    if (request.vector_index == desc_.rx_vector_index) {
        ucsra_ &= ~desc_.rxc_mask;
    } else if (request.vector_index == desc_.tx_vector_index) {
        ucsra_ &= ~desc_.txc_mask;
    } else if (request.vector_index == desc_.udre_vector_index) {
        ucsra_ &= ~desc_.udre_mask;
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
    if (ucsra_ & desc_.txc_mask) {
        data = udr_tx_;
        return true;
    }
    return false;
}

}  // namespace vioavr::core
