#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/memory_bus.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X SPI Fidelity Test") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    REQUIRE(device->spi8x_count >= 1);

    Machine machine(*device);
    auto& bus = machine.bus();

    const u16 SPI0_BASE = 0x08C0;
    const u16 SPI0_CTRLA    = SPI0_BASE + 0x00;
    const u16 SPI0_CTRLB    = SPI0_BASE + 0x01;
    const u16 SPI0_INTCTRL  = SPI0_BASE + 0x02;
    const u16 SPI0_INTFLAGS = SPI0_BASE + 0x03;
    const u16 SPI0_DATA     = SPI0_BASE + 0x04;

    // 1. Enable and configure as Master
    bus.write_data(SPI0_CTRLA, 0x21); // MASTER=1, ENABLE=1
    CHECK(bus.read_data(SPI0_CTRLA) == 0x21);

    // 2. Start Transfer
    bus.write_data(SPI0_DATA, 0x55);
    CHECK((bus.read_data(SPI0_INTFLAGS) & 0x80) == 0); // IF=0 (busy)

    // 3. Tick (Default duration is 128 cycles for /4 prescaler-ish)
    // Wait, my impl uses /4 as default (index 0). 4*8 = 32 cycles?
    // Let's check my Spi8x::write logic.
    // presc 0 -> /4. divisor=4. duration=divisor*8=32.
    bus.tick_peripherals(20);
    CHECK((bus.read_data(SPI0_INTFLAGS) & 0x80) == 0);

    bus.tick_peripherals(15);
    CHECK((bus.read_data(SPI0_INTFLAGS) & 0x80) == 0x80); // Done!

    // 4. Read clears flag
    CHECK(bus.read_data(SPI0_DATA) == 0x55);
    CHECK((bus.read_data(SPI0_INTFLAGS) & 0x80) == 0);
}
