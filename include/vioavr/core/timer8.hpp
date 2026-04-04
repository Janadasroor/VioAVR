#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <array>
#include <optional>

namespace vioavr::core {
class GpioPort;
class MemoryBus;

class Timer8 final : public IoPeripheral {
public:
    enum class Mode : u8 {
        normal,
        pc_pwm_ff,
        ctc_ocra,
        fast_pwm_ff,
        pc_pwm_ocra = 5,
        fast_pwm_ocra = 7
    };

    enum class PinAction : u8 {
        disconnected,
        toggle,
        clear,
        set
    };

    explicit Timer8(std::string_view name, const DeviceDescriptor& device) noexcept;
    explicit Timer8(std::string_view name, const Timer8Descriptor& desc) noexcept;

    void set_bus(MemoryBus& bus) noexcept { bus_ = &bus; }

    [[nodiscard]] std::string_view name() const noexcept override;

    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    void connect_compare_output_a(GpioPort& port, u8 bit) noexcept;
    void connect_compare_output_b(GpioPort& port, u8 bit) noexcept;
    void connect_adc_auto_trigger(class Adc& adc) noexcept;
    void connect_adc_overflow_auto_trigger(class Adc& adc) noexcept;

    [[nodiscard]] constexpr u8 interrupt_flags() const noexcept { return tifr_; }
    [[nodiscard]] constexpr u8 interrupt_mask() const noexcept { return timsk_; }
    [[nodiscard]] constexpr u8 counter() const noexcept { return tcnt_; }
    [[nodiscard]] constexpr u8 control_a() const noexcept { return tccra_; }
    [[nodiscard]] constexpr u8 control_b() const noexcept { return tccrb_; }
    [[nodiscard]] constexpr u8 compare_a() const noexcept { return ocra_; }
    [[nodiscard]] constexpr u8 compare_b() const noexcept { return ocrb_; }
    [[nodiscard]] constexpr bool running() const noexcept { return (tccrb_ & 0x07U) != 0U; }

private:
    struct BoundPin {
        GpioPort* port;
        u8 bit;
    };

    void perform_tick() noexcept;
    void update_mode() noexcept;
    void handle_compare_match_a() noexcept;
    void handle_compare_match_b() noexcept;
    void handle_overflow() noexcept;
    [[nodiscard]] u8 get_top() const noexcept;
    void apply_pin_action(std::optional<BoundPin> pin, PinAction action) noexcept;
    [[nodiscard]] PinAction get_pin_action_a() const noexcept;
    [[nodiscard]] PinAction get_pin_action_b() const noexcept;

    std::string_view name_;
    Timer8Descriptor desc_;
    std::array<AddressRange, 4> ranges_ {};
    MemoryBus* bus_ {};


    u8 tcnt_ {};
    u8 ocra_ {};
    u8 ocrb_ {};
    u8 tccra_ {};
    u8 tccrb_ {};
    u8 timsk_ {};
    u8 tifr_ {};

    Mode mode_ {Mode::normal};
    bool counting_up_ {true};
    std::optional<BoundPin> pin_a_;
    std::optional<BoundPin> pin_b_;
    u8 last_clk_pin_state_ {};
    u64 cycle_accumulator_ {};
    class Adc* adc_compare_trigger_ {};
    class Adc* adc_overflow_trigger_ {};
};

}  // namespace vioavr::core
