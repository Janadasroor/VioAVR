#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/atmega328p.hpp"
#include <array>

using namespace vioavr::core;
using namespace vioavr::core::devices;

struct WgmCase {
    const char* name;
    u8 tccra; u8 tccrb;
    u16 top;
    bool phase_correct;
    u16 ocra; u16 icr;
};

TEST_CASE("Timer16 Fidelity: All 16 WGM modes") {
    MemoryBus bus {atmega328p};
    vioavr::core::PinMux pm {10};
    Timer16 timer1 {"TIMER1", atmega328p.timers16[0], &pm};
    timer1.set_bus(bus);
    bus.attach_peripheral(timer1);

    GpioPort port {"PORTB", 0x23, 0x24, 0x25, pm};
    bus.attach_peripheral(port);
    timer1.connect_compare_output_a(port, 1);
    port.write(0x24, 0x02);

    auto get_oc1a = [&]() { return (port.read(0x23) >> 1) & 0x01; };

    // wgm -> (tccra, tccrb, top, is_phase_correct, ocra, icr)
    // tccra = (wgm & 0x03), tccrb = ((wgm >> 2) << 3) | CS=1
    // We set ocra=64, icr=255 to have a well-defined top for variable-top modes
    std::array<WgmCase, 16> cases = {{
        {"WGM=0 (Normal)",           0x00, 0x01, 0xFFFF, false, 64, 255},
        {"WGM=1 (PC 8-bit)",         0x01, 0x01, 0x00FF, true,  64, 255},
        {"WGM=2 (PC 9-bit)",         0x02, 0x01, 0x01FF, true,  64, 255},
        {"WGM=3 (Fast 10-bit)",      0x03, 0x01, 0x03FF, false, 64, 255},
        {"WGM=4 (CTC OCRA)",         0x00, 0x09, 64,     false, 64, 255},
        {"WGM=5 (Fast 8-bit)",       0x01, 0x09, 0x00FF, false, 64, 255},
        {"WGM=6 (Fast 9-bit)",       0x02, 0x09, 0x01FF, false, 64, 255},
        {"WGM=7 (Fast 10-bit)",      0x03, 0x09, 0x03FF, false, 64, 255},
        {"WGM=8 (PFC ICR)",          0x00, 0x11, 255,    true,  64, 255},
        {"WGM=9 (PFC OCRA)",         0x01, 0x11, 64,     true,  64, 255},
        {"WGM=10 (PC ICR)",          0x02, 0x11, 255,    true,  64, 255},
        {"WGM=11 (PC OCRA)",         0x03, 0x11, 64,     true,  64, 255},
        {"WGM=12 (CTC ICR)",         0x00, 0x19, 255,    false, 64, 255},
        {"WGM=13 (Fast OCRA)",       0x01, 0x19, 64,     false, 64, 255},
        {"WGM=14 (Fast ICR)",        0x02, 0x19, 255,    false, 64, 255},
        {"WGM=15 (Fast OCRA)",       0x03, 0x19, 64,     false, 64, 255},
    }};

    for (auto& c : cases) {
        MESSAGE(c.name);
        timer1.reset();
        port.reset();

        port.write(0x24, 0x02); // DDRB bit 1
        bus.write_data(atmega328p.timers16[0].tccra_address, c.tccra);
        bus.write_data(atmega328p.timers16[0].icr_address + 1, 0);
        bus.write_data(atmega328p.timers16[0].icr_address,     c.icr & 0xFF);
        bus.write_data(atmega328p.timers16[0].ocra_address + 1, 0);
        bus.write_data(atmega328p.timers16[0].ocra_address,     c.ocra & 0xFF);
        bus.write_data(atmega328p.timers16[0].tccrb_address, c.tccrb);

        // For buffered modes where TOP=OCRA (PFC/PC), ocra_ starts at 0 until
        // the first buffer load (at BOTTOM). Preload via an unbuffered mode write
        // so the first period is correct.
        bool ocra_top = (c.top == c.ocra);
        if (ocra_top && (c.tccrb & 0x18) != 0) {
            u8 saved_tccra = c.tccra;
            bus.write_data(atmega328p.timers16[0].tccra_address, 0); // WGM=0 unbuffered
            bus.write_data(atmega328p.timers16[0].ocra_address + 1, 0);
            bus.write_data(atmega328p.timers16[0].ocra_address, c.ocra & 0xFF);
            bus.write_data(atmega328p.timers16[0].tccra_address, saved_tccra);
        }

        // Run enough ticks to see at least one full cycle
        u64 ticks_needed = static_cast<u64>(c.top) * 2 + 100;
        if (ticks_needed > 200000) ticks_needed = 200000;
        u16 max_seen = 0;
        u16 min_seen = 0xFFFF;
        bool saw_overflow = false;
        u16 prev_tov = 0;
        for (u64 i = 0; i < ticks_needed; ++i) {
            bus.tick_peripherals(1);
            u16 cnt = timer1.counter();
            max_seen = std::max(max_seen, cnt);
            min_seen = std::min(min_seen, cnt);

            u8 tifr = bus.read_data(atmega328p.timers16[0].tifr_address);
            if ((tifr & 0x01) && !prev_tov) saw_overflow = true;
            prev_tov = (tifr & 0x01);
        }

        INFO("max_seen=", max_seen, " top=", c.top);
        CHECK(max_seen <= c.top);
        INFO("saw_overflow=", saw_overflow);
        CHECK(saw_overflow);
    }
}

