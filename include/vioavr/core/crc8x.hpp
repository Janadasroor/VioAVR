#pragma once

#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <string>
#include <array>
#include <vector>
#include <span>

namespace vioavr::core {

class Crc8x : public IoPeripheral {
public:
    Crc8x(const Crc8xDescriptor& descriptor, std::span<const u16> flash) noexcept;
    virtual ~Crc8x() override = default;

    virtual std::string_view name() const noexcept override { return "CRC8X"; }
    virtual std::span<const AddressRange> mapped_ranges() const noexcept override;
    virtual ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

    virtual void reset() noexcept override;
    virtual void tick(u64 elapsed_cycles) noexcept override;
    virtual u8 read(u16 address) noexcept override;
    virtual void write(u16 address, u8 value) noexcept override;
    
    void set_event_system(class EventSystem* evsys) noexcept;

private:
    const Crc8xDescriptor desc_;
    class EventSystem* evsys_ {nullptr};
    std::span<const u16> flash_;
    std::array<AddressRange, 1> ranges_{};

    u8 ctrla_{0};
    u8 status_{0};
    u16 checksum_{0};
    
    bool busy_{false};
    u64 remaining_cycles_{0};
    
    void start_scan() noexcept;
    u16 calculate_crc16(std::span<const u16> data) noexcept;
};

} // namespace vioavr::core
