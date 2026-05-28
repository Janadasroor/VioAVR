#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/digital_threshold.hpp"
#include <array>
#include <string>

namespace vioavr::core {

class Adc;
class PinMux;

class ExtInterrupt final : public IoPeripheral {
public:
    static constexpr u8 kIntCount = 8;

    ExtInterrupt(std::string_view name,
                 const ExtInterruptDescriptor& descriptor,
                 PinMux& pin_mux,
                 u8 source_id) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;
    [[nodiscard]] ClockDomain clock_domain() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] bool wants_tick() const noexcept override { return signal_bank_ != nullptr; }
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool on_external_pin_change(u16 pin_address, u8 bit_index, PinLevel level) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;
    [[nodiscard]] bool supports_interrupt_mask() const noexcept override { return true; }

    void bind_int0_signal(const AnalogSignalBank& signal_bank, u8 channel, DigitalThresholdConfig threshold = {}) noexcept;
    void connect_adc_auto_trigger(Adc& adc) noexcept;
    void set_int0_level(bool high) noexcept;
    void set_int1_level(bool high) noexcept;
    void set_int0_voltage(double normalized_voltage) noexcept;

private:
    void refresh_bound_input() noexcept;
    void update_interrupt_pending() noexcept;
    [[nodiscard]] u8 sense_mode(u8 index) const noexcept;
    void handle_level_change(u8 index, bool high) noexcept;
    void raise_interrupt(u8 index) noexcept;

    std::string name_;
    ExtInterruptDescriptor desc_;
    PinMux* pin_mux_ {};
    std::array<AddressRange, 4> ranges_;
    u8 source_id_;

    const AnalogSignalBank* signal_bank_ {};
    u8 signal_channel_ {};
    DigitalThresholdConfig threshold_config_ {};
    Adc* auto_trigger_adc_ {};

    u8 eicra_ {};
    u8 eicrb_ {};
    u8 eimsk_ {};
    u8 eifr_ {};
    std::array<bool, kIntCount> int_levels_ {};
    std::array<bool, kIntCount> int_pending_ {};
};

} // namespace vioavr::core
