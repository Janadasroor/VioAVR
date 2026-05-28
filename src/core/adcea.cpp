#include "vioavr/core/adcea.hpp"
#include "vioavr/core/analog_signal_bank.hpp"

namespace vioavr::core {

AdcEa::AdcEa(const AdcEaDescriptor& desc) noexcept
    : desc_(desc) {
    if (desc_.ctrla_address != 0) {
        u16 min_addr = desc_.ctrla_address;
        u16 max_addr = desc_.winht_address + 1;
        ranges_[0] = {min_addr, max_addr};
        num_ranges_ = 1;
    }
}

std::span<const AddressRange> AdcEa::mapped_ranges() const noexcept {
    return {ranges_.data(), num_ranges_};
}

bool AdcEa::is_enabled() const noexcept {
    return (ctrla_ & 0x01) != 0;
}

u8 AdcEa::get_prescaler() const noexcept {
    return ctrlb_ & 0x0F;
}

u8 AdcEa::get_resolution() const noexcept {
    u8 mode = (command_ >> 4) & 0x07;
    switch (mode) {
        case 0x00: return 8;
        case 0x01: return 10;
        case 0x02: return 12;
        default: return 12;
    }
}

u8 AdcEa::get_pga_gain() const noexcept {
    u8 gain_bits = (pgactrl_ >> 5) & 0x07;
    switch (gain_bits) {
        case 0: return 1;
        case 1: return 2;
        case 2: return 4;
        case 3: return 8;
        case 4: return 16;
        default: return 1;
    }
}

void AdcEa::reset() noexcept {
    ctrla_ = 0;
    ctrlb_ = 0;
    ctrlc_ = 0;
    ctrld_ = 0;
    intctrl_ = 0;
    intflags_ = 0;
    status_ = 0;
    dbgctrl_ = 0;
    ctrle_ = 0;
    ctrlf_ = 0;
    command_ = 0;
    pgactrl_ = 0;
    muxpos_ = 0;
    muxneg_ = 0;
    result_ = 0;
    sample_ = 0;
    temp0_ = 0;
    temp1_ = 0;
    temp2_ = 0;
    winlt_ = 0;
    winht_ = 0;
    cycles_remaining_ = 0;
    converting_ = false;
}

void AdcEa::tick(u64 elapsed_cycles) noexcept {
    if (!is_enabled() || !converting_) return;

    if (cycles_remaining_ > 0) {
        if (elapsed_cycles >= cycles_remaining_) {
            cycles_remaining_ = 0;
            complete_conversion();
        } else {
            cycles_remaining_ -= static_cast<u32>(elapsed_cycles);
        }
    }
}

void AdcEa::start_conversion() noexcept {
    if (!is_enabled()) return;

    u8 prescaler = get_prescaler();
    static const u16 presc_table[] = {2, 4, 6, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56, 64};
    u16 presc_div = (prescaler < 16) ? presc_table[prescaler] : 64;

    u8 sampdur = ctrle_;
    if (sampdur < 2) sampdur = 2;

    u8 resolution = get_resolution();
    u8 sample_count = (ctrlf_ & 0x0F) + 1;

    u32 total_cycles = (sampdur * sample_count + resolution + 1) * presc_div;

    converting_ = true;
    status_ |= 0x01;
    cycles_remaining_ = total_cycles;
}

void AdcEa::complete_conversion() noexcept {
    converting_ = false;
    status_ &= ~0x01;

    u32 raw = 0;
    u8 resolution = get_resolution();
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

        u8 gain = get_pga_gain();
        bool diff = (command_ & 0x80) != 0;

        double vin;
        if (diff) {
            double vpos = signal_bank_->voltage(muxpos_ & 0x3F);
            double vneg = signal_bank_->voltage(muxneg_ & 0x3F);
            vin = (vpos - vneg) * gain;
            if (vin > vref) vin = vref;
            if (vin < -vref) vin = -vref;
            raw = static_cast<u32>(((vin / vref) * 2047.0) + 2048.0);
            if (raw > 4095) raw = 4095;
            u16 max_val = (1 << resolution) - 1;
            if (raw > max_val) raw = max_val;
        } else {
            vin = signal_bank_->voltage(muxpos_ & 0x3F);
            vin *= gain;
            if (vin > vref) vin = vref;
            if (vin < 0.0) vin = 0.0;
            u16 max_val = (1 << resolution) - 1;
            raw = static_cast<u32>((vin / vref) * max_val + 0.5);
            if (raw > max_val) raw = max_val;
        }
    }

    bool leftadj = (ctrlf_ & 0x10) != 0;
    if (leftadj) {
        result_ = raw << (16 - resolution);
    } else {
        result_ = raw;
    }

    sample_ = static_cast<u16>(raw & 0xFFFF);

    if ((ctrla_ & 0x20) != 0) {
        int temp_val = static_cast<int>(temperature_ * tos_slope_ + tos_offset_ + 0.5);
        temp0_ = static_cast<u8>(temp_val & 0xFF);
        temp1_ = static_cast<u8>((temp_val >> 8) & 0x0F);
        temp2_ = static_cast<u8>(static_cast<int>((temperature_ - 27.0) * 256.0 + 0.5) & 0xFF);
    }

    intflags_ |= 0x01;

    bool wcmp_enabled = (ctrld_ & 0x07) != 0;
    if (wcmp_enabled) {
        u8 wincm = ctrld_ & 0x07;
        bool winsrc = (ctrld_ & 0x08) != 0;
        u16 cmp_val = winsrc ? sample_ : static_cast<u16>(result_ & 0xFFFF);
        bool wcmp_match = false;
        switch (wincm) {
            case 1: wcmp_match = cmp_val < winlt_; break;
            case 2: wcmp_match = cmp_val > winht_; break;
            case 3: wcmp_match = cmp_val >= winlt_ && cmp_val <= winht_; break;
            case 4: wcmp_match = cmp_val < winlt_ || cmp_val > winht_; break;
            default: break;
        }
        if (wcmp_match) {
            intflags_ |= 0x04;
        }
    }

