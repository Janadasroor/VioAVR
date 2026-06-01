#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <string>
#include <vector>

namespace vioavr::core {

class MemoryBus;
class AnalogSignalBank;

/**
 * @brief Represents an Analog-to-Digital Converter peripheral.
 */
class Dac final : public IoPeripheral {
public:
    explicit Dac(std::string_view name, const DacDescriptor& desc);

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_analog_signal_bank(AnalogSignalBank* bank) noexcept { signal_bank_ = bank; }
    void set_vref(double vref_voltage) noexcept { vref_ = vref_voltage; }
    
    [[nodiscard]] std::string_view name() const noexcept override { return name_; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

    /**
     * @brief Triggered by an external source if DAATE is set.
     */
    void notify_auto_trigger(AdcAutoTriggerSource source) noexcept;

    /**
     * @brief Get current output voltage (normalized 0.0 to 1.0).
     */
    [[nodiscard]] double voltage() const noexcept { return voltage_; }

private:
    void update_voltage() noexcept;
    [[nodiscard]] bool power_reduction_enabled() const noexcept;

    std::string name_;
    DacDescriptor desc_;
    MemoryBus* bus_ {nullptr};
    AnalogSignalBank* signal_bank_ {nullptr};
    double vref_ {5.0};
    std::vector<AddressRange> ranges_;

    u8 dacon_ {0};
    u16 data_ {0};
    u16 buffer_value_ {0};
    double voltage_ {0.0};
};

} // namespace vioavr::core
