#pragma once
#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/port_mux.hpp"
#include "vioavr/core/awex.hpp"
#include <array>
#include <string>

namespace vioavr::core {

class MemoryBus;
class EventSystem;
class PortMux;
class PinMux;

class Tc final : public IoPeripheral, public IRoutingObserver {
public:
    explicit Tc(std::string name, const TcDescriptor& desc) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_event_system(EventSystem* evsys) noexcept override;
    void set_port_mux(PortMux* port_mux) noexcept { port_mux_ = port_mux; }
    void set_pin_mux(PinMux* pm) noexcept { pin_mux_ = pm; }
    void set_port_index(u8 idx) noexcept { port_index_ = idx; }
    [[nodiscard]] u8 port_index() const noexcept { return port_index_; }
    void set_wg_mode_in_ctrld(bool v) noexcept { wg_mode_in_ctrld_ = v; }
    void set_awex(Awex* awex) noexcept {
        awex_companion_ = awex;
        if (awex) awex->set_port_index(port_index_);
    }

    [[nodiscard]] std::string_view name() const noexcept override { return name_; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    void on_routing_changed() noexcept override;

    [[nodiscard]] bool get_wo_level(u8 index) const noexcept;

    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

private:
    [[nodiscard]] bool is_enabled() const noexcept;
    [[nodiscard]] u8 get_clk_sel() const noexcept;
    [[nodiscard]] u8 get_wg_mode() const noexcept;
    [[nodiscard]] bool is_single_slope_pwm() const noexcept;

    void perform_tick() noexcept;
    void handle_matches(u16 prev_cnt) noexcept;
    void commit_buffers() noexcept;
    void handle_cmd(u8 cmd) noexcept;
    void update_prescaler() noexcept;
    void update_outputs() noexcept;
    void update_interrupt_state() noexcept;
    void update_active_state() noexcept;
    void on_event(bool level) noexcept;

    std::string name_;
    const TcDescriptor desc_;
    std::array<AddressRange, 8> ranges_;
    MemoryBus* bus_ {};
    EventSystem* evsys_ {};
    PortMux* port_mux_ {};
    PinMux* pin_mux_ {};
    Awex* awex_companion_ {};
    u8 port_index_ {0xFF};

    u8 ctrla_ {};
    u8 ctrlb_ {};
    u8 ctrlc_ {};
    u8 ctrld_ {};
    u8 ctrle_ {};
    u8 intctrla_ {};
    u8 intctrlb_ {};
    u8 ctrlf_ {};
    u8 ctrlg_ {};
    u8 intflags_ {};
    u8 temp_ {};

    u16 cnt_ {};
    u16 per_ {0xFFFF};
    u16 cca_ {};
    u16 ccb_ {};
    u16 ccc_ {};
    u16 ccd_ {};
    u16 per_buf_ {};
    u16 cca_buf_ {};
    u16 ccb_buf_ {};
    u16 ccc_buf_ {};
    u16 ccd_buf_ {};
    bool per_buf_valid_ {};
    bool cca_buf_valid_ {};
    bool ccb_buf_valid_ {};
    bool ccc_buf_valid_ {};
    bool ccd_buf_valid_ {};

    u64 prescaler_counter_ {};
    u64 prescaler_limit_ {1};
    bool counting_up_ {true};
    bool last_event_level_ {};
    bool just_hit_top_ {};
    bool just_hit_bottom_ {};
    bool wo_states_[4] {};
    bool wg_mode_in_ctrld_ {};
};

} // namespace vioavr::core
