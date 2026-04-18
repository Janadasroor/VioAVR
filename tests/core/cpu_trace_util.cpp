#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/analog_comparator.hpp"
#include "vioavr/core/watchdog_timer.hpp"
#include "vioavr/core/eeprom.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/atmega328.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/machine.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

using namespace vioavr::core;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: cpu_trace_util <hex_file> <cycles>" << std::endl;
        return 1;
    }

    std::string hex_file = argv[1];
    u64 cycle_limit = std::stoull(argv[2]);

    const auto* dev = DeviceCatalog::find("ATmega328P");
    if (!dev) {
        std::cerr << "Error: atmega328p not found" << std::endl;
        return 1;
    }

    Machine machine(*dev);
    
    try {
        HexImage image = HexImageLoader::load_file(hex_file, *dev);
        machine.bus().load_image(image);
    } catch (const std::exception& e) {
        std::cerr << "Error loading hex: " << e.what() << std::endl;
        return 1;
    }

    // Suppress debug/info logs
    Logger::set_callback([](LogLevel level, std::string_view message) {
        if (static_cast<int>(level) >= static_cast<int>(LogLevel::warning)) {
            std::cerr << "[" << (level == LogLevel::warning ? "WARN" : "ERROR") << "] " << message << std::endl;
        }
    });

    AvrCpu& cpu = machine.cpu();
    MemoryBus& bus = machine.bus();
    cpu.reset();

    // Print Header matching simavr_tracer.c
    std::cout << "Cycle,PC,SREG,SP";
    for (int i = 0; i < 32; ++i) std::cout << ",R" << i;
    std::cout << ",PORTB,DDRB,PORTD,DDRD,TCNT1L,TCNT1H,TCNT0,ADCSRA,ACSR,State" << std::endl;

    u64 last_reported_cycle = 0;
    while (last_reported_cycle < cycle_limit) {
        auto snap = cpu.snapshot();
        last_reported_cycle = snap.cycles;
        
        // Print State before step
        std::cout << std::dec << snap.cycles << "," 
                  << std::hex << std::setfill('0') << std::setw(4) << (snap.program_counter * 2) << ","
                  << std::setw(2) << (int)snap.sreg << ","
                  << std::setw(4) << snap.stack_pointer;
        
        for (int i = 0; i < 32; ++i) {
            std::cout << "," << std::setw(2) << (int)snap.gpr[i];
        }

        // GPIO and Timer Registers
        std::cout << "," << std::setw(2) << (int)bus.read_data(0x25)  // PORTB
                  << "," << std::setw(2) << (int)bus.read_data(0x24)  // DDRB
                  << "," << std::setw(2) << (int)bus.read_data(0x2b)  // PORTD
                  << "," << std::setw(2) << (int)bus.read_data(0x2a)  // DDRD
                  << "," << std::setw(2) << (int)bus.read_data(0x84)  // TCNT1L
                  << "," << std::setw(2) << (int)bus.read_data(0x85)  // TCNT1H
                  << "," << std::setw(2) << (int)bus.read_data(0x46)  // TCNT0
                  << "," << std::setw(2) << (int)bus.read_data(0x7a)  // ADCSRA
                  << "," << std::setw(2) << (int)bus.read_data(0x50)  // ACSR
                  << "," << (int)snap.state << std::endl;

        cpu.step();
        
        if (snap.state == CpuState::halted) break;
    }

    return 0;
}
