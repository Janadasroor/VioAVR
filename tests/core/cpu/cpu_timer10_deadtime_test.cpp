#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/cpu_control.hpp"
#include "vioavr/core/timer10.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/atmega32u4.hpp"

namespace {
using namespace vioavr::core;
using namespace vioavr::core::devices;
}

TEST_CASE("Timer10 Dead-Time Fidelity")
{
    MemoryBus bus {atmega32u4};
    Timer10 timer4 {"TIMER4", atmega32u4.timers10[0]};
    timer4.set_bus(bus);
    bus.attach_peripheral(timer4);
    
    AvrCpu cpu {bus};
    auto cpu_ctrl = std::make_shared<CpuControl>(cpu, atmega32u4);
    bus.attach_peripheral(*cpu_ctrl);

    vioavr::core::PinMux pm_portc { 10 }; GpioPort portc { "PORTC", 0x26, 0x27, 0x28, pm_portc }; // PC7=OC4A, PC6=~OC4A
    bus.attach_peripheral(portc);
    timer4.connect_compare_output_a(portc, 7);
    timer4.connect_compare_output_a_inverted(portc, 6);

    timer4.reset();
    
    // Set Non-inverting PWM mode
    bus.write_data(atmega32u4.timers10[0].tccra_address, 0x82); // COM4A=2, PWM4A=1
    bus.write_data(atmega32u4.timers10[0].ocra_address, 50);
    bus.write_data(atmega32u4.timers10[0].ocrc_address, 100);
    
    // Set Dead-Time DT4 = 0x55 (5 cycles delay for both H and L)
    bus.write_data(atmega32u4.timers10[0].dt4_address, 0x55);

    // Set DDRC7=1, DDRC6=1
    bus.write_data(0x27, 0xC0);

    // 1. Hit Overflow (TCNT 99 -> 100 -> 0)
    timer4.write(atmega32u4.timers10[0].tcnt_address, 99);
    timer4.tick(1); 
    
    // At Overflow:
    // ~OC4A should go LOW immediately.
    // OC4A should stay LOW and only go HIGH after 5 cycles.
    CHECK((bus.read_data(0x28) & 0x40) == 0); // ~OC4A LOW
    CHECK((bus.read_data(0x28) & 0x80) == 0); // OC4A Still LOW (Dead-time)

    timer4.tick(4);
    CHECK((bus.read_data(0x28) & 0x80) == 0); // Still LOW after 4 cycles
    
    timer4.tick(1);
    CHECK((bus.read_data(0x28) & 0x80) != 0); // Now HIGH after 5 cycles

    // 2. Hit Compare Match (TCNT 49 -> 50)
    timer4.write(atmega32u4.timers10[0].tcnt_address, 49);
    timer4.tick(1);

    // At Match:
    // OC4A should go LOW immediately.
    // ~OC4A should go HIGH after 5 cycles.
    CHECK((bus.read_data(0x28) & 0x80) == 0); // OC4A LOW
    CHECK((bus.read_data(0x28) & 0x40) == 0); // ~OC4A Still LOW (Dead-time)

    timer4.tick(5);
    CHECK((bus.read_data(0x28) & 0x40) != 0); // Now HIGH
}
