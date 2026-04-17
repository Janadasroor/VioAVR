#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <array>
#include <optional>
#include <string>

namespace vioavr::core {
class GpioPort;
class MemoryBus;
class Adc;

class Timer10 final : public IoPeripheral {
public:
    explicit Timer10(std::string_view name, const Timer10Descriptor& desc) noexcept;

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

    void connect_compare_output_a(GpioPort& port, u8 bit) noexcept;
    void connect_compare_output_a_inverted(GpioPort& port, u8 bit) noexcept;
    void connect_compare_output_b(GpioPort& port, u8 bit) noexcept;
    void connect_compare_output_b_inverted(GpioPort& port, u8 bit) noexcept;
    void connect_compare_output_d(GpioPort& port, u8 bit) noexcept;
    void connect_compare_output_d_inverted(GpioPort& port, u8 bit) noexcept;
    void connect_adc_auto_trigger(Adc& adc) noexcept;

private:
    enum class Mode {
        normal,
        pwm,
        fast_pwm,
        pfc_pwm
    };

    struct BoundPin {
        GpioPort* port {};
        u8 bit {};
    };

    [[nodiscard]] bool power_reduction_enabled() const noexcept;
    void perform_tick() noexcept;
    void apply_pin_level(std::optional<BoundPin> pin, bool high) noexcept;
    void handle_compare_match_a() noexcept;
    void handle_compare_match_b() noexcept;
    void handle_compare_match_d() noexcept;
    void handle_overflow() noexcept;

    const std::string name_;
    Timer10Descriptor desc_;
    std::array<AddressRange, 16> ranges_ {};
    u8 ranges_count_ {0};
    MemoryBus* bus_ {};
    Adc* adc_ {};

    bool pll_enabled_ {false};
    u32 pck_frequency_ {64000000}; 
    u32 clock_ratio_ {1}; 
    bool up_direction_ {true}; // For Phase Correct mode

    u16 tcnt_ {0};
    u8 tc4h_ {0};
    u16 ocra_ {0}, ocrb_ {0}, ocrc_ {0}, ocrd_ {0};
    u16 ocra_buf_ {0}, ocrb_buf_ {0}, ocrc_buf_ {0}, ocrd_buf_ {0};
    u8 tccra_ {0}, tccrb_ {0}, tccrc_ {0}, tccrd_ {0}, tccre_ {0};
    u8 dt4_ {0};
    u8 timsk_ {0}, tifr_ {0};
    u8 dt_a_ {0}, dt_a_neg_ {0};
    u8 dt_b_ {0}, dt_b_neg_ {0};
    u8 dt_d_ {0}, dt_d_neg_ {0};
    bool target_a_ {false}, target_a_neg_ {false};
    bool target_b_ {false}, target_b_neg_ {false};
    bool target_d_ {false}, target_d_neg_ {false};

    std::optional<BoundPin> pin_a_ {}, pin_a_neg_ {};
    std::optional<BoundPin> pin_b_ {}, pin_b_neg_ {};
    std::optional<BoundPin> pin_d_ {}, pin_d_neg_ {};
};

} // namespace vioavr::core
