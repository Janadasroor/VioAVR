#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <array>
#include <string>

namespace vioavr::core {

class MemoryBus;
class AnalogSignalBank;

class Opamp final : public IoPeripheral {
public:
    explicit Opamp(const OpampDescriptor& desc, u8 instance_index) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_analog_signal_bank(AnalogSignalBank* bank) noexcept { signal_bank_ = bank; }
    void set_vdd(double v) noexcept { vdd_ = v; }

    [[nodiscard]] std::string_view name() const noexcept override { return name_; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] bool wants_tick() const noexcept override { return true; }

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] double output_voltage() const noexcept { return vout_; }
    [[nodiscard]] bool is_enabled() const noexcept { return (ctrla_ & 0x01) != 0; }

    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

private:
    void evaluate();

    std::string name_;
    OpampDescriptor desc_;
    std::array<AddressRange, 1> ranges_{};
    MemoryBus* bus_{};
    AnalogSignalBank* signal_bank_{};
    u8 instance_index_{};
    double vdd_{5.0};
    double vout_{0.0};

    u8 ctrla_{0};
    u8 ctrlb_{0};
    u8 resctrl_{0};
    u8 muxctrl_{0};
};

} // namespace vioavr::core
