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

    MemoryBus bus {devices::atmega328};
    
    // Attach peripherals for "new features"
    PinMux pin_mux {8};
    Adc adc {"ADC", devices::atmega328.adcs[0], pin_mux, 6U};
    Timer8 timer0 {"TIMER0", devices::atmega328.timers8[0]};
    AnalogComparator ac {"AC", devices::atmega328.acs[0], pin_mux, 7U}; 
    AvrCpu cpu_for_wdt {bus}; // Used to pass to WDT for reset

    // Connect auto-triggers
    adc.connect_timer_overflow_auto_trigger(timer0);
    adc.connect_timer_compare_auto_trigger(timer0);
    adc.connect_comparator_auto_trigger(ac);

    WatchdogTimer wdt {"WDT", devices::atmega328.wdts[0], cpu_for_wdt};
    Eeprom eeprom {"EEPROM", devices::atmega328.eeproms[0]};

    bus.attach_peripheral(adc);
    bus.attach_peripheral(timer0);
    bus.attach_peripheral(ac);
    bus.attach_peripheral(wdt);
    bus.attach_peripheral(eeprom);

    try {
        HexImage image = HexImageLoader::load_file(hex_file, devices::atmega328);
        bus.load_image(image);
    } catch (const std::exception& e) {
        std::cerr << "Error loading hex: " << e.what() << std::endl;
        return 1;
    }

    AvrCpu cpu {bus};
    cpu.reset();

    // Print Header
    std::cout << "Cycle,PC,SREG,SP";
    for (int i = 0; i < 32; ++i) std::cout << ",R" << i;
    std::cout << ",TCNT0,ADCSRA,ACSR,SPMCSR,WDTCSR,EECR" << std::endl;

    for (u64 cycle = 0; cycle < cycle_limit; ++cycle) {
        auto snap = cpu.snapshot();
        
        // Print State before step
        std::cout << snap.cycles << "," 
                  << std::hex << snap.program_counter << ","
                  << (int)snap.sreg << ","
                  << snap.stack_pointer << std::dec;
        
        for (int i = 0; i < 32; ++i) {
            std::cout << "," << (int)snap.gpr[i];
        }

        // Selected I/O registers
        std::cout << "," << (int)bus.read_data(devices::atmega328.timers8[0].tcnt_address)
                  << "," << (int)bus.read_data(devices::atmega328.adcs[0].adcsra_address)
                  << "," << (int)bus.read_data(devices::atmega328.acs[0].acsr_address)
                  << "," << (int)bus.read_data(devices::atmega328.spmcsr_address)
                  << "," << (int)bus.read_data(devices::atmega328.wdts[0].wdtcsr_address)
                  << "," << (int)bus.read_data(devices::atmega328.eeproms[0].eecr_address)
                  << std::endl;

        cpu.step();
        
        if (snap.state == CpuState::halted) break;
    }

    return 0;
}
