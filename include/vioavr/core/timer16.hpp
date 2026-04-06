#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <array>
#include <optional>

namespace vioavr::core {
class GpioPort;
class MemoryBus;

class Timer16 final : public IoPeripheral {
public:
    explicit Timer16(std::string_view name, const DeviceDescriptor& device) noexcept;
    explicit Timer16(std::string_view name, const Timer16Descriptor& desc) noexcept;

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

    // Co-simulation hooks
    void connect_input_capture(GpioPort& port, u8 bit) noexcept;
    void connect_compare_output_a(GpioPort& port, u8 bit) noexcept;
    void connect_compare_output_b(GpioPort& port, u8 bit) noexcept;

    [[nodiscard]] constexpr u16 counter() const noexcept { return tcnt_; }
    [[nodiscard]] constexpr u16 input_capture() const noexcept { return icr_; }

private:
    enum class Mode {
        normal,
        pc_pwm_8bit, pc_pwm_9bit, pc_pwm_10bit,
        ctc_ocr,
        fast_pwm_8bit, fast_pwm_9bit, fast_pwm_10bit,
        pfc_pwm_icr, pfc_pwm_ocr,
        pc_pwm_icr, pc_pwm_ocr,
        ctc_icr,
        fast_pwm_icr, fast_pwm_ocr
    };

    enum class PinAction {
        none = 0,
        toggle = 1,
        clear = 2,
        set = 3
    };

    struct BoundPin {
        GpioPort* port {};
        u8 bit {};
    };

    void update_mode() noexcept;
    void perform_tick() noexcept;
    void handle_compare_match_a() noexcept;
    void handle_compare_match_b() noexcept;
    void handle_matches() noexcept;
    void handle_overflow() noexcept;
    void handle_input_capture() noexcept;
    [[nodiscard]] u16 get_top() const noexcept;
    
    void apply_pin_action(std::optional<BoundPin> pin, PinAction action) noexcept;
    [[nodiscard]] PinAction get_pin_action_a() const noexcept;
    [[nodiscard]] PinAction get_pin_action_b() const noexcept;

    std::string_view name_;
    Timer16Descriptor desc_;
    std::array<AddressRange, 4> ranges_ {};
    MemoryBus* bus_ {};

    u16 tcnt_ {};
    u16 ocra_ {};
    u16 ocrb_ {};
    u16 icr_ {};
    u8 tccra_ {};
    u8 tccrb_ {};
    u8 tccrc_ {};
    u8 timsk_ {};
    u8 tifr_ {};
    u8 temp_ {};

    Mode mode_ {Mode::normal};
    bool counting_up_ {true};
    
    std::optional<BoundPin> pin_a_;
    std::optional<BoundPin> pin_b_;
    std::optional<BoundPin> pin_icp_;
    u8 last_icp_state_ {0};
    u8 last_t1_state_ {0};
    u8 noise_canceler_register_ {0};
    u8 noise_canceler_counter_ {0};
    u64 cycle_accumulator_ {0};
};

}  // namespace vioavr::core
