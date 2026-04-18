#pragma once

#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <string>
#include <array>

namespace vioavr::core {

class AvrCpu;

class Wdt8x : public IoPeripheral {
public:
    Wdt8x(const Wdt8xDescriptor& descriptor, AvrCpu& cpu) noexcept;
    virtual ~Wdt8x() override = default;

    virtual std::string_view name() const noexcept override { return "WDT8X"; }
    virtual std::span<const AddressRange> mapped_ranges() const noexcept override;
    virtual ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

    virtual void reset() noexcept override;
    virtual void tick(u64 elapsed_cycles) noexcept override;
    virtual u8 read(u16 address) noexcept override;
    virtual void write(u16 address, u8 value) noexcept override;
    
    // External Interface
    void reset_timer() noexcept;

private:
    const Wdt8xDescriptor desc_;
    AvrCpu& cpu_;
    std::array<AddressRange, 1> ranges_{};

    u8 ctrla_{0};
    u8 status_{0};

    bool enabled_{false};
    u64 timeout_cycles_{0};
    u64 elapsed_cycles_{0};
    
    void update_timeout() noexcept;
};

} // namespace vioavr::core
