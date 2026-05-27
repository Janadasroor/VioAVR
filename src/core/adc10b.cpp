#include "vioavr/core/adc10b.hpp"
#include "vioavr/core/analog_signal_bank.hpp"

namespace vioavr::core {

Adc10b::Adc10b(const Adc10bDescriptor& desc) noexcept
    : desc_(desc) {
    if (desc_.ctrla_address != 0) {
        u16 min_addr = desc_.ctrla_address;
        u16 max_addr = desc_.temp_address;
        ranges_[0] = {min_addr, max_addr};
        num_ranges_ = 1;
    }
    if (desc_.temp_address != 0 && desc_.temp_address > desc_.winht_address) {
        ranges_[0] = {desc_.ctrla_address, desc_.temp_address};
    }
}

std::span<const AddressRange> Adc10b::mapped_ranges() const noexcept {
    return {ranges_.data(), num_ranges_};
}

bool Adc10b::is_enabled() const noexcept {
    return (ctrla_ & 0x01) != 0;
}

u8 Adc10b::get_prescaler() const noexcept {
    return ctrlb_ & 0x0F;
}

void Adc10b::reset() noexcept {
    ctrla_ = 0;
    ctrlb_ = 0;
    ctrlc_ = 0;
    ctrld_ = 0;
    ctrle_ = 0;
    ctrlf_ = 0;
    intctrl_ = 0;
    intflags_ = 0;
    status_ = 0;
    dbgctrl_ = 0;
    command_ = 0;
    muxpos_ = 0;
    result_ = 0;
    sample_ = 0;
    winlt_ = 0;
    winht_ = 0;
    cycles_remaining_ = 0;
    converting_ = false;
}

void Adc10b::tick(u64) noexcept {
    if (!is_enabled() || !converting_) return;

    if (cycles_remaining_ > 0) {
        --cycles_remaining_;
        if (cycles_remaining_ == 0) {
            complete_conversion();
        }
    }
}

void Adc10b::start_conversion() noexcept {
    if (!is_enabled()) return;

    u8 prescaler = get_prescaler();
    u16 presc_div;
    static const u16 presc_table[] = {2, 4, 6, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56, 64};
    if (prescaler < 16) {
        presc_div = presc_table[prescaler];
    } else {
        presc_div = 64;
    }

    u8 sampdur = ctrle_ & 0x1F;
    if (sampdur < 2) sampdur = 2;
    u8 mode = (command_ >> 4) & 0x07;
    u8 resolution = (mode == 0x00) ? 8 : 10;
    u8 sample_count = (ctrlf_ & 0x07) + 1;

    u32 total_cycles = sampdur * presc_div + resolution * presc_div + 1;
    total_cycles *= sample_count;

    converting_ = true;
    status_ |= 0x01;
    cycles_remaining_ = total_cycles;
}

void Adc10b::complete_conversion() noexcept {
    converting_ = false;
    status_ &= ~0x01;

    u16 raw = 0;
    if (signal_bank_) {
        double vref;
        u8 refsel = ctrlc_ & 0x07;
        switch (refsel) {
            case 4: vref = 1.024; break;
            case 5: vref = 2.048; break;
            case 6: vref = 4.096; break;
            case 7: vref = 2.5; break;
            default: vref = 5.0; break;
        }
        double vin = signal_bank_->voltage(muxpos_);
        if (vin > vref) vin = vref;
        if (vin < 0.0) vin = 0.0;
        raw = static_cast<u16>((vin / vref) * 1023.0 + 0.5);
    }

    u8 mode = (command_ >> 4) & 0x07;
    u8 resolution = (mode == 0x00) ? 8 : 10;
    if (resolution == 8) {
        result_ = static_cast<u16>((raw >> 2) & 0xFF);
    } else {
        result_ = raw & 0x3FF;
    }

    sample_ = raw & 0x3FF;

    intflags_ |= 0x01;

    bool wcmp_enabled = (ctrld_ & 0x07) != 0;
    if (wcmp_enabled) {
        u8 wincm = ctrld_ & 0x07;
        bool wcmp_match = false;
        switch (wincm) {
            case 1: wcmp_match = result_ < winlt_; break;
            case 2: wcmp_match = result_ > winht_; break;
            case 3: wcmp_match = result_ >= winlt_ && result_ <= winht_; break;
            case 4: wcmp_match = result_ < winlt_ || result_ > winht_; break;
            default: break;
        }
        if (wcmp_match) {
            intflags_ |= 0x04;
        }
    }

    bool freerun = (ctrlf_ & 0x40) != 0;
    if (freerun) {
        start_conversion();
    }
}

bool Adc10b::pending_interrupt_request(InterruptRequest& request) const noexcept {
    u8 pending = intflags_ & intctrl_;
    if (pending & 0x01) {
        request.vector_index = desc_.resrdy_vector_index;
        return true;
    }
    if (pending & 0x04) {
        request.vector_index = desc_.wcmp_vector_index;
        return true;
    }
    return false;
}

bool Adc10b::consume_interrupt_request(InterruptRequest& request) noexcept {
    u8 pending = intflags_ & intctrl_;
    if (pending & 0x01) {
        request.vector_index = desc_.resrdy_vector_index;
        intflags_ &= ~0x01;
        return true;
    }
    if (pending & 0x04) {
        request.vector_index = desc_.wcmp_vector_index;
        intflags_ &= ~0x04;
        return true;
    }
    return false;
}

u8 Adc10b::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.ctrlb_address) return ctrlb_;
    if (address == desc_.ctrlc_address) return ctrlc_;
    if (address == desc_.ctrld_address) return ctrld_;
    if (address == desc_.ctrle_address) return ctrle_;
    if (address == desc_.ctrlf_address) return ctrlf_;
    if (address == desc_.intctrl_address) return intctrl_;
    if (address == desc_.intflags_address) return intflags_;
    if (address == desc_.status_address) return status_;
    if (address == desc_.dbgctrl_address) return dbgctrl_;
    if (address == desc_.command_address) return command_;
    if (address == desc_.muxpos_address) return muxpos_;
    if (address == desc_.result_address) return result_ & 0xFF;
    if (address == desc_.result_address + 1) return (result_ >> 8) & 0xFF;
    if (address == desc_.sample_address) return sample_ & 0xFF;
    if (address == desc_.sample_address + 1) return (sample_ >> 8) & 0xFF;
    if (address == desc_.winlt_address) return winlt_ & 0xFF;
    if (address == desc_.winlt_address + 1) return (winlt_ >> 8) & 0xFF;
    if (address == desc_.winht_address) return winht_ & 0xFF;
    if (address == desc_.winht_address + 1) return (winht_ >> 8) & 0xFF;
    if (address == desc_.temp_address) return 0;
    return 0;
}

