#include "vioavr/core/usb.hpp"
#include <algorithm>

namespace vioavr::core {

Usb::Usb(std::string_view name, const UsbDescriptor& desc) noexcept
    : name_(name), desc_(desc) {
    size_t ri = 0;
    if (desc.uhwcon_address) ranges_[ri++] = {desc.uhwcon_address, desc.usbint_address};
    if (desc.udcon_address) ranges_[ri++] = {desc.udcon_address, desc.udaddr_address};
    if (desc.uenum_address) ranges_[ri++] = {desc.uenum_address, desc.ueconx_address};
    if (desc.ueintx_address) ranges_[ri++] = {desc.ueintx_address, desc.uesta1x_address};
    if (desc.uedatx_address) ranges_[ri++] = {desc.uedatx_address, desc.ueint_address};
    
    // Initialize FIFOs for endpoints
    for (auto& ep : endpoints_) {
        ep.fifo.resize(64); // Small default size
    }
}

std::span<const AddressRange> Usb::mapped_ranges() const noexcept {
    return {ranges_.data(), ranges_.size()};
}

void Usb::reset() noexcept {
    uhwcon_ = 0;
    usbcon_ = 0;
    usbsta_ = 0;
    usbint_ = 0;
    udcon_ = 0;
    udint_ = 0;
    udien_ = 0;
    udaddr_ = 0;
    uenum_ = 0;
    uerst_ = 0;
    ueint_ = 0;
    for (auto& ep : endpoints_) {
        ep.control = 0;
        ep.config0 = 0;
        ep.config1 = 0;
        ep.status0 = 0;
        ep.status1 = 0;
        ep.interrupt_flags = 0;
        ep.interrupt_enable = 0;
        ep.read_idx = 0;
        ep.write_idx = 0;
        ep.byte_count = 0;
    }
}

void Usb::tick(u64) noexcept {
    // Basic USB logic (e.g. state transitions) would go here
}

u8 Usb::read(u16 address) noexcept {
    if (address == desc_.uhwcon_address) return uhwcon_;
    if (address == desc_.usbcon_address) return usbcon_;
    if (address == desc_.usbsta_address) return usbsta_;
    if (address == desc_.usbint_address) return usbint_;
    if (address == desc_.udcon_address) return udcon_;
    if (address == desc_.udint_address) return udint_;
    if (address == desc_.udien_address) return udien_;
    if (address == desc_.udaddr_address) return udaddr_;
    if (address == desc_.uenum_address) return uenum_;
    if (address == desc_.uerst_address) return uerst_;
    if (address == desc_.ueint_address) return ueint_;

    u8 ep_idx = uenum_ & 0x07U;
    if (ep_idx >= endpoints_.size()) return 0;
    auto& ep = endpoints_[ep_idx];

    if (address == desc_.ueconx_address) return ep.control;
    if (address == desc_.uecfg0x_address) return ep.config0;
    if (address == desc_.uecfg1x_address) return ep.config1;
    if (address == desc_.uesta0x_address) return ep.status0;
    if (address == desc_.uesta1x_address) return ep.status1;
    if (address == desc_.ueintx_address) return ep.interrupt_flags;
    if (address == desc_.ueienx_address) return ep.interrupt_enable;
    if (address == desc_.uebclx_address) return static_cast<u8>(ep.byte_count & 0xFFU);
    if (address == desc_.uebchx_address) return static_cast<u8>((ep.byte_count >> 8U) & 0xFFU);
    
    if (address == desc_.uedatx_address) {
        if (ep.byte_count > 0) {
            u8 data = ep.fifo[ep.read_idx];
            ep.read_idx = (ep.read_idx + 1) % ep.fifo.size();
            ep.byte_count--;
            return data;
        }
        return 0;
    }

    return 0;
}

void Usb::write(u16 address, u8 value) noexcept {
    if (address == desc_.uhwcon_address) uhwcon_ = value;
    else if (address == desc_.usbcon_address) usbcon_ = value;
    else if (address == desc_.usbsta_address) usbsta_ = value;
    else if (address == desc_.usbint_address) usbint_ = value;
    else if (address == desc_.udcon_address) udcon_ = value;
    else if (address == desc_.udint_address) udint_ = value;
    else if (address == desc_.udien_address) udien_ = value;
    else if (address == desc_.udaddr_address) udaddr_ = value;
    else if (address == desc_.uenum_address) uenum_ = value;
    else if (address == desc_.uerst_address) uerst_ = value;
    
    u8 ep_idx = uenum_ & 0x07U;
    if (ep_idx >= endpoints_.size()) return;
    auto& ep = endpoints_[ep_idx];

    if (address == desc_.ueconx_address) ep.control = value;
    else if (address == desc_.uecfg0x_address) ep.config0 = value;
    else if (address == desc_.uecfg1x_address) ep.config1 = value;
    else if (address == desc_.ueintx_address) ep.interrupt_flags = value;
    else if (address == desc_.ueienx_address) ep.interrupt_enable = value;
    else if (address == desc_.uedatx_address) {
        if (ep.byte_count < ep.fifo.size()) {
            ep.fifo[ep.write_idx] = value;
            ep.write_idx = (ep.write_idx + 1) % ep.fifo.size();
            ep.byte_count++;
        }
    }
}

bool Usb::pending_interrupt_request(InterruptRequest&) const noexcept {
    return false; // To be implemented
}

bool Usb::consume_interrupt_request(InterruptRequest&) noexcept {
    return false; // To be implemented
}

} // namespace vioavr::core
