#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/devices/attiny861.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

TEST_CASE("ATtiny861 — TC1H 16-bit TCNT1 access") {
    PinMux pm{6};
    Timer16 timer1{"TIMER1", attiny861.timers16[0], &pm};
    MemoryBus bus{attiny861};
    timer1.set_bus(bus);
    bus.attach_peripheral(timer1);

    CHECK(timer1.counter() == 0U);

    bus.write_data(attiny861.timers16[0].tc1h_address, 0x12U);
    bus.write_data(attiny861.timers16[0].tcnt_address, 0x34U);
    CHECK(timer1.counter() == 0x1234U);

    u8 low = bus.read_data(attiny861.timers16[0].tcnt_address);
    u8 high = bus.read_data(attiny861.timers16[0].tc1h_address);
    CHECK(low == 0x34U);
    CHECK(high == 0x12U);
}

TEST_CASE("ATtiny861 — TC1H 16-bit OCR1D access") {
    PinMux pm{6};
    Timer16 timer1{"TIMER1", attiny861.timers16[0], &pm};
    MemoryBus bus{attiny861};
    timer1.set_bus(bus);
    bus.attach_peripheral(timer1);

    bus.write_data(attiny861.timers16[0].tc1h_address, 0x0FU);
    bus.write_data(attiny861.timers16[0].ocrd_address, 0xC0U);

    u8 low = bus.read_data(attiny861.timers16[0].ocrd_address);
    u8 high = bus.read_data(attiny861.timers16[0].tc1h_address);
    CHECK(low == 0xC0U);
    CHECK(high == 0x0FU);
}

TEST_CASE("ATtiny861 — Timer1 counts via TCCRB") {
    auto machine = Machine::create_for_device("ATtiny861");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();
    const auto& d = bus.device();
    const auto& t1 = d.timers16[0];

    // Timer1 TCNT starts at 0
    bus.write_data(t1.tcnt_address, 0x00U);
    bus.write_data(t1.tc1h_address, 0x00U);
    bus.write_data(t1.tccrb_address, 0x01U); // CS=1

    bus.tick_peripherals(10);

    // TCNT should have incremented — read via TC1H
    u8 low = bus.read_data(t1.tcnt_address);
    u8 high = bus.read_data(t1.tc1h_address);
    u16 cnt = (static_cast<u16>(high) << 8) | low;
    CHECK(cnt >= 5U);
}

TEST_CASE("ATtiny861 — GPIO PORTB output") {
    auto machine = Machine::create_for_device("ATtiny861");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();
    auto& mux = machine->pin_mux();
    const auto& d = bus.device();

    bus.write_data(d.ports[1].ddr_address, 0x01U);
    bus.write_data(d.ports[1].port_address, 0x01U);

    auto state = mux.get_state(1, 0);
    CHECK(state.is_output);
    CHECK(state.drive_level);
}
