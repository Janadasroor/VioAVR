#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

TEST_CASE("Timer2 uses a separate async tick domain when AS2 is enabled")
{
    using vioavr::core::MemoryBus;
    using vioavr::core::Timer8;
    using vioavr::core::devices::atmega328p;

    MemoryBus bus {atmega328p};
    Timer8 timer2 {"TIMER2", atmega328p.timers8[1]};
    bus.attach_peripheral(timer2);
    bus.reset();

    bus.write_data(atmega328p.timers8[1].tccrb_address, 0x01U); // clk/1

    SUBCASE("CPU-domain ticks do not advance Timer2 in async mode")
    {
        bus.write_data(atmega328p.timers8[1].assr_address, 0x20U); // AS2
        bus.tick_peripherals(5U);
        CHECK(bus.read_data(atmega328p.timers8[1].tcnt_address) == 0U);
    }

    SUBCASE("Explicit async ticks advance Timer2 in async mode")
    {
        bus.write_data(atmega328p.timers8[1].assr_address, 0x20U); // AS2
        timer2.tick_async(5U);
        CHECK(bus.read_data(atmega328p.timers8[1].tcnt_address) == 5U);
    }

    SUBCASE("Synchronous mode still advances from normal peripheral ticks")
    {
        bus.write_data(atmega328p.timers8[1].assr_address, 0x00U);
        bus.tick_peripherals(5U);
        CHECK(bus.read_data(atmega328p.timers8[1].tcnt_address) == 5U);
    }
}
