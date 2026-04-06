#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

TEST_CASE("Timer2 Descriptor and Basic Simulation Test")
{
    using vioavr::core::MemoryBus;
    using vioavr::core::Timer8;
    using vioavr::core::devices::atmega328p;

    MemoryBus bus {atmega328p};
    Timer8 timer2 {"TIMER2", atmega328p.timer2};
    bus.attach_peripheral(timer2);
    bus.reset();

    SUBCASE("Check Timer2 registers are descriptor-backed") {
        bus.write_data(atmega328p.timer2.ocra_address, 0x02U);
        bus.write_data(atmega328p.timer2.assr_address, 0x20U);
        bus.write_data(atmega328p.timer2.tccrb_address, 0x01U); // clk/1

        CHECK(timer2.compare_a() == 0x02U);
        CHECK(timer2.control_b() == 0x01U);
        CHECK(timer2.async_status() == 0x20U); // AS2
        CHECK(bus.read_data(atmega328p.timer2.assr_address) == 0x20U);
    }

    SUBCASE("Check Timer2 interrupt flags and clearing") {
        bus.write_data(atmega328p.timer2.ocra_address, 0x02U);
        bus.write_data(atmega328p.timer2.tccrb_address, 0x01U); // clk/1
        bus.tick_peripherals(2U);

        CHECK(bus.read_data(atmega328p.timer2.tcnt_address) == 0x02U);
        CHECK(timer2.interrupt_flags() == 0x02U); // OCF2A
        bus.write_data(atmega328p.timer2.tifr_address, 0x02U);
        CHECK(timer2.interrupt_flags() == 0x00U);
    }
}
