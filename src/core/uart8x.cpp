#include "vioavr/core/uart8x.hpp"
#include <algorithm>
#include <vector>

namespace vioavr::core {

Uart8x::Uart8x(const Uart8xDescriptor& descriptor) noexcept
    : desc_(descriptor)
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
    status_ = STATUS_DREIF | STATUS_TXCIF;
    baud_ = 0U;
    rx_data_ = 0U;
    tx_data_ = 0U;
    dbgctrl_ = 0x00U;
    tx_in_progress_ = false;
    tx_cycles_elapsed_ = 0;
    tx_duration_ = 160U;
}

void Uart8x::tick(u64 elapsed_cycles) noexcept {
    if (tx_in_progress_) {
        tx_cycles_elapsed_ += elapsed_cycles;
        if (tx_cycles_elapsed_ >= tx_duration_) {
            tx_in_progress_ = false;
            status_ |= (STATUS_DREIF | STATUS_TXCIF);
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
        status_ &= ~STATUS_RXCIF;
        return rx_data_;
    }
    if (address == desc_.rxdata_address + 1) return 0U; // Error flags
    if (address == desc_.dbgctrl_address) return dbgctrl_;
    return 0U;
}

void Uart8x::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) ctrla_ = value;
    else if (address == desc_.ctrlb_address) ctrlb_ = value;
    else if (address == desc_.ctrlc_address) ctrlc_ = value;
    else if (address == desc_.ctrld_address) ctrld_ = value;
    else if (address == desc_.status_address) {
        // Clear flags on write 1? No, Mega-0 USART flags are mostly clear-on-read or clear-on-action.
        // TXCIF is clear-on-write-1.
        if (value & STATUS_TXCIF) status_ &= ~STATUS_TXCIF;
    }
    else if (address == desc_.baud_address) baud_ = (baud_ & 0xFF00U) | value;
    else if (address == desc_.baud_address + 1) baud_ = (baud_ & 0x00FFU) | (static_cast<u16>(value) << 8U);
    else if (address == desc_.txdata_address) {
        tx_data_ = value;
        if (ctrlb_ & CTRLB_TXEN) {
            status_ &= ~(STATUS_DREIF | STATUS_TXCIF);
            tx_in_progress_ = true;
            tx_cycles_elapsed_ = 0;
            
            // Simplified baud calculation: duration = (64*16) / BAUD ? No, it's (64 * 16 * (1 + BAUD/64)) cycle-ish.
            // For now, use a fixed duration or derive from baud_.
            if (baud_ > 0) {
                // S = 16 * (1 + BAUD / 64)
                tx_duration_ = ((64LL * 16LL + (u64)baud_) * 10LL) / 64LL;
            } else {
                tx_duration_ = 160U;
            }
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
    
    if (request.vector_index == desc_.tx_vector_index) {
        status_ &= ~STATUS_TXCIF;
    }
    // RXC and DRE are hardware-cleared by action
    return true;
}

void Uart8x::inject_received_byte(u8 data) noexcept {
    rx_data_ = data;
    status_ |= STATUS_RXCIF;
}

bool Uart8x::consume_transmitted_byte(u8& data) noexcept {
    if (status_ & STATUS_TXCIF) {
        data = tx_data_;
        return true;
    }
    return false;
}

} // namespace vioavr::core
