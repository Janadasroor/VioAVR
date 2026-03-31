#pragma once

#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/io_peripheral.hpp"

namespace vioavr::core {

class Adc;

class AnalogComparator final : public IoPeripheral {
public:
    AnalogComparator(std::string_view name,
                     u16 acsr_address,
                     u8 vector_index,
                     u8 source_id,
                     double hysteresis = 0.02) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    void bind_signal_bank(const AnalogSignalBank& signal_bank, u8 positive_channel, u8 negative_channel) noexcept;
    void connect_adc_auto_trigger(Adc& adc) noexcept;
    void set_positive_input_voltage(double normalized_voltage) noexcept;
    void set_negative_input_voltage(double normalized_voltage) noexcept;

    [[nodiscard]] constexpr u8 acsr() const noexcept { return acsr_; }
    [[nodiscard]] constexpr bool output_high() const noexcept { return output_high_; }

private:
    void refresh_bound_inputs() noexcept;
    void evaluate_output() noexcept;
    void raise_interrupt_flag() noexcept;
    [[nodiscard]] u8 interrupt_mode() const noexcept;

    std::string_view name_;
    AddressRange range_;
    u16 acsr_address_;
    u8 vector_index_;
    u8 source_id_;
    double hysteresis_;
    const AnalogSignalBank* signal_bank_ {};
    u8 positive_channel_ {};
    u8 negative_channel_ {};
    Adc* auto_trigger_adc_ {};
    double positive_input_ {};
    double negative_input_ {};
    u8 acsr_ {};
    bool output_high_ {};
    bool pending_ {};
};

}  // namespace vioavr::core