void Adc10b::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value;
        if (!(value & 0x01) && converting_) {
            converting_ = false;
            cycles_remaining_ = 0;
            status_ &= ~0x01;
        }
    }
    else if (address == desc_.ctrlb_address) ctrlb_ = value & 0x0F;
    else if (address == desc_.ctrlc_address) ctrlc_ = value & 0x07;
    else if (address == desc_.ctrld_address) ctrld_ = value & 0x07;
    else if (address == desc_.ctrle_address) ctrle_ = value & 0x1F;
    else if (address == desc_.ctrlf_address) ctrlf_ = value & 0x47;
    else if (address == desc_.intctrl_address) intctrl_ = value;
    else if (address == desc_.intflags_address) intflags_ &= ~value;
    else if (address == desc_.dbgctrl_address) dbgctrl_ = value;
    else if (address == desc_.command_address) {
        u8 old_cmd = command_;
        command_ = value;
        u8 start = value & 0x07;
        if (start == 0x01) {
            start_conversion();
        }
    }
    else if (address == desc_.muxpos_address) {
        muxpos_ = value;
        u8 start = command_ & 0x07;
        if (start == 0x02) {
            start_conversion();
        }
    }
    else if (address == desc_.winlt_address) winlt_ = (winlt_ & 0xFF00) | value;
    else if (address == desc_.winlt_address + 1) winlt_ = (winlt_ & 0xFF) | (static_cast<u16>(value) << 8);
    else if (address == desc_.winht_address) winht_ = (winht_ & 0xFF00) | value;
    else if (address == desc_.winht_address + 1) winht_ = (winht_ & 0xFF) | (static_cast<u16>(value) << 8);
}

} // namespace vioavr::core
