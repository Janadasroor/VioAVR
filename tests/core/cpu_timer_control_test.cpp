#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

TEST_CASE("Timer0 Control and Prescaler Test")
{
    using vioavr::core::InterruptRequest;
    using vioavr::core::MemoryBus;
    using vioavr::core::Timer8;
    using vioavr::core::devices::atmega328p;

    MemoryBus bus {atmega328p};
    Timer8 timer0 {"TIMER0", atmega328p.timers8[0]};
    bus.attach_peripheral(timer0);
    bus.reset();

    SUBCASE("Initial State") {
        CHECK_FALSE(timer0.running());
        CHECK(bus.read_data(atmega328p.timers8[0].tcnt_address) == 0U);
        CHECK(bus.read_data(atmega328p.timers8[0].tccra_address) == 0U);
        CHECK(bus.read_data(atmega328p.timers8[0].tccrb_address) == 0U);
    }

    SUBCASE("Prescaler 8 (CS0[2:0]=010)") {
        bus.write_data(atmega328p.timers8[0].ocra_address, 0x02U);
        bus.write_data(atmega328p.timers8[0].tccrb_address, 0x02U); // clk/8
        
        CHECK(timer0.running());
        CHECK(timer0.control_b() == 0x02U);

        bus.tick_peripherals(7U); // Not enough for first increment
        CHECK(bus.read_data(atmega328p.timers8[0].tcnt_address) == 0U);

        bus.tick_peripherals(1U); // 8th tick -> TCNT=1
        CHECK(bus.read_data(atmega328p.timers8[0].tcnt_address) == 0x01U);

        bus.tick_peripherals(8U); // 16th tick -> TCNT=2 (Matches OCR0)
        CHECK(bus.read_data(atmega328p.timers8[0].tcnt_address) == 0x02U);
        CHECK(timer0.interrupt_flags() == 0x02U); // OCF0A set
    }

    SUBCASE("CTC Mode and Prescaler 32 (CS0[2:0]=011 -> wait, 011 is clk/64)") {
        // ATmega328 Timer0 CS bits: 001=1, 010=8, 011=64, 100=256, 101=1024
        // The old test used 0x03 for "32", but that's not standard for Timer0.
        // Let's use 0x03 as clk/64.
        
        bus.write_data(atmega328p.timers8[0].tccra_address, 0x02U); // WGM01=1 (CTC)
        bus.write_data(atmega328p.timers8[0].tccrb_address, 0x03U); // clk/64
        bus.write_data(atmega328p.timers8[0].ocra_address, 0x01U);
        bus.write_data(atmega328p.timers8[0].tcnt_address, 0x00U);
        
        // Clear flag
        bus.write_data(atmega328p.timers8[0].tifr_address, 0x02U);

        bus.tick_peripherals(63U);
        CHECK(bus.read_data(atmega328p.timers8[0].tcnt_address) == 0x00U);
        CHECK(timer0.interrupt_flags() == 0x00U);

        bus.tick_peripherals(1U); // 64th tick -> TCNT=1, matches OCRA
        CHECK(bus.read_data(atmega328p.timers8[0].tcnt_address) == 0x01U);
        CHECK((timer0.interrupt_flags() & 0x02U) != 0U);

        bus.tick_peripherals(64U); // Next 64 ticks -> TCNT clears to 0
        CHECK(bus.read_data(atmega328p.timers8[0].tcnt_address) == 0x00U);
    }
}
