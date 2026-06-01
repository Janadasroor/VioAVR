#include "vioavr/core/xmega_dac.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/analog_signal_bank.hpp"

namespace vioavr::core {

XmegaDac::XmegaDac(std::string_view name, const XmegaDacDescriptor& desc) noexcept
    : name_(name), desc_(desc)
{
    u8 r = 0;
    if (desc_.ctrla_address) {
        u16 e = desc_.ctrla_address;
        if (desc_.ctrlb_address) e = desc_.ctrlb_address;
        if (desc_.ctrlc_address) e = desc_.ctrlc_address;
        if (desc_.evctrl_address) e = desc_.evctrl_address;
        if (desc_.status_address) e = desc_.status_address;
        ranges_[r++] = {desc_.ctrla_address, e};
    }
    if (desc_.ch0gaincal_address && desc_.ch1offsetcal_address)
        ranges_[r++] = {desc_.ch0gaincal_address, desc_.ch1offsetcal_address};
    if (desc_.ch0data_address)
        ranges_[r++] = {desc_.ch0data_address, static_cast<u16>(desc_.ch0data_address + 1)};
    if (desc_.ch1data_address)
        ranges_[r++] = {desc_.ch1data_address, static_cast<u16>(desc_.ch1data_address + 1)};
    num_ranges_ = r;
}

std::span<const AddressRange> XmegaDac::mapped_ranges() const noexcept {
    return {ranges_.data(), num_ranges_};
}

void XmegaDac::reset() noexcept {
    ctrla_ = 0;
    ctrlb_ = 0;
    ctrlc_ = 0;
    evctrl_ = 0;
    status_ = 0;
    ch0gaincal_ = 0;
    ch0offsetcal_ = 0;
    ch1gaincal_ = 0;
    ch1offsetcal_ = 0;
    ch0data_ = 0;
    ch1data_ = 0;
    cycles_remaining_ = 0;
    converting_ = false;
    converting_channel_ = 0;
}

u16 XmegaDac::get_channel_data(u8 channel) const noexcept {
    u16 raw = channel == 0 ? ch0data_ : ch1data_;
    if (ctrlc_ & 0x01) return raw >> 4;
    return raw & 0x0FFF;
}

double XmegaDac::compute_voltage(u16 data) const noexcept {
    return static_cast<double>(data) * vref_ / 4096.0;
}

void XmegaDac::start_conversion(u8 channel) {
    if (channel == 0 && !(ctrla_ & 0x01)) return;
    if (channel == 1 && !(ctrla_ & 0x02)) return;
    if (converting_) return;
    converting_ = true;
    converting_channel_ = channel;
    cycles_remaining_ = 2;
}

void XmegaDac::complete_conversion(u8 channel) {
    u16 data = get_channel_data(channel);
    double vout = compute_voltage(data);
    if (signal_bank_) {
        u8 bank_ch = channel == 0 ? 0U : 1U;
        signal_bank_->set_voltage(bank_ch, vout);
    }
    if (channel == 0) status_ |= 0x01;
    else status_ |= 0x02;
}

void XmegaDac::tick(u64 elapsed_cycles) noexcept {
    if (!converting_) return;
    for (u64 i = 0; i < elapsed_cycles && cycles_remaining_ > 0; ++i) {
        --cycles_remaining_;
        if (cycles_remaining_ == 0) {
            complete_conversion(converting_channel_);
            converting_ = false;
        }
    }
}

u8 XmegaDac::read(u16 address) noexcept {
    if (address == desc_.ctrla_address)      return ctrla_;
    if (address == desc_.ctrlb_address)      return ctrlb_;
    if (address == desc_.ctrlc_address)      return ctrlc_;
    if (address == desc_.evctrl_address)     return evctrl_;
    if (address == desc_.status_address)     return status_;
    if (address == desc_.ch0gaincal_address) return ch0gaincal_;
    if (address == desc_.ch0offsetcal_address) return ch0offsetcal_;
    if (address == desc_.ch1gaincal_address) return ch1gaincal_;
    if (address == desc_.ch1offsetcal_address) return ch1offsetcal_;
    if (address == desc_.ch0data_address)    return static_cast<u8>(ch0data_);
    if (address == desc_.ch0data_address + 1) return static_cast<u8>(ch0data_ >> 8);
    if (address == desc_.ch1data_address)    return static_cast<u8>(ch1data_);
    if (address == desc_.ch1data_address + 1) return static_cast<u8>(ch1data_ >> 8);
    return 0;
}

void XmegaDac::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value;
    }
    else if (address == desc_.ctrlb_address) {
        ctrlb_ = value;
    }
    else if (address == desc_.ctrlc_address) {
        ctrlc_ = value;
    }
    else if (address == desc_.evctrl_address) {
        evctrl_ = value;
    }
    else if (address == desc_.status_address) {
        status_ &= ~(value & 0x03);
    }
    else if (address == desc_.ch0gaincal_address) ch0gaincal_ = value;
    else if (address == desc_.ch0offsetcal_address) ch0offsetcal_ = value;
    else if (address == desc_.ch1gaincal_address) ch1gaincal_ = value;
    else if (address == desc_.ch1offsetcal_address) ch1offsetcal_ = value;
    else if (address == desc_.ch0data_address) {
        ch0data_ = (ch0data_ & 0xFF00) | value;
    }
    else if (address == desc_.ch0data_address + 1) {
        ch0data_ = (ch0data_ & 0x00FF) | (static_cast<u16>(value) << 8);
        start_conversion(0);
    }
    else if (address == desc_.ch1data_address) {
        ch1data_ = (ch1data_ & 0xFF00) | value;
    }
    else if (address == desc_.ch1data_address + 1) {
        ch1data_ = (ch1data_ & 0x00FF) | (static_cast<u16>(value) << 8);
        start_conversion(1);
    }
}

bool XmegaDac::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (desc_.vector_index && (status_ & 0x03)) {
        request.vector_index = desc_.vector_index;
        return true;
    }
    return false;
}

bool XmegaDac::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (!desc_.vector_index || !(status_ & 0x03)) return false;
    request.vector_index = desc_.vector_index;
    status_ &= ~0x03;
    return true;
}

} // namespace vioavr::core
