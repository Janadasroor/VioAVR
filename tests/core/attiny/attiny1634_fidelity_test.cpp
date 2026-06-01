#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/attiny1634.hpp"

using namespace vioavr::core;

TEST_CASE("ATtiny1634 — GPIO multi-port output") {
    auto machine = Machine::create_for_device("ATtiny1634");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();
    auto& mux = machine->pin_mux();
    const auto& d = bus.device();

    REQUIRE(d.port_count >= 3U);

    // PORTB: ports[0], name "PORTB" => port_idx = 1
    bus.write_data(d.ports[0].ddr_address, 0x01U);
    bus.write_data(d.ports[0].port_address, 0x01U);
    auto state = mux.get_state(1, 0);
    CHECK(state.is_output);
    CHECK(state.drive_level);

    // PORTC: ports[1], name "PORTC" => port_idx = 2
    bus.write_data(d.ports[1].ddr_address, 0x01U);
    bus.write_data(d.ports[1].port_address, 0x01U);
    state = mux.get_state(2, 0);
    CHECK(state.is_output);
    CHECK(state.drive_level);

    // PORTA: ports[2], name "PORTA" => port_idx = 0
    bus.write_data(d.ports[2].ddr_address, 0x01U);
    bus.write_data(d.ports[2].port_address, 0x01U);
    state = mux.get_state(0, 0);
    CHECK(state.is_output);
    CHECK(state.drive_level);
}

TEST_CASE("ATtiny1634 — USI register access") {
    auto machine = Machine::create_for_device("ATtiny1634");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();
    const auto& d = bus.device();

    // USICR=0x4A, USISR=0x4B, USIDR=0x4C, USIBR=0x4D
    bus.write_data(0x4AU, 0x10U);
    CHECK(bus.read_data(0x4AU) == 0x10U);
    CHECK(bus.read_data(0x4BU) == 0x00U);
    CHECK(bus.read_data(0x4CU) == 0x00U);
}
