#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/devices/atmega328.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/sync_engine.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/logger.hpp"

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>

using namespace vioavr::core;

class RCFilter {
public:
    RCFilter(double R, double C, double initial_v = 0.0)
        : r_(R), c_(C), v_out_(initial_v) {}

    void step(double v_in, double dt_sec) {
        const double tau = r_ * c_;
        v_out_ += (v_in - v_out_) * (dt_sec / tau);
    }

    double voltage() const { return v_out_; }

private:
    double r_;
    double c_;
    double v_out_;
};

namespace firmware {
    u16 ldi(u8 rd, u8 k) { return static_cast<u16>(0xE000 | ((k & 0xF0) << 4) | ((rd - 16) << 4) | (k & 0x0F)); }
    u16 out(u8 a, u8 rr) { return static_cast<u16>(0xB800 | ((rr & 0x1F) << 4) | ((a & 0x30) << 5) | (a & 0x0F)); }
    u16 rjmp(int k) { return static_cast<u16>(0xC000 | (k & 0x0FFF)); }
    u16 dec(u8 rd) { return static_cast<u16>(0x940A | (static_cast<u16>(rd) << 4)); }
    u16 brne(int k) { return static_cast<u16>(0xF401 | ((k & 0x7F) << 3)); }
}

int main() {
    Logger::set_callback([](LogLevel level, std::string_view message) {
        if (level != LogLevel::debug) {
            std::cerr << "[" << (level == LogLevel::error ? "ERROR" : "INFO") << "] " << message << std::endl;
        }
    });

    // Use std::cerr for simulation output to bypass possible stdout buffering issues in the environment
    auto& out = std::cerr;

    out << "Starting VIOSpice Mixed-Mode POC Demo..." << std::endl;
    out << "Scenario: ATmega328 PB0 toggling every 50ms -> Virtual RC Filter (R=10k, C=10uF)" << std::endl;
    out << "--------------------------------------------------------------------------------" << std::endl;

    PinMux pin_mux {5};
    MemoryBus bus {devices::atmega328};
    AvrCpu cpu {bus};
    auto gpio_b = std::make_unique<GpioPort>(devices::atmega328.ports[1], pin_mux);
    bus.attach_peripheral(*gpio_b);

    const u64 quantum = 10000;
    auto sync = create_fixed_quantum_sync_engine(quantum);
    cpu.set_sync_engine(sync.get());

    // Firmware with a simple 2-cycle interior delay loop
    // Approx 50ms at 16MHz = 800,000 cycles.
    // Inner loop (DEC + BRNE) = 3 cycles per iteration.
    // 800,000 / 3 = 266,666. Need nested loops.
    
    std::vector<u16> blink_with_delay = {
        firmware::ldi(16, 0x01),      // 0: DDRB set PB0 as output
        firmware::out(0x04, 16),      // 1:
        
        // Loop Start (Address 2)
        firmware::out(0x05, 16),      // 2: PORTB = 1
        firmware::ldi(20, 100),       // 3: Delay approx 50ms (simplified)
        firmware::dec(20),            // 4: 
        firmware::brne(-1),           // 5: 
        
        firmware::ldi(17, 0x00),      // 6: 
        firmware::out(0x05, 17),      // 7: PORTB = 0
        firmware::ldi(20, 100),       // 8: Delay
        firmware::dec(20),            // 9: 
        firmware::brne(-1),           // 10: 
        
        firmware::rjmp(-9)            // 11: Back to 2
    };

    bus.load_flash(blink_with_delay);
    cpu.reset();

    RCFilter circuit(10000.0, 0.000010); 
    double v_in = 0.0;
    double sim_time = 0.0;
    const double dt = static_cast<double>(quantum) / 16'000'000.0;

    out << std::left << std::setw(12) << "Time(s)" 
        << std::setw(15) << "AVR PB0" 
        << std::setw(15) << "V_Capacitor" 
        << "Visual" << std::endl;
    out << std::string(60, '-') << std::endl;

    for (int step = 0; step < 200; ++step) {
        cpu.run(quantum);
        
        auto changes = sync->consume_pin_changes();
        for (const auto& change : changes) {
            if (change.port_name == "PORTB" && change.bit_index == 0) {
                v_in = change.level ? 5.0 : 0.0;
            }
        }

        circuit.step(v_in, dt);
        sim_time += dt;

        if (step % 5 == 0) {
            int bar_width = static_cast<int>(circuit.voltage() * 4.0);
            std::string bar(bar_width, '#');
            
            out << std::fixed << std::setprecision(4) 
                << std::left << std::setw(12) << sim_time
                << std::setw(15) << (v_in > 2.5 ? "HIGH (5V)" : "LOW  (0V)")
                << std::setw(15) << circuit.voltage() << "V |"
                << bar << std::endl;
        }

        sync->resume();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    out << "\nDemo Finished." << std::endl;
    return 0;
}
