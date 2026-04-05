#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/pin_mux.hpp"

#include <array>

namespace vioavr::core {

class AnalogComparator;
class ExtInterrupt;
class Timer8;

class Adc final : public IoPeripheral {
public:
    enum class AutoTriggerSource : u8 {
        none = 0,
        free_running = 0,
        comparator = 1,
        external_interrupt_0 = 2,
        timer_compare = 3,
        timer_overflow = 4,
        timer1_compare_b = 5,
        timer1_overflow = 6,
        timer1_capture = 7
    };

    Adc(std::string_view name,
        const AdcDescriptor& descriptor,
        PinMux& pin_mux,
        u8 source_id,
        u16 conversion_cycles = 13U) noexcept;

    // Legacy constructor for backward compatibility in simple tests
    Adc(std::string_view name,
        const DeviceDescriptor& device,
        u8 source_id,
        u16 conversion_cycles = 13U) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    void bind_signal_bank(const AnalogSignalBank& signal_bank) noexcept;
    void select_auto_trigger_source(AutoTriggerSource source) noexcept;
    void connect_comparator_auto_trigger(AnalogComparator& comparator) noexcept;
    void connect_external_interrupt_0_auto_trigger(ExtInterrupt& ext_interrupt) noexcept;
    void connect_timer_compare_auto_trigger(Timer8& timer) noexcept;
    void connect_timer_overflow_auto_trigger(Timer8& timer) noexcept;
    void set_channel_voltage(u8 channel, double normalized_voltage) noexcept;
    void notify_auto_trigger(AutoTriggerSource source) noexcept;

    [[nodiscard]] constexpr u8 adcsra() const noexcept { return adcsra_; }
    [[nodiscard]] constexpr u8 admux() const noexcept { return admux_; }
    [[nodiscard]] constexpr u8 trigger_select_register() const noexcept { return trigger_select_register_; }
    [[nodiscard]] constexpr u16 result() const noexcept { return result_; }

private:
    [[nodiscard]] bool auto_trigger_enabled() const noexcept;
    [[nodiscard]] bool free_running_enabled() const noexcept;
    [[nodiscard]] AutoTriggerSource resolve_auto_trigger_source(u8 selector) const noexcept;
    void update_auto_trigger_source_from_register() noexcept;
    void start_conversion() noexcept;
    void restart_free_running_conversion() noexcept;
    void complete_conversion() noexcept;
    [[nodiscard]] u8 selected_channel() const noexcept;
    
    void update_pin_ownership() noexcept;

    std::string_view name_;
    AdcDescriptor desc_;
    PinMux* pin_mux_ {};
    std::array<AddressRange, 5> ranges_;
    u8 source_id_;
    u16 conversion_cycles_;
    const AnalogSignalBank* signal_bank_ {};
    std::array<double, 8> local_channel_voltage_ {};
    u8 adcsra_ {};
    u8 admux_ {};
    u8 adcsrb_ {};
    u8 didr0_ {};
    u8 trigger_select_register_ {};
    u16 result_ {};
    u16 cycles_remaining_ {};
    AutoTriggerSource auto_trigger_source_ {AutoTriggerSource::none};
    bool converting_ {};
    bool pending_ {};
};

}  // namespace vioavr::core
