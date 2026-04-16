#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <array>
#include <vector>

namespace vioavr::core {

class Usb final : public IoPeripheral {
public:
    Usb(std::string_view name, const UsbDescriptor& desc) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override { return name_; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;
    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

private:
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
    std::array<AddressRange, 5> ranges_;

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
