#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/digital_threshold.hpp"
#include <array>

namespace vioavr::core {

class Adc;
class PinMux;

class ExtInterrupt final : public IoPeripheral {
public:
    ExtInterrupt(std::string_view name,
                 const ExtInterruptDescriptor& descriptor,
                 PinMux& pin_mux,
                 u8 source_id) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool on_external_pin_change(u8 bit_index, PinLevel level) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    void bind_int0_signal(const AnalogSignalBank& signal_bank, u8 channel, DigitalThresholdConfig threshold = {}) noexcept;
    void connect_adc_auto_trigger(Adc& adc) noexcept;
    void set_int0_level(bool high) noexcept;
    void set_int0_voltage(double normalized_voltage) noexcept;

private:
    void refresh_bound_input() noexcept;
    [[nodiscard]] u8 int0_sense_mode() const noexcept;
    void raise_int0() noexcept;

    std::string_view name_;
    ExtInterruptDescriptor desc_;
    PinMux* pin_mux_ {};
    std::array<AddressRange, 3> ranges_;
    u8 source_id_;

    const AnalogSignalBank* signal_bank_ {};
    u8 signal_channel_ {};
    DigitalThresholdConfig threshold_config_ {};
    Adc* auto_trigger_adc_ {};

    u8 eicra_ {};
    u8 eimsk_ {};
    u8 eifr_ {};
    bool int0_level_ {true};
    bool int1_level_ {true};
    bool int0_pending_ {};
    bool int1_pending_ {};
};

}  // namespace vioavr::core