TEST_CASE("Timer16 Fidelity: Fast PWM (Mode 14, ICR1 as TOP)")
{
    MemoryBus bus {atmega328p};
    vioavr::core::PinMux pm {10};
    Timer16 timer1 {"TIMER1", atmega328p.timers16[0], &pm};
    timer1.set_bus(bus);
    bus.attach_peripheral(timer1);

    GpioPort port {"PORTB", 0x23, 0x24, 0x25, pm};
    bus.attach_peripheral(port);
    timer1.connect_compare_output_a(port, 1); // OC1A
    
    port.write(0x24, 0x02); // DDRB

    bus.write_data(atmega328p.timers16[0].tccra_address, 0x82U);
    bus.write_data(atmega328p.timers16[0].icr_address, 0x04);
    bus.write_data(atmega328p.timers16[0].ocra_address, 0x02);
    bus.write_data(atmega328p.timers16[0].tccrb_address, 0x19U);

    auto get_oc1a = [&]() { return (port.read(0x23) >> 1) & 0x01; };

    // Stabilize
    for(int i=0; i<20; ++i) bus.tick_peripherals(1);
    while(timer1.counter() != 0) bus.tick_peripherals(1);
    
    // TCNT was 0 from the loop end. handle_matches(0) just ran and set it to 1.
    CHECK(get_oc1a() == 1);

    bus.tick_peripherals(1); // TCNT: 0 -> 1. handle_matches(1) ran. Still 1.
    CHECK(get_oc1a() == 1);
    
    bus.tick_peripherals(1); // TCNT: 1 -> 2. handle_matches(2) ran. MATCH! Clear.
    CHECK(get_oc1a() == 0);
    
    bus.tick_peripherals(1); // TCNT: 2 -> 3.
    CHECK(get_oc1a() == 0);
    
    bus.tick_peripherals(1); // TCNT: 3 -> 4.
    CHECK(get_oc1a() == 0);
    
    bus.tick_peripherals(1); // TCNT: 4 -> 0. handle_matches(0) ran. SET!
    CHECK(get_oc1a() == 1);
}

TEST_CASE("Timer16 Fidelity: Phase Correct PWM (Mode 1)")
{
    MemoryBus bus {atmega328p};
    vioavr::core::PinMux pm {10};
    Timer16 timer1 {"TIMER1", atmega328p.timers16[0], &pm};
    timer1.set_bus(bus);
    bus.attach_peripheral(timer1);

    GpioPort port {"PORTB", 0x23, 0x24, 0x25, pm};
    bus.attach_peripheral(port);
    timer1.connect_compare_output_a(port, 1);
    port.write(0x24, 0x02); // DDRB

    bus.write_data(atmega328p.timers16[0].tccra_address, 0x81U);
    bus.write_data(atmega328p.timers16[0].tccrb_address, 0x01U);
    bus.write_data(atmega328p.timers16[0].ocra_address, 0x80U);

    auto get_oc1a = [&]() { return (port.read(0x23) >> 1) & 0x01; };

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