    bool freerun = (ctrlf_ & 0x20) != 0;
    if (freerun) {
        start_conversion();
    }
}

bool AdcEa::pending_interrupt_request(InterruptRequest& request) const noexcept {
    u8 pending = intflags_ & intctrl_;
    if (pending & 0x01) {
        request.vector_index = desc_.resrdy_vector_index;
        request.source_id = 0;
        return true;
    }
    if (pending & 0x02) {
        request.vector_index = desc_.samprdy_vector_index;
        request.source_id = 0;
        return true;
    }
    if (pending & 0x20) {
        request.vector_index = desc_.error_vector_index;
        request.source_id = 0;
        return true;
    }
    if (pending & (0x04 | 0x08 | 0x10)) {
        request.vector_index = desc_.resrdy_vector_index;
        request.source_id = 0;
        return true;
    }
    return false;
}

bool AdcEa::consume_interrupt_request(InterruptRequest& request) noexcept {
    u8 pending = intflags_ & intctrl_;
    if (pending & 0x01) {
        request.vector_index = desc_.resrdy_vector_index;
        intflags_ &= ~0x01;
        request.source_id = 0;
        return true;
    }
    if (pending & 0x02) {
        request.vector_index = desc_.samprdy_vector_index;
        intflags_ &= ~0x02;
        request.source_id = 0;
        return true;
    }
    if (pending & 0x20) {
        request.vector_index = desc_.error_vector_index;
        intflags_ &= ~0x20;
        request.source_id = 0;
        return true;
    }
    if (pending & (0x04 | 0x08 | 0x10)) {
        request.vector_index = desc_.resrdy_vector_index;
        intflags_ &= ~pending & (0x04 | 0x08 | 0x10);
        request.source_id = 0;
        return true;
    }
    return false;
}

u8 AdcEa::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.ctrlb_address) return ctrlb_;
    if (address == desc_.ctrlc_address) return ctrlc_;
    if (address == desc_.ctrld_address) return ctrld_;
    if (address == desc_.intctrl_address) return intctrl_;
    if (address == desc_.intflags_address) return intflags_;
    if (address == desc_.status_address) return status_;
    if (address == desc_.dbgctrl_address) return dbgctrl_;
    if (address == desc_.ctrle_address) return ctrle_;
    if (address == desc_.ctrlf_address) return ctrlf_;
    if (address == desc_.command_address) return command_;
    if (address == desc_.pgactrl_address) return pgactrl_;
    if (address == desc_.muxpos_address) return muxpos_;
    if (address == desc_.muxneg_address) return muxneg_;
    if (address == desc_.result_address) return result_ & 0xFF;
    if (address == desc_.result_address + 1) return (result_ >> 8) & 0xFF;
    if (address == desc_.result_address + 2) return (result_ >> 16) & 0xFF;
    if (address == desc_.result_address + 3) return (result_ >> 24) & 0xFF;
    if (address == desc_.sample_address) return sample_ & 0xFF;
    if (address == desc_.sample_address + 1) return (sample_ >> 8) & 0xFF;
    if (address == desc_.temp0_address) return temp0_;
    if (address == desc_.temp1_address) return temp1_;
    if (address == desc_.temp2_address) return temp2_;
    if (address == desc_.winlt_address) return winlt_ & 0xFF;
    if (address == desc_.winlt_address + 1) return (winlt_ >> 8) & 0xFF;
    if (address == desc_.winht_address) return winht_ & 0xFF;
    if (address == desc_.winht_address + 1) return (winht_ >> 8) & 0xFF;
    return 0;
}

void AdcEa::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value & 0xA1;
        if (!(value & 0x01) && converting_) {
            converting_ = false;
            cycles_remaining_ = 0;
            status_ &= ~0x01;
        }
    }
    else if (address == desc_.ctrlb_address) ctrlb_ = value & 0x0F;
    else if (address == desc_.ctrlc_address) ctrlc_ = value & 0x07;
    else if (address == desc_.ctrld_address) ctrld_ = value & 0x0F;
    else if (address == desc_.intctrl_address) intctrl_ = value;
    else if (address == desc_.intflags_address) intflags_ &= ~value;
    else if (address == desc_.dbgctrl_address) dbgctrl_ = value & 0x01;
    else if (address == desc_.ctrle_address) ctrle_ = value;
    else if (address == desc_.ctrlf_address) ctrlf_ = value & 0x7F;
    else if (address == desc_.pgactrl_address) pgactrl_ = value;
    else if (address == desc_.muxpos_address) {
        muxpos_ = value;
        u8 start = command_ & 0x07;
        if (start == 0x02) {
            start_conversion();
        }
    }
    else if (address == desc_.muxneg_address) {
        muxneg_ = value;
    }
    else if (address == desc_.command_address) {
        u8 old_cmd = command_;
        command_ = value;
        u8 start = value & 0x07;
        if (start == 0x01) {
            start_conversion();
        }
    }
    else if (address == desc_.winlt_address) winlt_ = (winlt_ & 0xFF00) | value;
    else if (address == desc_.winlt_address + 1) winlt_ = (winlt_ & 0xFF) | (static_cast<u16>(value) << 8);
    else if (address == desc_.winht_address) winht_ = (winht_ & 0xFF00) | value;
    else if (address == desc_.winht_address + 1) winht_ = (winht_ & 0xFF) | (static_cast<u16>(value) << 8);
}

} // namespace vioavr::core
