#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <array>
#include <string>
#include <string_view>

namespace vioavr::core {

class AnalogSignalBank;
class EventSystem;

class XmegaDac final : public IoPeripheral {
public:
    explicit XmegaDac(std::string_view name, const XmegaDacDescriptor& desc) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }

    void set_analog_signal_bank(AnalogSignalBank* bank) noexcept { signal_bank_ = bank; }
    void set_event_system(EventSystem* evsys) noexcept override { evsys_ = evsys; }
    void set_vref(double v) noexcept { vref_ = v; }

    [[nodiscard]] double ch0_voltage() const noexcept {
        return compute_voltage(get_channel_data(0));
    }
    [[nodiscard]] double ch1_voltage() const noexcept {
        return compute_voltage(get_channel_data(1));
    }

    [[nodiscard]] std::string_view name() const noexcept override { return name_; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] bool wants_tick() const noexcept override { return true; }

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

private:
    void start_conversion(u8 channel);
    void complete_conversion(u8 channel);
    [[nodiscard]] u16 get_channel_data(u8 channel) const noexcept;
    [[nodiscard]] double compute_voltage(u16 data) const noexcept;

    std::string name_;
    XmegaDacDescriptor desc_;
    std::array<AddressRange, 4> ranges_{};
    u8 num_ranges_{0};
    MemoryBus* bus_ {};
    AnalogSignalBank* signal_bank_ {};
    EventSystem* evsys_ {};
    double vref_ {5.0};

    u8 ctrla_{};
    u8 ctrlb_{};
    u8 ctrlc_{};
    u8 evctrl_{};
    u8 status_{};
    u8 ch0gaincal_{};
    u8 ch0offsetcal_{};
    u8 ch1gaincal_{};
    u8 ch1offsetcal_{};
    u16 ch0data_{};
    u16 ch1data_{};

    u16 cycles_remaining_{};
    u8 converting_channel_{};
    bool converting_{};
};

} // namespace vioavr::core
