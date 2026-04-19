#pragma once

#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_change_interrupt.hpp"
#include "vioavr/core/sync_engine.hpp"
#include "vioavr/core/pin_map.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/timer8.hpp"

#include <memory>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>

namespace vioavr::core {

class Dac;
class GpioPort;

/**
 * @brief High-level bridge for ngspice/XSPICE integration.
 */
class VioSpice {
public:
    VioSpice(const DeviceDescriptor& device);
    ~VioSpice() = default;

    void set_pin_map(std::unique_ptr<PinMap> pin_map);
    void add_pin_mapping(std::string_view port_name, u8 bit_index, u32 external_id, std::string_view label = "");
    void add_trace_hook(ITraceHook* hook);
    void set_quantum(u64 cycles);
    void set_frequency(double hz);
    
    bool load_hex(std::string_view path);
    void reset();

    void step_duration(double seconds);
    void tick_timer2_async(u64 ticks);
    
    void set_external_pin(u32 external_id, PinLevel level);
    void set_external_voltage(u8 channel, double normalized_voltage);

    std::vector<PinStateChange> consume_pin_changes();
    [[nodiscard]] std::optional<u32> get_external_id(std::string_view port_name, u8 bit_index) const;
    std::vector<double> get_analog_outputs();

    [[nodiscard]] AvrCpu& cpu() noexcept { return cpu_; }
    [[nodiscard]] MemoryBus& bus() noexcept { return bus_; }
    [[nodiscard]] AnalogSignalBank& analog_signal_bank() noexcept { return analog_signal_bank_; }

private:
    PinMux pin_mux_;
    MemoryBus bus_;
    AvrCpu cpu_;
    AnalogSignalBank analog_signal_bank_;
    Timer8* timer2_ {};
    std::vector<Dac*> dacs_;
    std::vector<GpioPort*> ports_;
    std::unordered_map<std::string, GpioPort*> port_map_;
    
    PinChangeInterruptSharedState pcint_shared_state_ {};
    std::unique_ptr<PinMap> pin_map_;
    TraceMultiplexer trace_mux_;
    u64 quantum_ {1000};
    double frequency_ {16000000.0};
};

} // namespace vioavr::core
