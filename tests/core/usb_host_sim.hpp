#pragma once

#include "vioavr/core/usb.hpp"
#include <vector>
#include <optional>

namespace vioavr::core::testing {

class UsbHostSim {
public:
    explicit UsbHostSim(Usb& usb) : usb_(usb) {}

    void reset_device() {
        usb_.simulate_usb_reset();
    }

    void set_vbus(bool high) {
        usb_.simulate_vbus_event(high);
    }

    void send_setup(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength) {
        Usb::SetupPacket setup { bmRequestType, bRequest, wValue, wIndex, wLength };
        usb_.simulate_setup_packet(setup);
    }

    std::vector<u8> receive_in_packet(u8 ep_idx) {
        auto data = usb_.get_endpoint_data(ep_idx);
        usb_.simulate_in_token(ep_idx);
        return data;
    }

    void send_out_packet(u8 ep_idx, std::span<const u8> data) {
        usb_.simulate_out_packet(ep_idx, data);
    }

private:
    Usb& usb_;
};

} // namespace vioavr::core::testing
