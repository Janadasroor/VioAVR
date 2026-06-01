#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/attiny85.hpp"

using namespace vioavr::core;

TEST_CASE("ATtiny85 — GPIO toggle via PIN register") {
    auto machine = Machine::create_for_device("ATtiny85");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();
    const auto& d = bus.device();

    bus.write_data(d.ports[0].ddr_address, 0x01U);
    bus.write_data(d.ports[0].port_address, 0x00U);
    CHECK(bus.read_data(d.ports[0].port_address) == 0x00U);

    bus.write_data(d.ports[0].pin_address, 0x01U);
    CHECK(bus.read_data(d.ports[0].port_address) == 0x01U);

    bus.write_data(d.ports[0].pin_address, 0x01U);
    CHECK(bus.read_data(d.ports[0].port_address) == 0x00U);
}

TEST_CASE("ATtiny85 — Timer0 counts") {
    auto machine = Machine::create_for_device("ATtiny85");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();
    const auto& d = bus.device();
    const auto& t0 = d.timers8[0];

    bus.write_data(t0.tcnt_address, 0x00U);
    bus.write_data(t0.tccrb_address, 0x01U);

    bus.tick_peripherals(10);

    CHECK(bus.read_data(t0.tcnt_address) >= 5U);
    CHECK((bus.read_data(t0.tifr_address) & 0x02U) == 0U); // no overflow yet
}

TEST_CASE("ATtiny85 — Timer0 overflow") {
    auto machine = Machine::create_for_device("ATtiny85");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();
    const auto& d = bus.device();
    const auto& t0 = d.timers8[0];

    bus.write_data(t0.tcnt_address, 0xFEU);
    bus.write_data(t0.tccrb_address, 0x01U);

    bus.tick_peripherals(5);

    CHECK((bus.read_data(t0.tifr_address) & 0x02U) != 0U);
}

TEST_CASE("ATtiny85 — EEPROM read/write") {
    auto machine = Machine::create_for_device("ATtiny85");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();
    const auto& d = bus.device();

    bus.write_data(0x3EU, 0x10U);
    bus.write_data(0x3DU, 0x42U);
    bus.write_data(0x3CU, 0x06U);
    bus.tick_peripherals(10);

    bus.write_data(0x3EU, 0x10U);
    bus.write_data(0x3CU, 0x01U);
    CHECK(bus.read_data(0x3DU) == 0x42U);
}
