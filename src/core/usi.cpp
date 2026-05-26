#include "vioavr/core/usi.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"

namespace vioavr::core {

// USICR bits
constexpr u8 USISIE   = 0x80;
constexpr u8 USIOIE   = 0x40;
constexpr u8 USIWM1   = 0x20;
constexpr u8 USIWM0   = 0x10;
constexpr u8 USICS1   = 0x08;
constexpr u8 USICS0   = 0x04;
constexpr u8 USICLK   = 0x02;
constexpr u8 USITC    = 0x01;

// USISR bits
constexpr u8 USISIF    = 0x80;
constexpr u8 USIOIF    = 0x40;
constexpr u8 USIPF     = 0x20;
constexpr u8 USIDC     = 0x10;
constexpr u8 USICNT_MASK = 0x0F;

Usi::Usi(std::string_view name, const UsiDescriptor& desc) noexcept
    : name_(name), desc_(desc)
{
    if (desc_.usicr_address != 0) {
        ranges_[range_count_++] = {desc_.usicr_address, desc_.usicr_address};
    }
    if (desc_.usisr_address != 0) {
        ranges_[range_count_++] = {desc_.usisr_address, desc_.usisr_address};
    }
    if (desc_.usidr_address != 0) {
        ranges_[range_count_++] = {desc_.usidr_address, desc_.usidr_address};
    }
    if (desc_.usibr_address != 0) {
        ranges_[range_count_++] = {desc_.usibr_address, desc_.usibr_address};
    }
}

std::span<const AddressRange> Usi::mapped_ranges() const noexcept {
    return {ranges_.data(), range_count_};
}

void Usi::reset() noexcept {
    usicr_ = 0;
    usisr_ = 0;
    usidr_ = 0;
    usibr_ = 0;
    int_pending_ = false;
    prev_sda_ = false;
    prev_scl_ = false;
    prev_do_ = false;
    sda_driven_ = false;
}

void Usi::tick(u64 elapsed_cycles) noexcept {
    if (!bus_ || desc_.scl_pin_address == 0) return;
    auto* pm = bus_->pin_mux();
    if (!pm) return;

    for (u64 i = 0; i < elapsed_cycles; ++i) {
        bool cur_sda = pm->get_state_by_address(desc_.sda_pin_address, desc_.sda_pin_bit).drive_level;
        bool cur_scl = pm->get_state_by_address(desc_.scl_pin_address, desc_.scl_pin_bit).drive_level;

        if (is_twowire()) {
            if (prev_scl_ && cur_scl && prev_sda_ && !cur_sda) {
                usisr_ |= USISIF;
                usisr_ &= ~(USIPF | USIDC);
                update_interrupt_pending();
            }
            if (prev_scl_ && cur_scl && !prev_sda_ && cur_sda) {
                usisr_ |= USIPF;
                update_interrupt_pending();
            }
        }

        if (!prev_scl_ && cur_scl) {
            u8 clk_src = usicr_ & (USICS1 | USICS0);
            if (clk_src == 0x00 || clk_src == 0x04) {
                bool bit_in = cur_sda;
                u8 cnt = usisr_ & USICNT_MASK;
                if (cnt > 0) {
                    usidr_ = (usidr_ << 1) | (bit_in ? 1 : 0);
                }
                if (cnt < 15) {
                    usisr_ = (usisr_ & ~USICNT_MASK) | ((cnt + 1) & USICNT_MASK);
                }
                if ((cnt + 1) >= 16) {
                    usisr_ &= ~USICNT_MASK;
                    usisr_ |= USIOIF;
                    update_interrupt_pending();
                }
            }
        }

        if (!is_twowire() && desc_.do_pin_address != 0) {
            bool do_val = (usidr_ & 0x80) != 0;
            pm->update_pin_by_address(desc_.do_pin_address, desc_.do_pin_bit, PinOwner::spi, true, do_val);
        }

        prev_sda_ = cur_sda;
        prev_scl_ = cur_scl;
    }
}

u8 Usi::reg_offset(u16 address) const noexcept {
    if (address == desc_.usicr_address) return 0;
    if (address == desc_.usisr_address) return 1;
    if (address == desc_.usidr_address) return 2;
    if (address == desc_.usibr_address) return 3;
    return 0xFF;
}

u8 Usi::read(u16 address) noexcept {
    switch (reg_offset(address)) {
    case 0: return usicr_ & ~(USICLK | USITC); // USICLK and USITC read as 0
    case 1: return usisr_;
    case 2: return usidr_;
    case 3: return usibr_;
    default: return 0;
    }
}

bool Usi::is_twowire() const noexcept {
    return (usicr_ & (USIWM1 | USIWM0)) != 0; // both 2-wire modes
}

void Usi::write(u16 address, u8 value) noexcept {
    switch (reg_offset(address)) {
    case 0: {
        u8 clock_strobe = value & USICLK;
        u8 toggle_clock = value & USITC;

        // Preserve USICLK (write-only, reads as 0) and USITC (reads as 0)
        usicr_ = (value & (USISIE | USIOIE | USIWM1 | USIWM0 | USICS1 | USICS0)) | USICLK;

        // Clock strobe: shift data and increment/decrement counter
        if (clock_strobe) {
            usicr_ |= USICLK;
            u8 cnt = usisr_ & USICNT_MASK;
            bool bit_in = false;
            if (bus_ && bus_->pin_mux()) {
                if (is_twowire()) {
                    bit_in = bus_->pin_mux()->get_state_by_address(desc_.sda_pin_address, desc_.sda_pin_bit).drive_level;
                } else {
                    bit_in = bus_->pin_mux()->get_state_by_address(desc_.sda_pin_address, desc_.sda_pin_bit).drive_level;
                }
            }
            usidr_ = (usidr_ << 1) | (bit_in ? 1 : 0);
            if (cnt < 15) {
                usisr_ = (usisr_ & ~USICNT_MASK) | ((cnt + 1) & USICNT_MASK);
            }
            if ((cnt + 1) >= 16) {
                usisr_ &= ~USICNT_MASK; // wrap to 0
                usisr_ |= USIOIF;
            }
            update_interrupt_pending();
        } else {
            usicr_ &= ~USICLK;
        }

        // Toggle clock pin
        if (toggle_clock && desc_.scl_pin_address != 0) {
            // Clock pin toggling handled externally via pin_mux
        }

        // If software clock source, counter also increments on USICLK=1
        // handled above

        // USICS field selects clock source
        // 00 = external, 01 = external + software, 10 = reserved/TIMER0, 11 = software
        break;
    }
    case 1: {
        // Write to USISR: USICNT lower nibble is written directly
        // Upper nibble flags are write-1-to-clear
        u8 flag_clear = value & 0xF0;
        u8 cnt_set = value & USICNT_MASK;

        usisr_ = (usisr_ & ~flag_clear); // Clear flags written as 1
        usisr_ = (usisr_ & ~USICNT_MASK) | cnt_set; // Set counter
        // Loading USICNT with non-zero clears USISIF per datasheet
        if (cnt_set > 0) usisr_ &= ~USISIF;
        update_interrupt_pending();
        break;
    }
    case 2:
        usidr_ = value;
        break;
    case 3:
        // USIBR is read-only
        break;
    }
}

bool Usi::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (!int_pending_) return false;
    if ((usicr_ & USISIE) && (usisr_ & USISIF)) {
        request = InterruptRequest{.vector_index = desc_.start_vector_index, .source_id = 0U};
        return true;
    }
    if ((usicr_ & USIOIE) && (usisr_ & USIOIF)) {
        request = InterruptRequest{.vector_index = desc_.overflow_vector_index, .source_id = 0U};
        return true;
    }
    return false;
}

bool Usi::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (!int_pending_) return false;
    if ((usicr_ & USISIE) && (usisr_ & USISIF)) {
        request = InterruptRequest{.vector_index = desc_.start_vector_index, .source_id = 0U};
        usisr_ &= ~USISIF;
        update_interrupt_pending();
        return true;
    }
    if ((usicr_ & USIOIE) && (usisr_ & USIOIF)) {
        request = InterruptRequest{.vector_index = desc_.overflow_vector_index, .source_id = 0U};
        usisr_ &= ~USIOIF;
        update_interrupt_pending();
        return true;
    }
    return false;
}

void Usi::update_interrupt_pending() noexcept {
    int_pending_ = false;
    if ((usicr_ & USISIE) && (usisr_ & USISIF)) int_pending_ = true;
    if ((usicr_ & USIOIE) && (usisr_ & USIOIF)) int_pending_ = true;
}

} // namespace vioavr::core
