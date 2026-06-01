#pragma once
#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/port_mux.hpp"
#include <array>
#include <string>

namespace vioavr::core {

class MemoryBus;
class EventSystem;
class PortMux;

class Tce final : public IoPeripheral, public IRoutingObserver {
public:
    explicit Tce(std::string name, const TceDescriptor& desc) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_event_system(EventSystem* evsys) noexcept override;
    void set_port_mux(PortMux* port_mux) noexcept { port_mux_ = port_mux; }

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
    [[nodiscard]] u8 reg_offset(u16 address) const noexcept;
    [[nodiscard]] bool is_enabled() const noexcept;
    [[nodiscard]] u8 get_prescaler() const noexcept;
    [[nodiscard]] u8 get_wg_mode() const noexcept;

    void perform_tick() noexcept;
    void handle_matches() noexcept;
    void handle_cmd(u8 cmd) noexcept;
    void update_prescaler() noexcept;
    void update_outputs() noexcept;
    void update_interrupt_state() noexcept;
    void update_active_state() noexcept;
    void on_event(bool level) noexcept;

    std::string name_;
    const TceDescriptor desc_;
    std::array<AddressRange, 8> ranges_;
    MemoryBus* bus_ {};
    EventSystem* evsys_ {};
    PortMux* port_mux_ {};

    u8 ctrla_ {};
    u8 ctrlb_ {};
    u8 ctrlc_ {};
    u8 ctrle_ {};
    u8 ctrlf_ {};
    u8 evgenctrl_ {};
    u8 evctrl_ {};
    u8 intctrl_ {};
    u8 intflags_ {};
    u8 dbgctrl_ {};
    u8 temp_ {};

    u16 tcnt_ {};
    u16 per_ {0xFFFF};
    u16 cmp0_ {};
    u16 cmp1_ {};
    u16 cmp2_ {};
    u16 per_buf_ {};
    u16 cmp0_buf_ {};
    u16 cmp1_buf_ {};
    u16 cmp2_buf_ {};
    bool per_buf_valid_ {};
    bool cmp0_buf_valid_ {};
    bool cmp1_buf_valid_ {};
    bool cmp2_buf_valid_ {};

    u64 prescaler_counter_ {};
    u64 prescaler_limit_ {1};
    bool counting_up_ {true};
    bool last_event_level_ {};
    bool wo_states_[3] {};
};

} // namespace vioavr::core
