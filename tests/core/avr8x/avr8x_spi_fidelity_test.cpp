#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/spi8x.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X SPI - Prescaler Fidelity") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // Find first SPI
    auto spi_list = machine.peripherals_of_type<Spi8x>();
    REQUIRE(!spi_list.empty());

    // SPI0: Base at 0x0820U? No, let's check.
    // In atmega4809.hpp: SPI0 CTRLA is 0x08C0U.
    const u16 spi0_ctrla = 0x08C0;
    const u16 spi0_intflags = 0x08C3;
    const u16 spi0_data = 0x08C4;

    // 1. Configure SPI Master, Prescaler 16 (PRESC = 0x01 at bits 2:1)
    // MASTER=1 (bit 5), PRESC=1 (0x01 << 1), ENABLE=1 (bit 0)
    // Value: 0x20 | 0x02 | 0x01 = 0x23
    bus.write_data(spi0_ctrla, 0x23);

    // Initial check
    CHECK((bus.read_data(spi0_intflags) & 0x80) == 0); // IF = 0

    // 2. Write data to trigger transfer
    bus.write_data(spi0_data, 0xAA);

    // Expected duration: 8 bits * 16 prescaler = 128 cycles.
    // Tick 120 cycles. IF should still be 0.
    bus.tick_peripherals(120);
    CHECK((bus.read_data(spi0_intflags) & 0x80) == 0);

    // Tick remaining duration
    bus.tick_peripherals(10);
    CHECK((bus.read_data(spi0_intflags) & 0x80) != 0); // IF = 1
}

TEST_CASE("AVR8X SPI - Double Speed (CLK2X) Fidelity") {
    auto device = DeviceCatalog::find("ATmega4809");
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    const u16 spi0_ctrla = 0x08C0;
    const u16 spi0_intflags = 0x08C3;
    const u16 spi0_data = 0x08C4;

    // MASTER=1, PRESC=1 (16), ENABLE=1, CLK2X=1 (bit 7)
    // Value: 0x80 | 0x20 | 0x02 | 0x01 = 0xA3
    bus.write_data(spi0_ctrla, 0xA3);

    // Expected duration: (8 * 16) / 2 = 64 cycles.
    bus.write_data(spi0_data, 0x55);

    bus.tick_peripherals(60);
    CHECK((bus.read_data(spi0_intflags) & 0x80) == 0);
    bus.tick_peripherals(10);
    CHECK((bus.read_data(spi0_intflags) & 0x80) != 0);
}
