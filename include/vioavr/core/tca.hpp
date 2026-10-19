#pragma once
#include "vioavr/core/port_mux.hpp"
#include <array>
#include <functional>
#include <string>
#include <span>
#include <vector>


namespace vioavr::core {

class MemoryBus;

/**
 * @brief 16-bit Timer/Counter Type A (TCA)
 * Found in modern AVR devices (AVR8X, megaAVR-0, tinyAVR-0/1/2)
 * Supports Single Mode (16-bit) and Split Mode (2x 8-bit)
 */
class Tca : public IoPeripheral, public IRoutingObserver {
public:
    explicit Tca(std::string name, const TcaDescriptor& desc);

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

    [[nodiscard]] bool get_wo_level(u8 index) const noexcept;
    // CLKTCA divisor for a cascaded TCB (TCB CLKSEL = TCA0/TCA1), or 0 when
    // this timer is stopped (the source clock is then frozen too).
    [[nodiscard]] u32 cascade_divisor() const noexcept {
        return is_enabled() ? prescaler_limit_ : 0U;
    }
    // True when the counter is clocked by EVSYS events rather than the
    // prescaled peripheral clock (EVACTA = CNT_POSEDGE/CNT_ANYEDGE).
    [[nodiscard]] bool is_event_clocked() const noexcept {
        if (!is_enabled() || (evctrl_ & 0x01U) == 0U) return false;
        const u8 action = evact_a();
        return action == 0U || action == 1U;
    }

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_event_system(EventSystem* evsys) noexcept override;
    // Consumers that are clocked by this TCA (a cascaded TCB). Each is
    // notified on every event that clocks this counter, so a cascaded TCB
    // does not have to guess an event rate in its own tick().
    void add_event_clock_consumer(std::function<void()> consumer) {
        event_clock_consumers_.push_back(std::move(consumer));
    }
    void set_port_mux(PortMux* pm) noexcept { port_mux_ = pm; }

private:
    std::string name_;
    const TcaDescriptor desc_;
    std::array<AddressRange, 8> ranges_ {};
    MemoryBus* bus_ {nullptr};
    EventSystem* evsys_ {nullptr};
    std::vector<std::function<void()>> event_clock_consumers_ {};
    PortMux* port_mux_ {nullptr};

    // Registers
    u8 ctrla_ {0x00};
    u8 ctrlb_ {0x00};
    u8 ctrlc_ {0x00};
    u8 ctrld_ {0x00};
    u8 evctrl_ {0x00};
    u8 intctrl_ {0x00};
    u8 intflags_ {0x00};
    u8 dbgctrl_ {0x00};
    u8 temp_ {0x00};
    u8 ctrle_ {0x00};  // CMD, DIR, LUPD
    u8 ctrlf_ {0x00};  // PERBV, CMP0BV, CMP1BV, CMP2BV

    // Register State
    struct {
        u16 tcnt {0x0000};
        u16 per {0xFFFF};
        u16 cmp0 {0x0000};
        u16 cmp1 {0x0000};
        u16 cmp2 {0x0000};
    } norm_;

    // Buffer Registers
    struct {
        u16 per {0xFFFF};
        u16 cmp0 {0x0000};
        u16 cmp1 {0x0000};
        u16 cmp2 {0x0000};
        bool per_valid {false};
        bool cmp0_valid {false};
        bool cmp1_valid {false};
        bool cmp2_valid {false};
    } buf_;

    // Split Mode accessors (convenience)
    u8& cnt_l() { return reinterpret_cast<u8*>(&norm_.tcnt)[0]; }
    u8 cnt_l() const { return reinterpret_cast<const u8*>(&norm_.tcnt)[0]; }
    u8& cnt_h() { return reinterpret_cast<u8*>(&norm_.tcnt)[1]; }
    u8 cnt_h() const { return reinterpret_cast<const u8*>(&norm_.tcnt)[1]; }
    u8& per_l() { return reinterpret_cast<u8*>(&norm_.per)[0]; }
    u8 per_l() const { return reinterpret_cast<const u8*>(&norm_.per)[0]; }
    u8& per_h() { return reinterpret_cast<u8*>(&norm_.per)[1]; }
    u8 per_h() const { return reinterpret_cast<const u8*>(&norm_.per)[1]; }
    u8& cmp0_l() { return reinterpret_cast<u8*>(&norm_.cmp0)[0]; }
    u8 cmp0_l() const { return reinterpret_cast<const u8*>(&norm_.cmp0)[0]; }
    u8& cmp0_h() { return reinterpret_cast<u8*>(&norm_.cmp0)[1]; }
    u8 cmp0_h() const { return reinterpret_cast<const u8*>(&norm_.cmp0)[1]; }
    u8& cmp1_l() { return reinterpret_cast<u8*>(&norm_.cmp1)[0]; }
    u8 cmp1_l() const { return reinterpret_cast<const u8*>(&norm_.cmp1)[0]; }
    u8& cmp1_h() { return reinterpret_cast<u8*>(&norm_.cmp1)[1]; }
    u8 cmp1_h() const { return reinterpret_cast<const u8*>(&norm_.cmp1)[1]; }
    u8& cmp2_l() { return reinterpret_cast<u8*>(&norm_.cmp2)[0]; }
    u8 cmp2_l() const { return reinterpret_cast<const u8*>(&norm_.cmp2)[0]; }
    u8& cmp2_h() { return reinterpret_cast<u8*>(&norm_.cmp2)[1]; }
    u8 cmp2_h() const { return reinterpret_cast<const u8*>(&norm_.cmp2)[1]; }


    // Prescaler
    u32 prescaler_counter_ {0};
    u32 prescaler_limit_ {1};

    // Buffers and Status
    bool counting_up_ {true};
    bool last_event_level_a_ {false};
    bool last_event_level_b_ {false};

    // Internal logic
    void handle_matches();
    void perform_tick();
    void perform_tick_split();
    void update_outputs();
    void handle_cmd(u8 cmd) noexcept;
    void handle_split_cmd(u8 cmd) noexcept;
    void on_event(bool level) noexcept;
    void on_event_b(bool level) noexcept;
    void perform_event_clock() noexcept;
    void update_direction_from_events() noexcept;

    [[nodiscard]] bool is_enabled() const noexcept { return ctrla_ & 0x01; }
    [[nodiscard]] bool is_split_mode() const noexcept { return ctrld_ & 0x01; }
    // EVACTA[2:0] (bits 3:1); the same numbering on ATmega4809 and DA/DB.
    [[nodiscard]] u8 evact_a() const noexcept { return static_cast<u8>((evctrl_ >> 1) & 0x07U); }
    // EVACTB[2:0] (bits 7:5), AVR DA/DB only.
    [[nodiscard]] u8 evact_b() const noexcept { return static_cast<u8>((evctrl_ >> 5) & 0x07U); }
public:
    // Event input stimulus, used by the EVSYS wiring and by tests.
    void on_event_for_test(bool level) noexcept { on_event(level); }
    void on_event_b_for_test(bool level) noexcept { on_event_b(level); }
private:
    void update_prescaler() noexcept;
    void update_active_state() noexcept;

    std::array<bool, 6> wo_states_ {false, false, false, false, false, false};
    void update_interrupt_state() noexcept;
};

} // namespace vioavr::core
