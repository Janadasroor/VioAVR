#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <array>
#include <string>

namespace vioavr::core {

class MemoryBus;
class AnalogSignalBank;

class XmegaAdc final : public IoPeripheral {
public:
    explicit XmegaAdc(const XmegaAdcDescriptor& desc) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
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
    enum class AdcPhase { idle, converting };

    [[nodiscard]] bool is_enabled() const noexcept { return ctrla_ & 0x01U; }
    [[nodiscard]] bool is_free_running() const noexcept { return ctrla_ & 0x02U; }
    [[nodiscard]] bool is_12bit() const noexcept;
    [[nodiscard]] u32 conversion_cycles() const noexcept;
    [[nodiscard]] u16 adc_cycle_divider() const noexcept;

    void start_conversion() noexcept;
    void complete_conversion() noexcept;
    void update_interrupt_state() noexcept;

    XmegaAdcDescriptor desc_;
    MemoryBus* bus_ {};
    AnalogSignalBank* signal_bank_ {};
    std::array<AddressRange, 6> ranges_;
    size_t num_ranges_ {};

    u8 ctrla_ {};
    u8 ctrlb_ {};
    u8 refctrl_ {};
    u8 evctrl_ {};
    u8 intflags_ {};
    u8 prescaler_ {};
    u8 temp_ {};
    u8 sampctrl_ {};
    u16 ch0res_ {};
    u16 cmp_ {};
    u16 ch0res_latch_ {};
    bool ch0res_latch_valid_ {false};

    AdcPhase state_ {AdcPhase::idle};
    u64 fractional_cycles_ {0};
    u32 cycles_remaining_ {0};
};

} // namespace vioavr::core
