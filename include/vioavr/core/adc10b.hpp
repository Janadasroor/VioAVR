#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <array>
#include <string>

namespace vioavr::core {

class MemoryBus;
class AnalogSignalBank;

class Adc10b final : public IoPeripheral {
public:
    explicit Adc10b(const Adc10bDescriptor& desc) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_analog_signal_bank(AnalogSignalBank* bank) noexcept { signal_bank_ = bank; }

    [[nodiscard]] std::string_view name() const noexcept override { return "ADC"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

private:
    [[nodiscard]] bool is_enabled() const noexcept;
    [[nodiscard]] u8 get_prescaler() const noexcept;

    void start_conversion() noexcept;
    void complete_conversion() noexcept;

    Adc10bDescriptor desc_;
    MemoryBus* bus_ {};
    AnalogSignalBank* signal_bank_ {};
    std::array<AddressRange, 8> ranges_;
    size_t num_ranges_ {};

    u8 ctrla_ {};
    u8 ctrlb_ {};
    u8 ctrlc_ {};
    u8 ctrld_ {};
    u8 ctrle_ {};
    u8 ctrlf_ {};
    u8 intctrl_ {};
    u8 intflags_ {};
    u8 status_ {};
    u8 dbgctrl_ {};
    u8 command_ {};
    u8 muxpos_ {};
    u16 result_ {};
    u16 sample_ {};
    u16 winlt_ {};
    u16 winht_ {};

    u64 cycles_remaining_ {0};
    bool converting_ {false};
};

} // namespace vioavr::core
