#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer10.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/devices/atmega32u4.hpp"

namespace {
using namespace vioavr::core;
using namespace vioavr::core::devices;
}

TEST_CASE("Timer10 Phase Correct PWM Fidelity")
{
    MemoryBus bus {atmega32u4};
    Timer10 timer4 {"TIMER4", atmega32u4.timers10[0]};
    timer4.set_bus(bus);
    bus.attach_peripheral(timer4);
    
    GpioPort portc {"PORTC", 0x26, 0x27, 0x28};
    bus.attach_peripheral(portc);
    timer4.connect_compare_output_a(portc, 7);
    bus.write_data(0x27, 0x80); // DDRC7 = 1

    timer4.reset();
    
    // Set OCR values first (while PWM is disabled, so they apply immediately)
    bus.write_data(atmega32u4.timers10[0].ocra_address, 10);
    bus.write_data(atmega32u4.timers10[0].ocrc_address, 20); // TOP 

    // Set Phase Correct PWM mode (WGM41:0 = 1 in TCCR4D)
    bus.write_data(atmega32u4.timers10[0].tccrd_address, 0x01);
    // COM4A = 2 (Clear on Match Up, Set on Match Down), PWM4A = 1
    bus.write_data(atmega32u4.timers10[0].tccra_address, 0x82);
    
    // 0 -> 1 ... -> 9
    timer4.tick(10);
    CHECK(timer4.read(atmega32u4.timers10[0].tcnt_address) == 10);
    // At match 10 while UP: Clear OC4A
    CHECK((bus.read_data(0x28) & 0x80) == 0); 
    
    // 10 -> 20 (TOP)
    timer4.tick(10);
    CHECK(timer4.read(atmega32u4.timers10[0].tcnt_address) == 20);
    CHECK((bus.read_data(0x28) & 0x80) == 0); 

    // Now it should start counting DOWN
    timer4.tick(5);
    CHECK(timer4.read(atmega32u4.timers10[0].tcnt_address) == 15);
    
    // Continue down to 10
    timer4.tick(5);
    CHECK(timer4.read(atmega32u4.timers10[0].tcnt_address) == 10);
    // At match 10 while DOWN: Set OC4A
    CHECK((bus.read_data(0x28) & 0x80) != 0); 

    // Continue down to 0
    timer4.tick(10);
    CHECK(timer4.read(atmega32u4.timers10[0].tcnt_address) == 0);
    // Hit Overflow at Bottom
    CHECK((bus.read_data(atmega32u4.timers10[0].tifr_address) & 0x04) != 0);
}
