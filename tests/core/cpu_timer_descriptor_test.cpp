#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/devices/atmega328.hpp"

TEST_CASE("Timer0 Descriptor and Basic Simulation Test")
{
    using vioavr::core::MemoryBus;
    using vioavr::core::Timer8;
    using vioavr::core::devices::atmega328;

    MemoryBus bus {atmega328};
    Timer8 timer0 {"TIMER0", atmega328.timers8[0]};
    bus.attach_peripheral(timer0);
    bus.reset();

    bus.write_data(atmega328.timers8[0].ocra_address, 0x02U);
    bus.write_data(atmega328.timers8[0].tccrb_address, 0x01U); // Start clk/1
    bus.tick_peripherals(2U);

    SUBCASE("Check compare and control registers") {
        CHECK(timer0.compare_a() == 0x02U);
        CHECK(timer0.control_b() == 0x01U);
        CHECK(bus.read_data(atmega328.timers8[0].tcnt_address) == 0x02U);
    }

    SUBCASE("Check interrupt flags and clearing") {
        CHECK(timer0.interrupt_flags() == 0x02U); // OCF0A set
        bus.write_data(atmega328.timers8[0].tifr_address, 0x02U); // Clear by writing 1
        CHECK(timer0.interrupt_flags() == 0x00U);
    }
}
