#include "vioavr/core/uart.hpp"
#include "vioavr/core/memory_bus.hpp"
#include <algorithm>
#include <vector>
#include <string>

namespace vioavr::core {

namespace {
constexpr u8 kFeMask = 0x10U;
constexpr u8 kDorMask = 0x08U;
constexpr u8 kUpeMask = 0x04U;
}

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
    ucsrc_ = desc_.ucsrc_reset;
    ubrrh_ = 0U;
    ubrrl_ = 0U;
    tx_active_ = false;
    rx_active_ = false;
    tx_cycle_accumulator_ = 0;
    rx_cycle_accumulator_ = 0;
    tx_bit_duration_ = 160U;
    tx_output_queue_.clear();
    tx_buffer_full_ = false;
    rx_shift_reg_ = 0;
    rx_bits_left_ = 0;
    update_interrupt_state();
}

void Uart::tick(const u64 elapsed_cycles) noexcept
{
    if (power_reduction_enabled()) return;

    const u8 old_ucsra = ucsra_;

    // Start transmission if buffer is full and shift register is idle
    if (!tx_active_ && tx_buffer_full_ && (ucsrb_ & desc_.txen_mask)) {
        tx_shift_reg_ = ((ucsrb_ & 0x01U) << 8) | udr_tx_;
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
        u8 data_bits = (ucsrb_ & 0x04U) ? 9 : ((ucsrc_ >> 1) & 0x03U) + 5;
        u8 parity_bits = ((ucsrc_ & 0x30U) != 0) ? 1 : 0;
        u8 stop_bits = (ucsrc_ & 0x08U) ? 2 : 1;
        const u64 limit = bit_duration * (1ULL + data_bits + parity_bits + stop_bits);
        
        if (tx_cycle_accumulator_ >= limit) {
            // Byte finished shifting
            tx_output_queue_.push_back(static_cast<u16>(tx_shift_reg_));
            ucsra_ |= desc_.txc_mask;
            
            // Check for next byte in buffer
            if (tx_buffer_full_) {
                tx_shift_reg_ = ((ucsrb_ & 0x01U) << 8) | udr_tx_;
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

    // Abort RX if receiver disabled mid-reception
    if ((ucsrb_ & desc_.rxen_mask) == 0U) rx_active_ = false;

    // RX timing
    if (rx_active_) {
        rx_cycle_accumulator_ += elapsed_cycles;
        u16 ubrr = (static_cast<u16>(ubrrh_) << 8) | ubrrl_;
        u32 bit_duration = ((ucsra_ & desc_.u2x_mask) ? 8U : 16U) * (ubrr + 1U);
        u8 data_bits = (ucsrb_ & 0x04U) ? 9 : ((ucsrc_ >> 1) & 0x03U) + 5;
        u8 parity_bits = ((ucsrc_ & 0x30U) != 0) ? 1 : 0;
        u8 stop_bits = (ucsrc_ & 0x08U) ? 2 : 1;
        const u64 limit = bit_duration * (1ULL + data_bits + parity_bits + stop_bits);
        if (rx_cycle_accumulator_ >= limit) {
            rx_active_ = false;
            ucsra_ |= desc_.rxc_mask;
        }
    }

    if (ucsra_ != old_ucsra) {
        update_interrupt_state();
    }
}

u8 Uart::read(const u16 address) noexcept
{
    if (address == desc_.udr_address) {
        ucsra_ &= ~(desc_.rxc_mask | kDorMask);
        update_interrupt_state();
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
        ucsra_ = static_cast<u8>((ucsra_ & ~(desc_.u2x_mask | desc_.mpcm_mask)) | (value & (desc_.u2x_mask | desc_.mpcm_mask)));
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
    update_interrupt_state();
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
        update_interrupt_state();
    }
    return true;
}

void Uart::inject_received_byte(const u16 data) noexcept
{
    if ((ucsrb_ & desc_.rxen_mask) == 0U) return;

    if ((ucsra_ & desc_.rxc_mask) != 0U || rx_active_) {
        ucsra_ |= kDorMask;
        return;
    }

    rx_shift_reg_ = data;
    udr_rx_ = static_cast<u8>(data & 0xFFU);
    ucsrb_ = static_cast<u8>((ucsrb_ & ~0x02U) | ((data >> 7) & 0x02U));
    rx_cycle_accumulator_ = 0;
    rx_active_ = true;
    update_interrupt_state();
}

bool Uart::consume_transmitted_byte(u16& data) noexcept
{
    if (tx_output_queue_.empty()) return false;
    data = tx_output_queue_.front();
    tx_output_queue_.pop_front();
    return true;
}

bool Uart::power_reduction_enabled() const noexcept
{
    if (!bus_ || desc_.pr_address == 0U || desc_.pr_bit == 0xFFU) {
        return false;
    }
    return (bus_->read_data(desc_.pr_address) & (1U << desc_.pr_bit)) != 0;
}

void Uart::update_interrupt_state() noexcept
{
    const bool rxc = (ucsra_ & desc_.rxc_mask) && (ucsrb_ & desc_.rxcie_mask);
    const bool udre = (ucsra_ & desc_.udre_mask) && (ucsrb_ & desc_.udrie_mask);
    const bool txc = (ucsra_ & desc_.txc_mask) && (ucsrb_ & desc_.txcie_mask);
    set_interrupt_pending(rxc || udre || txc);
}

} // namespace vioavr::core
