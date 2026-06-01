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
constexpr u8 USICS_EXTERNAL = 0x00;
constexpr u8 USICS_EXTSW    = 0x04;
constexpr u8 USICS_TIMER0   = 0x08;
constexpr u8 USICS_SWONLY   = 0x0C;

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
    timer0_clock_pending_ = 0;
}

void Usi::on_timer0_clock() noexcept {
    u8 clk_src = usicr_ & (USICS1 | USICS0);
    if (clk_src == USICS_TIMER0) {
        timer0_clock_pending_++;
    }
}

void Usi::tick(u64 elapsed_cycles) noexcept {
    if (!bus_) return;
    auto* pm = bus_->pin_mux();

    u8 clk_src = usicr_ & (USICS1 | USICS0);
    bool ext_mode = (clk_src == USICS_EXTERNAL || clk_src == USICS_EXTSW);
    bool has_scl = (desc_.scl_pin_address != 0);
    bool has_pins = has_scl && pm;

    for (u64 i = 0; i < elapsed_cycles; ++i) {
        bool cur_sda = false;
        bool cur_scl = false;
        if (has_pins) {
            cur_sda = pm->get_state_by_address(desc_.sda_pin_address, desc_.sda_pin_bit).drive_level;
            cur_scl = pm->get_state_by_address(desc_.scl_pin_address, desc_.scl_pin_bit).drive_level;

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

            if (ext_mode && !prev_scl_ && cur_scl) {
                shift_clock(cur_sda);
            }
        }

        // Handle pending Timer0 clock events (no SCL pin needed)
        if (clk_src == USICS_TIMER0 && timer0_clock_pending_ > 0) {
            for (; timer0_clock_pending_ > 0; timer0_clock_pending_--) {
                shift_clock(cur_sda);
            }
        }

        // Update DO pin on change
        if (pm && !is_twowire() && desc_.do_pin_address != 0) {
            bool do_val = (usidr_ & 0x80) != 0;
            if (do_val != prev_do_) {
                pm->update_pin_by_address(desc_.do_pin_address, desc_.do_pin_bit, PinOwner::spi, true, do_val);
                prev_do_ = do_val;
            }
        }

        prev_sda_ = cur_sda;
        prev_scl_ = cur_scl;
    }
}

void Usi::shift_clock(bool sda_level) noexcept {
    u8 msb = usidr_ & 0x80;

    // Data collision detection in 2-wire mode
    if (is_twowire() && msb != 0 && !sda_level) {
        usisr_ |= USIDC;
    }

    usidr_ = (usidr_ << 1) | (sda_level ? 1 : 0);
    u8 cnt = usisr_ & USICNT_MASK;
    if (cnt < 15) {
        usisr_ = (usisr_ & ~USICNT_MASK) | ((cnt + 1) & USICNT_MASK);
    }
    if ((cnt + 1) >= 16) {
        usisr_ &= ~USICNT_MASK;
        usisr_ |= USIOIF;
        update_interrupt_pending();
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

        u8 clk_src = value & (USICS1 | USICS0);
        bool sw_clock_enabled = (clk_src == USICS_EXTSW || clk_src == USICS_SWONLY);
        bool ext_clock_enabled = (clk_src == USICS_EXTERNAL || clk_src == USICS_EXTSW);

        // USICLK and USITC are write-only (read as 0)
        usicr_ = (value & (USISIE | USIOIE | USIWM1 | USIWM0 | USICS1 | USICS0)) | USICLK;

        // USICLK: software clock strobe
        if (clock_strobe && sw_clock_enabled) {
            usicr_ |= USICLK;
            bool sda_level = false;
            if (bus_ && bus_->pin_mux()) {
                sda_level = bus_->pin_mux()->get_state_by_address(desc_.sda_pin_address, desc_.sda_pin_bit).drive_level;
            }
            shift_clock(sda_level);
        } else {
            usicr_ &= ~USICLK;
        }

        // USITC: toggle SCL pin, shift data if external clock enabled
        if (toggle_clock) {
            if (desc_.scl_pin_address != 0 && bus_ && bus_->pin_mux()) {
                auto* pm = bus_->pin_mux();
                bool cur = pm->get_state_by_address(desc_.scl_pin_address, desc_.scl_pin_bit).drive_level;
                pm->update_pin_by_address(desc_.scl_pin_address, desc_.scl_pin_bit, PinOwner::timer, true, !cur);
            }
            if (ext_clock_enabled) {
                bool sda_level = false;
                if (bus_ && bus_->pin_mux()) {
                    sda_level = bus_->pin_mux()->get_state_by_address(desc_.sda_pin_address, desc_.sda_pin_bit).drive_level;
                }
                shift_clock(sda_level);
            }
        }
        break;
    }
    case 1: {
        u8 flag_clear = value & 0xF0;
        u8 cnt_set = value & USICNT_MASK;

        usisr_ = (usisr_ & ~flag_clear);
        usisr_ = (usisr_ & ~USICNT_MASK) | cnt_set;
        if (cnt_set > 0) usisr_ &= ~USISIF;
        update_interrupt_pending();
        break;
    }
    case 2:
        usidr_ = value;
        break;
    case 3:
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
