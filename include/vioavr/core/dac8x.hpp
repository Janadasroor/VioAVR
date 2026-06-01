#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <array>

namespace vioavr::core {

class MemoryBus;
class AnalogSignalBank;

class Dac8x final : public IoPeripheral {
public:
    explicit Dac8x(const Dac8xDescriptor& desc, u8 instance_index) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_analog_signal_bank(AnalogSignalBank* bank) noexcept { signal_bank_ = bank; }
    void set_vref(double v) noexcept { vref_ = v; }

    [[nodiscard]] std::string_view name() const noexcept override { return "DAC8x"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] bool wants_tick() const noexcept override { return true; }

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

    [[nodiscard]] double output_voltage() const noexcept {
        return static_cast<double>(data_) * vref_ / 255.0;
    }

private:
    void start_conversion();
    void complete_conversion();

    Dac8xDescriptor desc_;
    std::array<AddressRange, 1> ranges_{};
    MemoryBus* bus_{};
    AnalogSignalBank* signal_bank_{};
    double vref_{5.0};
    u8 instance_index_{};

    u8 ctrla_{0};
    u8 data_{0};
    u16 cycles_remaining_{0};
    bool converting_{false};
};

} // namespace vioavr::core
