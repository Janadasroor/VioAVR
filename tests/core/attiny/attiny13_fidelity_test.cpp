#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/attiny13.hpp"

using namespace vioavr::core;

TEST_CASE("ATtiny13 — GPIO basic output") {
    auto machine = Machine::create_for_device("ATtiny13");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();
    auto& mux = machine->pin_mux();
    const auto& d = bus.device();

    bus.write_data(d.ports[0].ddr_address, 0x01U);
    bus.write_data(d.ports[0].port_address, 0x01U);

    auto state = mux.get_state(1, 0);
    CHECK(state.is_output);
    CHECK(state.drive_level);
}

TEST_CASE("ATtiny13 — Timer8 counts + overflow") {
    auto machine = Machine::create_for_device("ATtiny13");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();
    const auto& d = bus.device();
    const auto& t0 = d.timers8[0];

    bus.write_data(t0.tcnt_address, 0xFEU);
    bus.write_data(t0.tccrb_address, 0x01U);
    bus.tick_peripherals(5);

    CHECK((bus.read_data(t0.tifr_address) & 0x02U) != 0U); // TOV0 = bit 1
}

TEST_CASE("ATtiny13 — ADC conversion") {
    auto machine = Machine::create_for_device("ATtiny13");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();
    const auto& d = bus.device();

    bus.write_data(0x27U, 0x00U);
    bus.write_data(0x26U, 0xC0U);
    for (int i = 0; i < 50; ++i)
        bus.tick_peripherals(10);

    CHECK((bus.read_data(0x26U) & 0x10U) != 0U);
    CHECK((bus.read_data(0x26U) & 0x40U) == 0U);
}
