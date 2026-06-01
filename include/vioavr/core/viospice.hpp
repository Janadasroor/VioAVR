#pragma once

#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_change_interrupt.hpp"
#include "vioavr/core/sync_engine.hpp"
#include "vioavr/core/pin_map.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/hc05.hpp"

#include <memory>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace vioavr::core {

class Dac;
class XmegaDac;
class GpioPort;
class Ircom;
class LcdController;
class PortMux;
class EventSystem;
class Uart;
class Uart8x;

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
    bool load_hex_image(const HexImage& image);
    void reset();

    void step_duration(double seconds);
    void tick_timer2_async(u64 ticks);
    
    void set_external_pin(u32 external_id, PinLevel level);
    void set_external_voltage(u8 channel, double voltage_volts);
    void set_external_voltage_to_digital(u32 external_id, double voltage);
    void set_operating_voltage(double vcc);

    // HC-05 Bluetooth bridge
    void enable_hc05();
    [[nodiscard]] bool hc05_enabled() const noexcept { return hc05_enabled_; }
    [[nodiscard]] Hc05& hc05() noexcept { return hc05_; }
    void set_hc05_pty_fd(int fd) noexcept { hc05_pty_fd_ = fd; }

    std::vector<PinStateChange> consume_pin_changes();
    [[nodiscard]] std::optional<u32> get_external_id(std::string_view port_name, u8 bit_index) const;
    std::vector<double> get_analog_outputs();
    void set_ircom_output_pin(u32 external_id);

    [[nodiscard]] AvrCpu& cpu() noexcept { return cpu_; }
    [[nodiscard]] MemoryBus& bus() noexcept { return bus_; }
    [[nodiscard]] AnalogSignalBank& analog_signal_bank() noexcept { return analog_signal_bank_; }
    [[nodiscard]] double frequency() const noexcept { return frequency_; }
    [[nodiscard]] LcdController* lcd() const noexcept { return lcd_; }

private:
    PinMux pin_mux_;
    MemoryBus bus_;
    AvrCpu cpu_;
    AnalogSignalBank analog_signal_bank_;
    Timer8* timer2_ {};
    std::vector<Dac*> dacs_;
    std::vector<XmegaDac*> xmega_dacs_;
    std::vector<GpioPort*> ports_;
    std::unordered_map<std::string, GpioPort*> port_map_;
    std::unordered_map<u8, std::string> port_idx_to_name_;
    LcdController* lcd_ {nullptr};
    PortMux* port_mux_ {nullptr};
    EventSystem* evsys_ {nullptr};
    
    std::vector<PinStateChange> pending_pin_changes_;
    PinChangeInterruptSharedState pcint_shared_state_ {};
    std::unique_ptr<PinMap> pin_map_;
    TraceMultiplexer trace_mux_;
    u64 quantum_ {1000};
    double frequency_ {16000000.0};
    std::vector<std::unique_ptr<IoPeripheral>> owned_peripherals_;
    Uart* uart0_ {nullptr};
    Uart8x* uart8x0_ {nullptr};
    Hc05 hc05_;
    bool hc05_enabled_ {false};
    int  hc05_pty_fd_ {-1};

    void bridge_hc05();
};

} // namespace vioavr::core
