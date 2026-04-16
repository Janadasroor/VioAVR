#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/cpu_control.hpp"
#include "vioavr/core/timer10.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/devices/atmega32u4.hpp"
#include "vioavr/core/logger.hpp"

namespace {
using namespace vioavr::core;
using namespace vioavr::core::devices;
}

TEST_CASE("Timer10 High-Speed PLL Clock Fidelity")
{
    MemoryBus bus {atmega32u4};
    Timer10 timer4 {"TIMER4", atmega32u4.timers10[0]};
    timer4.set_bus(bus);
    bus.attach_peripheral(timer4);
    
    AvrCpu cpu {bus};
    auto cpu_ctrl = std::make_shared<CpuControl>(cpu, atmega32u4);
    bus.attach_peripheral(*cpu_ctrl);

    // 1. Initial State: PCKE=0, Ratio=1
    timer4.reset();
    bus.write_data(atmega32u4.timers10[0].ocrc_address, 0xFF);
    timer4.tick(10);
    CHECK(timer4.read(atmega32u4.timers10[0].tcnt_address) == 10);

    // 2. Enable PLL PCKE (PLLCSR = 0x04)
    bus.write_data(atmega32u4.timers10[0].pllcsr_address, 0x04);
    timer4.reset();
    bus.write_data(atmega32u4.timers10[0].ocrc_address, 0xFF);
    timer4.tick(10); // 10 CPU cycles = 80 Timer cycles
    CHECK(timer4.read(atmega32u4.timers10[0].tcnt_address) == 80);
}

TEST_CASE("Timer10 Output PWM Fidelity")
{
    MemoryBus bus {atmega32u4};
    Timer10 timer4 {"TIMER4", atmega32u4.timers10[0]};
    timer4.set_bus(bus);
    bus.attach_peripheral(timer4);

    AvrCpu cpu {bus};
    auto cpu_ctrl = std::make_shared<CpuControl>(cpu, atmega32u4);
    bus.attach_peripheral(*cpu_ctrl);
    
    GpioPort portc {"PORTC", 0x26, 0x27, 0x28}; // Port C for OC4A (PC7)
    bus.attach_peripheral(portc);
    timer4.connect_compare_output_a(portc, 7);

    timer4.reset();
    
    // Set TCCRA = 0 to allow immediate OCR update, then set to PWM
    bus.write_data(atmega32u4.timers10[0].tccra_address, 0x00);
    bus.write_data(atmega32u4.timers10[0].ocra_address, 40);
    bus.write_data(atmega32u4.timers10[0].ocrc_address, 100);
    bus.write_data(atmega32u4.timers10[0].tccra_address, 0x82);

    // Set DDRC7 = 1
    bus.write_data(0x27, 0x80);

    // Force one overflow to set the pin
    timer4.write(atmega32u4.timers10[0].tcnt_address, 99);
    timer4.tick(1); // TCNT 99 -> 100 (Match OC4C/TOP -> Overflow to 0)
    CHECK((bus.read_data(0x28) & 0x80) != 0); // Should be HIGH now (set at overflow)

    timer4.tick(39); // TCNT 0 -> 39
    CHECK((bus.read_data(0x28) & 0x80) != 0); // Still HIGH
    
    timer4.tick(1); // TCNT 39 -> 40 (Match OC4A)
    CHECK((bus.read_data(0x28) & 0x80) == 0); // Should be LOW now

    timer4.tick(59); // TCNT 40 -> 99
    CHECK((bus.read_data(0x28) & 0x80) == 0);

    timer4.tick(1); // TCNT 99 -> 100 -> Overflow to 0
    CHECK((bus.read_data(0x28) & 0x80) != 0); // Should be HIGH again
}
