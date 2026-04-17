#pragma once

#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/device.hpp"

#include <vector>

namespace vioavr::core {

class Adc;
class Dac;

class AnalogComparator final : public IoPeripheral {
public:
    AnalogComparator(std::string_view name,
                     const AnalogComparatorDescriptor& descriptor,
                     PinMux& pin_mux,
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
    void connect_timer_input_capture(Timer16& timer) noexcept;
    void connect_psc_fault(class Psc& psc) noexcept;
    void connect_dac(const class Dac& dac) noexcept;
    void connect_adc(const class Adc& adc) noexcept;
    void set_positive_input_voltage(double normalized_voltage) noexcept;
    void set_negative_input_voltage(double normalized_voltage) noexcept;

    [[nodiscard]] constexpr bool output_high() const noexcept { return output_high_; }
    [[nodiscard]] bool is_disabled() const noexcept;

private:
    void refresh_bound_inputs() noexcept;
    void evaluate_output() noexcept;
    void raise_interrupt_flag() noexcept;
    [[nodiscard]] u8 interrupt_mode() const noexcept;
    void update_pin_ownership() noexcept;

    std::string_view name_;
    AnalogComparatorDescriptor desc_;
    PinMux* pin_mux_ {};
    std::vector<AddressRange> mapped_ranges_;
    u8 source_id_;
    double hysteresis_;
    const AnalogSignalBank* signal_bank_ {};
    u8 positive_channel_ {};
    u8 negative_channel_ {};
    Adc* auto_trigger_adc_ {};
    const Adc* source_adc_ {};
    const Dac* dac_ {};
    Timer16* input_capture_timer_ {};
    std::vector<class Psc*> psc_fault_listeners_ {};
    double positive_input_ {};
    double negative_input_ {};
    u8 acsr_ {0x00U};
    u8 accon_ {0x00U};
    u8 didr_ {0x00U};
    bool output_high_ {false};
    bool pending_ {false};
};

}  // namespace vioavr::core
