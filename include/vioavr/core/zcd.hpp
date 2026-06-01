#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <array>

namespace vioavr::core {

class MemoryBus;
class AnalogSignalBank;

class Zcd final : public IoPeripheral {
public:
    explicit Zcd(const ZcdDescriptor& desc, u8 instance_index = 0) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_analog_signal_bank(AnalogSignalBank* bank) noexcept { signal_bank_ = bank; }
    void set_input_channel(u8 ch) noexcept { input_channel_ = ch; }
    void set_vdd(double v) noexcept { vdd_ = v; }

    [[nodiscard]] std::string_view name() const noexcept override { return "ZCD"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] bool wants_tick() const noexcept override { return true; }

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }
    [[nodiscard]] bool on_external_pin_change(u16 pin_address, u8 bit_index, PinLevel level) noexcept override;

private:
    void sample_analog();
    void set_state(bool high) noexcept;

    ZcdDescriptor desc_;
    std::array<AddressRange, 1> ranges_{};
    MemoryBus* bus_{};
    AnalogSignalBank* signal_bank_{};
    u8 instance_index_{};
    u8 input_channel_{0};
    double vdd_{5.0};

    u8 ctrla_{0};
    u8 intctrl_{0};
    u8 status_{0};
    bool int_pending_{false};
    bool sampled_{false};
};

} // namespace vioavr::core
