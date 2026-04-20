#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/pin_map.hpp"

#include <array>
#include <string>

namespace vioavr::core {

class AnalogComparator;
class ExtInterrupt;
class Timer8;
class MemoryBus;

class Adc final : public IoPeripheral {
public:
    using AutoTriggerSource = AdcAutoTriggerSource;

    Adc(std::string_view name,
        const AdcDescriptor& descriptor,
        PinMux& pin_mux,
        u8 source_id,
        u16 conversion_cycles = 0U) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_bus(MemoryBus& bus) noexcept { bus_ = &bus; }

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    [[nodiscard]] ClockDomain clock_domain() const noexcept override;

    void bind_signal_bank(const AnalogSignalBank& signal_bank) noexcept;
    void bind_pin_map(const PinMap& pin_map) noexcept;
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
    [[nodiscard]] bool power_reduction_enabled() const noexcept;
    [[nodiscard]] bool auto_trigger_enabled() const noexcept;
    [[nodiscard]] bool free_running_enabled() const noexcept;
    [[nodiscard]] AutoTriggerSource resolve_auto_trigger_source(u8 selector) const noexcept;
    void update_auto_trigger_source_from_register() noexcept;
    struct MuxMapping {
        u8 positive_channel;
        u8 negative_channel;
        double gain;
        bool differential;
    };

    void start_conversion() noexcept;
    void restart_free_running_conversion() noexcept;
    void complete_conversion() noexcept;
    [[nodiscard]] MuxMapping resolve_mux() const noexcept;
    [[nodiscard]] double get_voltage(u8 channel) const noexcept;
    
    void update_pin_ownership() noexcept;

    std::string name_;
    AdcDescriptor desc_;
    MemoryBus* bus_ {};
    PinMux* pin_mux_ {};
    const PinMap* pin_map_ {};
    std::array<AddressRange, 5> ranges_;
    size_t ri_ {0};
    u8 source_id_;
    u16 conversion_cycles_;
    const AnalogSignalBank* signal_bank_ {};
    std::array<double, 16> local_channel_voltage_ {};
    u8 adcsra_ {};
    u8 admux_ {};
    u8 adcsrb_ {};
    u8 didr0_ {};
    u8 trigger_select_register_ {};
    u16 result_ {};
    u16 cycles_remaining_ {};
    bool first_conversion_ {true};
    AutoTriggerSource auto_trigger_source_ {AutoTriggerSource::none};
    bool converting_ {};
    bool pending_ {};
};

}  // namespace vioavr::core
