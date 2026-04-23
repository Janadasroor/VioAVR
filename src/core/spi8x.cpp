#include "vioavr/core/spi8x.hpp"
#include "vioavr/core/port_mux.hpp"
#include <algorithm>
#include <vector>
#include "vioavr/core/evsys.hpp"

namespace vioavr::core {

Spi8x::Spi8x(const Spi8xDescriptor& descriptor) noexcept
    : desc_(descriptor)
{
    const std::array<u16, 5> addrs = {
        desc_.ctrla_address, desc_.ctrlb_address, desc_.intctrl_address,
        desc_.intflags_address, desc_.data_address
    };
    
    std::vector<u16> sorted;
    for (auto a : addrs) if (a != 0) sorted.push_back(a);
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

std::span<const AddressRange> Spi8x::mapped_ranges() const noexcept {
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

void Spi8x::reset() noexcept {
    ctrla_ = 0x00U;
    ctrlb_ = 0x00U;
    intctrl_ = 0x00U;
    intflags_ = 0x00U;
    data_ = 0x00U;
    tx_buffer_ = 0x00U;
    shift_register_ = 0x00U;
    tx_buffer_full_ = false;
    transfer_in_progress_ = false;
    transfer_cycles_elapsed_ = 0;
}

void Spi8x::tick(u64 elapsed_cycles) noexcept {
    if (!(ctrla_ & CTRLA_ENABLE)) return;

    for (u64 i = 0; i < elapsed_cycles; ++i) {
        if (transfer_in_progress_) {
            transfer_cycles_elapsed_++;
            
            if (port_mux_ && (ctrla_ & CTRLA_MASTER)) {
                u64 bit_duration = transfer_duration_ / 8;
                u8 bit_idx = static_cast<u8>(transfer_cycles_elapsed_ / bit_duration);
                if (bit_idx > 7) bit_idx = 7;

                // Simple bit-level driving for Master
                bool mosi = (shift_register_ >> (7 - bit_idx)) & 1;
                port_mux_->drive_spi_mosi(desc_.index, mosi ? PinLevel::high : PinLevel::low);
                
                // SCK toggle (simplified)
                bool sck = (transfer_cycles_elapsed_ % bit_duration) < (bit_duration / 2);
                if (ctrla_ & 0x10) sck = !sck; // CPOL
                port_mux_->drive_spi_sck(desc_.index, sck ? PinLevel::high : PinLevel::low);
                
                // SS Active Low
                port_mux_->drive_spi_ss(desc_.index, PinLevel::low);
            }

            if (transfer_cycles_elapsed_ >= transfer_duration_) {
                transfer_in_progress_ = false;
                intflags_ |= INTFLAGS_IF;
                
                // Finalize Master pins
                if (port_mux_ && (ctrla_ & CTRLA_MASTER)) {
                    port_mux_->drive_spi_ss(desc_.index, PinLevel::high);
                }

                // For now, simulate loopback by loading data_ with shift_register_
                // Or if MISO is driven, read it.
                if (port_mux_ && (ctrla_ & CTRLA_MASTER)) {
                    // Logic to read MISO bit-by-bit would be in the loop above
                }
                data_ = shift_register_;
                
                // Auto-load next byte if available
                if (tx_buffer_full_) {
                    shift_register_ = tx_buffer_;
                    tx_buffer_full_ = false;
                    transfer_in_progress_ = true;
                    transfer_cycles_elapsed_ = 0;
                }
            }
        } else if (tx_buffer_full_) {
            // Start delayed transfer
            shift_register_ = tx_buffer_;
            tx_buffer_full_ = false;
            transfer_in_progress_ = true;
            transfer_cycles_elapsed_ = 0;
        }
    }
}

u8 Spi8x::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.ctrlb_address) return ctrlb_;
    if (address == desc_.intctrl_address) return intctrl_;
    if (address == desc_.intflags_address) return intflags_;
    if (address == desc_.data_address) {
        intflags_ &= ~INTFLAGS_IF;
        return data_;
    }
    return 0U;
}

void Spi8x::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        u8 old_ctrla = ctrla_;
        ctrla_ = value;
        if (port_mux_) {
            if (!(value & CTRLA_ENABLE)) {
                port_mux_->drive_spi_mosi(desc_.index, PinLevel::high, false);
                port_mux_->drive_spi_sck(desc_.index, PinLevel::high, false);
                port_mux_->drive_spi_ss(desc_.index, PinLevel::high, false);
            } else if (!(old_ctrla & CTRLA_ENABLE)) {
                bool is_master = (value & CTRLA_MASTER);
                port_mux_->drive_spi_mosi(desc_.index, PinLevel::high, is_master);
                port_mux_->drive_spi_sck(desc_.index, (value & 0x10) ? PinLevel::high : PinLevel::low, is_master);
                if (is_master) port_mux_->drive_spi_ss(desc_.index, PinLevel::high, true);
            }
        }
    }
    else if (address == desc_.ctrlb_address) ctrlb_ = value;
    else if (address == desc_.intctrl_address) intctrl_ = value;
    else if (address == desc_.intflags_address) {
        if (value & INTFLAGS_IF) intflags_ &= ~INTFLAGS_IF;
    }
    else if (address == desc_.data_address) {
        if (!(ctrla_ & CTRLA_ENABLE)) return;

        if (tx_buffer_full_) {
            intflags_ |= INTFLAGS_WRCOL;
            return;
        }

        tx_buffer_ = value;
        tx_buffer_full_ = true;
        intflags_ &= ~INTFLAGS_IF;

        if (!transfer_in_progress_) {
            // Start immediately if idle
            shift_register_ = tx_buffer_;
            tx_buffer_full_ = false;
            transfer_in_progress_ = true;
            transfer_cycles_elapsed_ = 0;
        }

        // Recalculate duration in case it changed
        u32 divisor = 4; // Default is /4
        u8 presc = (ctrla_ >> 1) & 0x07U;
        switch(presc) {
            case 0: divisor = 4; break;
            case 1: divisor = 16; break;
            case 2: divisor = 64; break;
            case 3: divisor = 128; break;
            case 4: divisor = 2; break;
            case 5: divisor = 8; break;
            case 6: divisor = 32; break;
            default: divisor = 4; break;
        }
        if (ctrla_ & 0x80U) divisor /= 2;
        transfer_duration_ = static_cast<u64>(divisor) * 8U;
    }
}

bool Spi8x::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if ((intflags_ & INTFLAGS_IF) && (intctrl_ & INTCTRL_IE)) {
        request = {desc_.vector_index, 0U};
        return true;
    }
    return false;
}

bool Spi8x::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (!pending_interrupt_request(request)) return false;
    intflags_ &= ~INTFLAGS_IF;
    return true;
}
    
void Spi8x::set_event_system(EventSystem* evsys) noexcept {
    evsys_ = evsys;
    // SPI on 4809 doesn't have a direct EVSYS user register in some docs,
    // but the descriptor allows for it if the device metadata includes it.
    // We'll leave it prepared for future expansion or specific device needs.
}

} // namespace vioavr::core
