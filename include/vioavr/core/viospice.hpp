#pragma once

#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_change_interrupt.hpp"
#include "vioavr/core/sync_engine.hpp"
#include "vioavr/core/pin_map.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/timer8.hpp"

#include <memory>
#include <string_view>

namespace vioavr::core {

/**
 * @brief High-level bridge for ngspice/XSPICE integration.
 * Manages CPU, Memory, Sync, and Pin Mapping in one unit.
 */
class VioSpice {
public:
    VioSpice(const DeviceDescriptor& device);
    ~VioSpice() = default;

    // Configuration
    void set_pin_map(std::unique_ptr<PinMap> pin_map);
    void add_pin_mapping(std::string_view port_name, u8 bit_index, u32 external_id, std::string_view label = "");
    void set_quantum(u64 cycles);
    
    // Lifecycle
    bool load_hex(std::string_view path);
    void reset();

    // Simulation Step (Called by SPICE)
    void step_duration(double seconds);
    void tick_timer2_async(u64 ticks);
    
    // External Signal Injection (From SPICE nodes to AVR pins)
    void set_external_pin(u32 external_id, PinLevel level);
    void set_external_voltage(u8 channel, double normalized_voltage);

    // Get changes to publish back to SPICE
    std::vector<PinStateChange> consume_pin_changes();

    // Mapping helper
    [[nodiscard]] std::optional<u32> get_external_id(std::string_view port_name, u8 bit_index) const;

    // Accessors
    [[nodiscard]] AvrCpu& cpu() noexcept { return cpu_; }
    [[nodiscard]] MemoryBus& bus() noexcept { return bus_; }
    [[nodiscard]] AnalogSignalBank& analog_signal_bank() noexcept { return analog_signal_bank_; }

private:
    [[nodiscard]] bool is_timer2_async_input(std::string_view port_name, u8 bit_index) const noexcept;

    MemoryBus bus_;
    AvrCpu cpu_;
    AnalogSignalBank analog_signal_bank_;
    Timer8* timer2_ {};
    PinChangeInterruptSharedState pcint_shared_state_ {};
    bool timer2_async_input_high_ {};
    std::unique_ptr<PinMap> pin_map_;
    std::unique_ptr<SyncEngine> sync_;
    u64 quantum_ {1000};
};

} // namespace vioavr::core
