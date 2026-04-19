#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <array>
#include <string>

namespace vioavr::core {

class MemoryBus;
class EventSystem;
class AnalogSignalBank;

class Adc8x final : public IoPeripheral {
public:
    explicit Adc8x(const Adc8xDescriptor& desc) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_event_system(EventSystem* evsys) noexcept override;
    void set_analog_signal_bank(AnalogSignalBank* bank) noexcept { signal_bank_ = bank; }

    [[nodiscard]] std::string_view name() const noexcept override { return "ADC"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override { return {ranges_.data(), num_ranges_}; }

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

private:
    enum class AdcPhase {
        idle,
        startup,
        sample,
        convert
    };

    [[nodiscard]] bool is_enabled() const noexcept;
    [[nodiscard]] u16 get_prescaler() const noexcept;
    [[nodiscard]] u32 get_sample_count() const noexcept;
    
    void start_conversion() noexcept;
    void complete_conversion() noexcept;
    void process_sample_result(u16 result) noexcept;

    Adc8xDescriptor desc_;
    MemoryBus* bus_ {};
    EventSystem* evsys_ {};
    AnalogSignalBank* signal_bank_ {};
    std::array<AddressRange, 8> ranges_;
    size_t num_ranges_ {};

    u8 ctrla_ {};
    u8 ctrlb_ {};
    u8 ctrlc_ {};
    u8 ctrld_ {};
    u8 ctrle_ {};
    u8 sampctrl_ {};
    u8 muxpos_ {};
    u8 muxneg_ {};
    u8 command_ {};
    u8 evctrl_ {};
    u8 intctrl_ {};
    u8 intflags_ {};
    u8 dbgctrl_ {};
    u16 res_ {};
    u16 winlt_ {};
    u16 winht_ {};

    AdcPhase state_ {AdcPhase::idle};
    u64 fractional_cycles_ {0};
    u32 cycles_in_phase_ {0};
    u32 current_sample_ {0};
    u32 accumulated_res_ {0};
    bool converting_ {false};
};

} // namespace vioavr::core
