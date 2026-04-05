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
    Timer8 timer2 {"TIMER2", atmega328p.timer2};
    bus.attach_peripheral(timer2);
    bus.reset();

    bus.write_data(atmega328p.timer2.assr_address, 0x20U); // AS2
    CHECK(timer2.async_status() == 0x20U);

    bus.write_data(atmega328p.timer2.tcnt_address, 0x11U);
    bus.write_data(atmega328p.timer2.ocra_address, 0x22U);
    bus.write_data(atmega328p.timer2.ocrb_address, 0x33U);
    bus.write_data(atmega328p.timer2.tccra_address, 0x02U);
    bus.write_data(atmega328p.timer2.tccrb_address, 0x01U);

    CHECK((timer2.async_status() & 0x1FU) == 0x1FU);

    bus.tick_peripherals(1U);
    CHECK(timer2.async_status() == 0x20U);
}
