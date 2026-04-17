#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <array>
#include <vector>

namespace vioavr::core {

class Usb final : public IoPeripheral {
public:
    Usb(std::string_view name, const UsbDescriptor& desc) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    [[nodiscard]] std::string_view name() const noexcept override { return name_; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;
    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

    void force_general_interrupt(u8 mask) noexcept { udint_ |= mask; }
    void force_endpoint_interrupt(u8 ep_idx, u8 mask) noexcept {
        if (ep_idx < endpoints_.size()) {
            endpoints_[ep_idx].interrupt_flags |= mask;
            update_ueint();
        }
    }

    struct SetupPacket {
        u8 bmRequestType;
        u8 bRequest;
        u16 wValue;
        u16 wIndex;
        u16 wLength;
    };

    void simulate_usb_reset() noexcept;
    void simulate_vbus_event(bool high) noexcept;
    void simulate_setup_packet(const SetupPacket& setup) noexcept;
    void simulate_out_packet(u8 ep_idx, std::span<const u8> data) noexcept;
    void simulate_in_token(u8 ep_idx) noexcept;

    [[nodiscard]] std::vector<u8> get_endpoint_data(u8 ep_idx) const noexcept;

private:
    void update_ueint() noexcept;

    struct Endpoint {
        u8 control {};
        u8 config0 {};
        u8 config1 {};
        u8 status0 {};
        u8 status1 {};
        u8 interrupt_flags {};
        u8 interrupt_enable {};
        std::vector<u8> fifo;
        size_t read_idx {0};
        size_t write_idx {0};
        u16 byte_count {0};
    };

    std::string_view name_;
    UsbDescriptor desc_;
    MemoryBus* bus_ {};
    std::array<AddressRange, 5> ranges_;

    u64 cycle_accumulator_ {0};
    u32 frame_cycles_ {0}; // 1ms worth of cycles for SOF
    u16 frame_number_ {0};

    u8 uhwcon_ {};
    u8 usbcon_ {};
    u8 usbsta_ {};
    u8 usbint_ {};
    u8 udcon_ {};
    u8 udint_ {};
    u8 udien_ {};
    u8 udaddr_ {};
    u8 uenum_ {};
    u8 uerst_ {};
    u8 ueint_ {};
    
    std::array<Endpoint, 7> endpoints_;
};

} // namespace vioavr::core
