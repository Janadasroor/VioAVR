#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/spi8x.hpp"

using namespace vioavr::core;

static constexpr u16 CTRLA    = 0x08C0;
static constexpr u16 CTRLB    = 0x08C1;
static constexpr u16 INTCTRL  = 0x08C2;
static constexpr u16 INTFLAGS = 0x08C3;
static constexpr u16 DATA     = 0x08C4;

TEST_CASE("SPI8X — Reset defaults") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    CHECK(bus.read_data(CTRLA) == 0x00);
    CHECK(bus.read_data(CTRLB) == 0x00);
    CHECK(bus.read_data(INTCTRL) == 0x00);
    CHECK(bus.read_data(INTFLAGS) == 0x00);
    CHECK(bus.read_data(DATA) == 0x00);
}

TEST_CASE("SPI8X — Register round-trips") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    bus.write_data(CTRLA, 0xA7);
    CHECK(bus.read_data(CTRLA) == 0xA7);

    bus.write_data(CTRLB, 0x55);
    CHECK(bus.read_data(CTRLB) == 0x55);

    bus.write_data(INTCTRL, 0x01);
    CHECK(bus.read_data(INTCTRL) == 0x01);
}

TEST_CASE("SPI8X — Disabled ignores data write") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    bus.write_data(DATA, 0xAA); // ENABLE=0
    bus.tick_peripherals(100);
    CHECK((bus.read_data(INTFLAGS) & 0x80) == 0); // No IF
    CHECK(bus.read_data(DATA) == 0x00); // DATA unchanged
}

TEST_CASE("SPI8X — CPOL=1 with slow prescaler") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // On AVR8X, bit 2 doubles as PRESC bit 1 and CPOL
    // PRESC=3(/128): bits 3:1=011 → CPOL=1 (bit2=1)
    // CTRLA = MASTER|PRESC3|ENABLE = 0x20|0x06|0x01 = 0x27
    bus.write_data(CTRLA, 0x27);
    bus.write_data(DATA, 0x55);
    // 8 bits * 128 = 1024 cycles
    bus.tick_peripherals(1050);
    CHECK((bus.read_data(INTFLAGS) & 0x80) != 0);
}

TEST_CASE("SPI8X — DORD LSB-first transfer") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // MASTER=1, PRESC=1(/16), ENABLE=1, DORD=1
    // DORD is bit 4: 0x10
    // CTRLA = 0x20 | 0x02 | 0x01 | 0x10 = 0x33
    bus.write_data(CTRLA, 0x33);
    bus.write_data(DATA, 0x55); // 01010101
    bus.tick_peripherals(128);
    CHECK((bus.read_data(INTFLAGS) & 0x80) != 0);
    CHECK(bus.read_data(DATA) != 0x55); // Different value when received back
}

TEST_CASE("SPI8X — Prescaler /128 timing") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // MASTER=1, PRESC=3(/128), ENABLE=1
    // presc=3 -> bits 3:1 = 0b011 -> CTRLA bits 3=0,2=1,1=1
    // CTRLA = 0x20 | 0x06 | 0x01 = 0x27
    bus.write_data(CTRLA, 0x27);
    // 8 bits * 128 = 1024 cycles
    bus.write_data(DATA, 0xAA);
    bus.tick_peripherals(1000);
    CHECK((bus.read_data(INTFLAGS) & 0x80) == 0);
    bus.tick_peripherals(50);
    CHECK((bus.read_data(INTFLAGS) & 0x80) != 0);
}

TEST_CASE("SPI8X — Prescaler /2 timing") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // MASTER=1, PRESC=4(/2), ENABLE=1
    // presc=4 -> bits 3:1 = 0b100 -> CTRLA bits 3=1,2=0,1=0
    // CTRLA = 0x20 | 0x08 | 0x01 = 0x29
    bus.write_data(CTRLA, 0x29);
    // 8 bits * 2 = 16 cycles
    bus.write_data(DATA, 0x55);
    bus.tick_peripherals(16);
    CHECK((bus.read_data(INTFLAGS) & 0x80) != 0);
}

TEST_CASE("SPI8X — WRCOL write collision flag") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    bus.write_data(CTRLA, 0x21); // Master, /4, Enable
    bus.write_data(DATA, 0xAA);
    bus.write_data(DATA, 0xBB); // Buffered
    bus.write_data(DATA, 0xCC); // WRCOL!
    CHECK((bus.read_data(INTFLAGS) & 0x40) != 0);

    // Clear by writing 1
    bus.write_data(INTFLAGS, 0x40);
    CHECK((bus.read_data(INTFLAGS) & 0x40) == 0);
}

TEST_CASE("SPI8X — Consume without pending returns false") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    auto spis = machine.peripherals_of_type<Spi8x>();
    InterruptRequest irq;
    CHECK(spis[0]->consume_interrupt_request(irq) == false);
}

TEST_CASE("SPI8X — Interrupt disabled with IF set") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    bus.write_data(CTRLA, 0x21); // Master, /4, Enable
    bus.write_data(DATA, 0xAA);
    bus.tick_peripherals(32);
    CHECK((bus.read_data(INTFLAGS) & 0x80) != 0);

    // INTCTRL.IE=0 → no pending interrupt despite IF
    InterruptRequest irq;
    auto spis = machine.peripherals_of_type<Spi8x>();
    CHECK(spis[0]->pending_interrupt_request(irq) == false);
}

TEST_CASE("SPI8X — Multiple transfers preserve INTFLAGS") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    bus.write_data(CTRLA, 0x21); // Master, /4, Enable
    bus.write_data(DATA, 0x11);
    bus.tick_peripherals(32);
    CHECK((bus.read_data(INTFLAGS) & 0x80) != 0);

    bus.read_data(DATA); // Clear IF

    bus.write_data(DATA, 0x22);
    bus.tick_peripherals(32);
    CHECK((bus.read_data(INTFLAGS) & 0x80) != 0);
}

TEST_CASE("SPI8X — DATA read returns received byte") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    bus.write_data(CTRLA, 0x21); // Master, /4, Enable
    bus.write_data(DATA, 0xAB);
    bus.tick_peripherals(32);
    CHECK((bus.read_data(INTFLAGS) & 0x80) != 0);
    // Reading DATA clears IF
    u8 rx = bus.read_data(DATA);
    CHECK((bus.read_data(INTFLAGS) & 0x80) == 0); // IF cleared by read
    (void)rx;
}

TEST_CASE("SPI8X — Unmapped addresses return 0") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    CHECK(bus.read_data(0x08C5) == 0);
    CHECK(bus.read_data(0x08C6) == 0);
    CHECK(bus.read_data(0x08BF) == 0);
}
