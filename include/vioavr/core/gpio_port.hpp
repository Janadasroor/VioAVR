#pragma once

#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/digital_threshold.hpp"
#include <array>
#include <string>

namespace vioavr::core {
class PinMux;

class GpioPort final : public IoPeripheral {
public:
    GpioPort(std::string_view name, u16 base_address, PinMux& pin_mux) noexcept;
    GpioPort(std::string_view name, u16 pin_address, u16 ddr_address, u16 port_address, PinMux& pin_mux) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;
    [[nodiscard]] u16 num_mapped_ranges() const noexcept { return num_ranges_; }

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool on_external_pin_change(u8 bit_index, PinLevel level) noexcept override;
    [[nodiscard]] bool consume_pin_change(PinStateChange& change) noexcept override;

    void set_input_levels(u8 levels) noexcept;

    [[nodiscard]] constexpr u8 ddr_register() const noexcept { return ddr_; }
    [[nodiscard]] constexpr u8 port_register() const noexcept { return port_; }
    [[nodiscard]] constexpr u8 output_levels() const noexcept { return static_cast<u8>(port_ & ddr_); }
    [[nodiscard]] constexpr u16 port_address() const noexcept { return port_addr_; }
    [[nodiscard]] constexpr u16 pin_address() const noexcept { return pin_addr_; }

    void bind_input_signal(u8 bit_index, const AnalogSignalBank& bank, u8 channel,
                           DigitalThresholdConfig threshold = {}) noexcept;
    void set_input_voltage(u8 bit_index, double normalized_voltage) noexcept;
    [[nodiscard]] u8 sample_levels() noexcept;

private:
    void apply_pin_voltage(u8 bit_index, double voltage, DigitalThresholdConfig threshold) noexcept;
    void update_pin_latched() noexcept;

    PinMux* pin_mux_ {};
    std::string name_;
    std::array<AddressRange, 3> ranges_;
    u16 pin_addr_;
    u16 ddr_addr_;
    u16 port_addr_;
    u8 num_ranges_ {0};
    u8 port_idx_ {0xFFU};

    u8 ddr_ {};
    u8 port_ {};
    u8 pin_ {};
    u8 pin_latched_ {};
    u8 external_levels_ {};
    u8 pending_changes_mask_ {};

    struct AnalogBinding {
        const AnalogSignalBank* bank {};
        u8 channel {};
        DigitalThresholdConfig threshold {};
        bool active {false};
    };
    std::array<AnalogBinding, 8> pin_bindings_ {};
    std::array<double, 8> pin_voltages_ {};
    std::array<bool, 8> pin_levels_ {};
    std::array<bool, 8> has_voltage_input_ {};
    std::array<bool, 8> has_analog_binding_ {};
};

}  // namespace vioavr::core
