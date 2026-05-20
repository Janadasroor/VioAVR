#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <array>
#include <optional>
#include <string>
#include <span>
#include <vector>

namespace vioavr::core {
class GpioPort;
class MemoryBus;
class Adc;
class Dac;
class AnalogComparator;

class Timer16 final : public IoPeripheral {
public:
    explicit Timer16(std::string_view name, const Timer16Descriptor& desc, class PinMux* pin_mux = nullptr) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_bus(MemoryBus& bus) noexcept { bus_ = &bus; }

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    [[nodiscard]] u16 counter() const noexcept { return tcnt_; }
    [[nodiscard]] u16 input_capture() const noexcept { return icr_; }

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] bool wants_tick() const noexcept override { return false; }
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;
    
    void sync() noexcept override;
    void on_power_state_change() noexcept override;
    void on_event(u64 cycle) noexcept;
    bool on_external_pin_change(u16 address, u8 bit, PinLevel level) noexcept override;
    void notify_input_capture(bool high) noexcept;

    void connect_input_capture(GpioPort& port, u8 bit) noexcept { pin_icp_ = {&port, bit}; }
    void connect_compare_output_a(GpioPort& port, u8 bit) noexcept { pin_a_ = {&port, bit}; }
    void connect_compare_output_b(GpioPort& port, u8 bit) noexcept { pin_b_ = {&port, bit}; }
    void connect_compare_output_c(GpioPort& port, u8 bit) noexcept { pin_c_ = {&port, bit}; }
    void connect_adc(Adc& adc) noexcept { adc_auto_triggers_.push_back(&adc); }
    void connect_dac(Dac& dac) noexcept { dac_triggers_.push_back(&dac); }
    void connect_analog_comparator(AnalogComparator& ac) noexcept;

private:
    [[nodiscard]] bool power_reduction_enabled() const noexcept;
    void perform_tick() noexcept;
    void recalculate_event() noexcept;
    void handle_matches() noexcept;
    void handle_input_capture() noexcept;
    void update_pwm_pins() noexcept;
    void update_mode() noexcept;
    void update_pin_ownership() noexcept;

    enum class Mode { normal, ctc_ocr, ctc_icr };

    struct BoundPin { GpioPort* port; u8 bit; };

    PinMux* pin_mux_ {};
    std::string name_;
    Timer16Descriptor desc_;
    std::vector<Adc*> adc_auto_triggers_;
    std::vector<Dac*> dac_triggers_;
    std::array<AddressRange, 8> ranges_ {};
    MemoryBus* bus_ {};

    u16 tcnt_ {};
    u16 ocra_ {};
    u16 ocrb_ {};
    u16 ocrc_ {};
    u16 ocra_buffer_ {};
    u16 ocrb_buffer_ {};
    u16 ocrc_buffer_ {};
    u16 icr_ {};
    u8 tccra_ {};
    u8 tccrb_ {};
    u8 tccrc_ {};
    u8 timsk_ {};
    u8 tifr_ {};
    u8 temp_ {};

    Mode mode_ {Mode::normal};
    bool counting_up_ {true};
    
    std::optional<BoundPin> pin_icp_;
    std::optional<BoundPin> pin_a_;
    std::optional<BoundPin> pin_b_;
    std::optional<BoundPin> pin_c_;
    bool icp_initialized_ {false};
    u8 last_icp_state_ {0};
    u8 last_t1_state_ {0};
    u8 noise_canceler_register_ {0};
    u8 noise_canceler_counter_ {0};
    u64 cycle_accumulator_ {0};
    u64 last_sync_cycle_ {0};
};

} // namespace vioavr::core
