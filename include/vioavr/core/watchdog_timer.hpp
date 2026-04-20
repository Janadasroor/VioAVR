#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <string>
#include <array>

namespace vioavr::core {

class AvrCpu;

class WatchdogTimer final : public IoPeripheral {
public:
    explicit WatchdogTimer(std::string_view name, const WdtDescriptor& desc, AvrCpu& cpu) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    void reset_watchdog() noexcept;

private:
    void complete_timeout() noexcept;
    [[nodiscard]] u32 get_timeout_cycles() const noexcept;

    std::string name_;
    WdtDescriptor desc_;
    AvrCpu& cpu_;
    std::array<AddressRange, 1> ranges_;

    u16 wdtcsr_ {};
    u32 cycles_left_ {};
    bool timed_sequence_active_ {false};
    u8 timed_sequence_cycles_left_ {0};
    bool interrupt_pending_ {false};

};

}  // namespace vioavr::core
