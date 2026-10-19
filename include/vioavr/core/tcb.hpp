#pragma once
#include "vioavr/core/port_mux.hpp"
#include <string>
#include <array>
#include <span>


namespace vioavr::core {

class MemoryBus;

/**
 * @brief 16-bit Timer/Counter Type B (TCB)
 * Optimized for input capture and PWM in modern AVR devices.
 */
class Tcb : public IoPeripheral, public IRoutingObserver {
public:
    explicit Tcb(std::string name, const TcbDescriptor& desc);

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] std::string_view name() const noexcept override { return name_; }
    u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    void on_routing_changed() noexcept override;
    void on_power_state_change() noexcept override;

    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;
    
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;
    [[nodiscard]] bool supports_interrupt_mask() const noexcept override { return true; }
    
    [[nodiscard]] bool get_wo_level() const noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_event_system(EventSystem* evsys) noexcept override;
    void set_port_mux(PortMux* pm) noexcept { port_mux_ = pm; }

private:
    std::string name_;
    u8 index_{0};
    const TcbDescriptor desc_;
    MemoryBus* bus_ {nullptr};
    EventSystem* evsys_ {nullptr};
    PortMux* port_mux_ {nullptr};

    void on_event(bool level) noexcept;
    void update_outputs() noexcept;

    // Registers
    u8 ctrla_ {0x00};
    u8 ctrlb_ {0x00};
    u8 evctrl_ {0x00};
    u8 intctrl_ {0x00};
    u8 intflags_ {0x00};
    u8 status_ {0x00};
    u8 dbgctrl_ {0x00};
    u8 temp_ {0x00};

    u16 cnt_ {0x0000};
    u16 ccmp_ {0x0000};

    std::array<AddressRange, 8> ranges_ {};

    // Internal logic
    void handle_matches();
    void perform_tick(bool clock_event = true);
    bool is_enabled() const noexcept { return ctrla_ & 0x01; }
    u8 get_mode() const noexcept { return ctrlb_ & 0x07; }
    // CLKSEL width follows the device: megaAVR-0/tiny have CLKSEL[1:0]
    // (0=DIV1, 1=DIV2, 2=CLKTCA, 3=reserved; bit 3 is SYNCUPD), AVR-Dx/Ex
    // have CLKSEL[2:0] (0=DIV1, 1=DIV2, 2=TCA0/TCE0, 3=TCA1/reserved,
    // 4-6=reserved, 7=EVENT).
    u8 clksel_width() const noexcept;
    u8 effective_clksel() const noexcept;
    // Prescaler divisor of a sibling TCA/TCE clock source ("TCA0", "TCA1",
    // "TCE0"), or 0 when the source is missing or stopped.
    u32 sibling_tca_divisor(const char* name) const noexcept;

    u32 prescaler_counter_ {0};
    void update_interrupt_state() noexcept;
    void update_active_state() noexcept;
};

} // namespace vioavr::core
