#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

TEST_CASE("Timer16 Fidelity: Fast PWM (Mode 14, ICR1 as TOP)")
{
    MemoryBus bus {atmega328p};
    Timer16 timer1 {"TIMER1", atmega328p.timers16[0]};
    timer1.set_bus(bus);
    bus.attach_peripheral(timer1);

    vioavr::core::PinMux pm {10};
    GpioPort port {"PORTB", 0x23, 0x24, 0x25, pm};
    bus.attach_peripheral(port);
    timer1.connect_compare_output_a(port, 1); // OC1A
    
    port.write(0x24, 0x02); // DDRB

    bus.write_data(atmega328p.timers16[0].tccra_address, 0x82U);
    bus.write_data(atmega328p.timers16[0].tccrb_address, 0x19U);
    bus.write_data(atmega328p.timers16[0].icr_address, 0x04);
    bus.write_data(atmega328p.timers16[0].ocra_address, 0x02);

    auto get_oc1a = [&]() { return (port.read(0x25) >> 1) & 0x01; };

    // Stabilize
    for(int i=0; i<20; ++i) bus.tick_peripherals(1);
    while(timer1.counter() != 0) bus.tick_peripherals(1);
    
    // At TCNT=0, handle_matches(0) hasn't run yet for THIS cycle.
    // Let's tick once to process TCNT=0.
    bus.tick_peripherals(1); // Process 0, TCNT becomes 1. 
                            // Mode 14 sets at TOP (4), so at 0 it remains 1.
    CHECK(get_oc1a() == 1);
    
    bus.tick_peripherals(1); // Process 1, TCNT becomes 2.
    CHECK(get_oc1a() == 1);
    
    bus.tick_peripherals(1); // Process 2, TCNT becomes 3. Match! Clear.
    CHECK(get_oc1a() == 0);
    
    bus.tick_peripherals(1); // Process 3, TCNT becomes 4.
    CHECK(get_oc1a() == 0);
    
    bus.tick_peripherals(1); // Process 4, TCNT becomes 0. Match TOP! Set.
    CHECK(get_oc1a() == 1);
}

TEST_CASE("Timer16 Fidelity: Phase Correct PWM (Mode 1)")
{
    MemoryBus bus {atmega328p};
    Timer16 timer1 {"TIMER1", atmega328p.timers16[0]};
    timer1.set_bus(bus);
    bus.attach_peripheral(timer1);

    vioavr::core::PinMux pm {10};
    GpioPort port {"PORTB", 0x23, 0x24, 0x25, pm};
    bus.attach_peripheral(port);
    timer1.connect_compare_output_a(port, 1);
    port.write(0x24, 0x02); // DDRB

    bus.write_data(atmega328p.timers16[0].tccra_address, 0x81U);
    bus.write_data(atmega328p.timers16[0].tccrb_address, 0x01U);
    bus.write_data(atmega328p.timers16[0].ocra_address, 0x80U);

    auto get_oc1a = [&]() { return (port.read(0x25) >> 1) & 0x01; };

    for(int i=0; i<1000; ++i) bus.tick_peripherals(1);
    while(timer1.counter() != 0) bus.tick_peripherals(1);
    
    // Process TCNT=0 (BOTTOM)
    bus.tick_peripherals(1); 
    CHECK(get_oc1a() == 1);
    
    while(timer1.counter() != 128) bus.tick_peripherals(1);
    // Process TCNT=128 while up-counting
    bus.tick_peripherals(1);
    CHECK(get_oc1a() == 0);
    
    while(timer1.counter() != 128) bus.tick_peripherals(1);
    // Process TCNT=128 while down-counting
    bus.tick_peripherals(1);
    CHECK(get_oc1a() == 1);
}

TEST_CASE("Timer16 Fidelity: Input Capture Noise Canceler")
{
    MemoryBus bus {atmega328p};
    Timer16 timer1 {"TIMER1", atmega328p.timers16[0]};
    timer1.set_bus(bus);
    bus.attach_peripheral(timer1);

    vioavr::core::PinMux pm {10};
    GpioPort port {"PORTB", 0x23, 0x24, 0x25, pm};
    bus.attach_peripheral(port);
    timer1.connect_input_capture(port, 0);
    
    // TCCR1B = 0xC1 (ICNC=1, ICES=1, CS=1) -> Rising Edge
    bus.write_data(atmega328p.timers16[0].tccrb_address, 0xC1U);
    
    port.set_input_levels(0x00);
    for(int i=0; i<10; ++i) bus.tick_peripherals(1);
    
    port.set_input_levels(0x01); // Pulse
    bus.tick_peripherals(1); // Sample 1
    CHECK(timer1.input_capture() == 0);
    bus.tick_peripherals(1); // Sample 2
    CHECK(timer1.input_capture() == 0);
    bus.tick_peripherals(1); // Sample 3
    CHECK(timer1.input_capture() == 0);
    
    bus.tick_peripherals(1); // Sample 4! Trigger
    CHECK(timer1.input_capture() > 0);
}
