#include "vioavr/core/uart0.hpp"
#include <algorithm>
#include <vector>

namespace vioavr::core {

Uart0::Uart0(std::string_view name, const DeviceDescriptor& device) noexcept
    : name_(name), desc_(device.uart0)
{
    const std::array<u16, 4> addrs = {
        desc_.udr_address, desc_.ucsra_address, 
        desc_.ucsrb_address, desc_.ucsrc_address
    };
    std::vector<u16> sorted_addrs(addrs.begin(), addrs.end());
    std::sort(sorted_addrs.begin(), sorted_addrs.end());
    
    size_t ri = 0;
    ranges_.fill({0, 0});
    for (size_t i = 0; i < sorted_addrs.size(); ++i) {
        if (sorted_addrs[i] == 0) continue;
        if (ri > 0 && sorted_addrs[i] == ranges_[ri-1].end + 1) {
             ranges_[ri-1].end = sorted_addrs[i];
        } else {
             ranges_[ri++] = AddressRange{sorted_addrs[i], sorted_addrs[i]};
        }
    }
}

std::string_view Uart0::name() const noexcept
{
    return name_;
}

std::span<const AddressRange> Uart0::mapped_ranges() const noexcept
{
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

void Uart0::reset() noexcept
{
    udr_rx_ = 0U;
    udr_tx_ = 0U;
    ucsra_ = 0x20U; // UDRE initially set
    ucsrb_ = 0U;
    ucsrc_ = 0x06U; // default format
}

void Uart0::tick(const u64 elapsed_cycles) noexcept
{
    (void)elapsed_cycles;
}

u8 Uart0::read(const u16 address) noexcept
{
    if (address == desc_.udr_address) {
        ucsra_ &= 0x7FU; // Clear RXC
        return udr_rx_;
    }
    if (address == desc_.ucsra_address) return ucsra_;
    if (address == desc_.ucsrb_address) return ucsrb_;
    if (address == desc_.ucsrc_address) return ucsrc_;
    return 0U;
}

void Uart0::write(const u16 address, const u8 value) noexcept
{
    if (address == desc_.udr_address) {
        udr_tx_ = value;
        ucsra_ &= 0xDFU; // Clear UDRE (bit 5)
        // TXC (bit 6) logic: usually stays set until transmission starts?
        // Let's say it clears when you write a new byte and starts.
        ucsra_ &= 0xBFU; 
    } else if (address == desc_.ucsra_address) {
        // Most bits read-only, TXC can be cleared by writing 1
        if (value & 0x40U) ucsra_ &= 0xBFU;
    } else if (address == desc_.ucsrb_address) {
        ucsrb_ = value;
    } else if (address == desc_.ucsrc_address) {
        ucsrc_ = value;
    }
}

bool Uart0::pending_interrupt_request(InterruptRequest& request) const noexcept
{
    if ((ucsra_ & 0x80U) && (ucsrb_ & 0x80U)) {
        request = {desc_.rx_vector_index, 0U};
        return true;
    }
    if ((ucsra_ & 0x20U) && (ucsrb_ & 0x20U)) {
        request = {desc_.udre_vector_index, 0U};
        return true;
    }
    if ((ucsra_ & 0x40U) && (ucsrb_ & 0x40U)) {
        request = {desc_.tx_vector_index, 0U};
        return true;
    }
    return false;
}

bool Uart0::consume_interrupt_request(InterruptRequest& request) noexcept
{
    if (pending_interrupt_request(request)) {
        if (request.vector_index == desc_.tx_vector_index) ucsra_ &= 0xBFU;
        return true;
    }
    return false;
}

void Uart0::inject_received_byte(const u8 data) noexcept
{
    udr_rx_ = data;
    ucsra_ |= 0x80U; // Set RXC (bit 7)
}

bool Uart0::consume_transmitted_byte(u8& data) noexcept
{
    if (!(ucsra_ & 0x20U) || (ucsra_ & 0x40U)) { // If we have something in TX buffer (UDRE=0) or just finished (TXC=1)
        // This is a bit simplified. If UDRE=0, it means we have data pending to be sent.
        // If the test calls this, it expects to see the byte we just wrote.
        if (!(ucsra_ & 0x20U)) {
            data = udr_tx_;
            ucsra_ |= 0x60U; // Set UDRE and TXC back
            return true;
        }
    }
    return false;
}

}  // namespace vioavr::core
