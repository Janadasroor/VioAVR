#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

TEST_CASE("Timer2 ASSR busy flags track async-domain register writes")
{
    using vioavr::core::MemoryBus;
    using vioavr::core::Timer8;
    using vioavr::core::devices::atmega328p;

    MemoryBus bus {atmega328p};
    Timer8 timer2 {"TIMER2", atmega328p.timers8[1]};
    bus.attach_peripheral(timer2);
    bus.reset();

    bus.write_data(atmega328p.timers8[1].assr_address, 0x20U); // AS2
    CHECK(timer2.async_status() == 0x20U);

    bus.write_data(atmega328p.timers8[1].tcnt_address, 0x11U);
    bus.write_data(atmega328p.timers8[1].ocra_address, 0x22U);
    bus.write_data(atmega328p.timers8[1].tccrb_address, 0x01U);

    // Verify all relevant flags are set
    CHECK((timer2.async_status() & atmega328p.timers8[1].tcn2ub_mask));
    CHECK((timer2.async_status() & atmega328p.timers8[1].ocr2aub_mask));
    CHECK((timer2.async_status() & atmega328p.timers8[1].tcr2bub_mask));

    // After 10 cycles, they should still be set
    bus.tick_peripherals(10U);
    CHECK((timer2.async_status() & atmega328p.timers8[1].tcn2ub_mask));

    // After 600 cycles, they should be synchronized (cleared)
    bus.tick_peripherals(600U);
    CHECK((timer2.async_status() & 0x7U) == 0U); // Busy bits cleared
    CHECK((timer2.async_status() & 0x20U) == 0x20U); // AS2 stayed set
}
