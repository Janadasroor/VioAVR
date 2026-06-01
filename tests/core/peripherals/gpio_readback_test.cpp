#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/pin_mux.hpp"
#include <cstdio>

namespace {
using namespace vioavr::core;
}

TEST_CASE("GPIO PIN readback with multiple output bits") {
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);

    auto ports = machine->peripherals_of_type<GpioPort>();
    REQUIRE(ports.size() >= 1);

    GpioPort* portb = nullptr;
    for (auto* p : ports) {
        if (p->name() == "PORTB") portb = p;
    }
    REQUIRE(portb != nullptr);

    const u16 PINB  = 0x23;
    const u16 DDRB  = 0x24;
    const u16 PORTB = 0x25;

    // Set DDRB=0x23 and PORTB=0x01 (like the firmware does)
    machine->bus().write_data(DDRB, 0x23);
    machine->bus().write_data(PORTB, 0x01);

    uint8_t ddrb = machine->bus().read_data(DDRB);
    uint8_t portb_val = machine->bus().read_data(PORTB);
    uint8_t pinb = machine->bus().read_data(PINB);

    printf("DDRB=0x%02X PORTB=0x%02X PINB=0x%02X\n", ddrb, portb_val, pinb);
    printf("Expected PINB=0x01 (only bit 0 should be high)\n");

    // Now try: DDRB=0x01 PORTB=0x01 (only bit 0 as output)
    machine->bus().write_data(DDRB, 0x01);
    machine->bus().write_data(PORTB, 0x01);

    ddrb = machine->bus().read_data(DDRB);
    portb_val = machine->bus().read_data(PORTB);
    pinb = machine->bus().read_data(PINB);

    printf("After reset: DDRB=0x%02X PORTB=0x%02X PINB=0x%02X\n", ddrb, portb_val, pinb);

    CHECK(pinb == 0x01);

    // Test: DDRB=0x23, PORTB=0x21 (pin 0 and pin 5 high)
    machine->bus().write_data(DDRB, 0x23);
    machine->bus().write_data(PORTB, 0x21);

    pinb = machine->bus().read_data(PINB);
    printf("DDRB=0x23 PORTB=0x21 PINB=0x%02X (expected 0x21)\n", pinb);
    CHECK(pinb == 0x21);
}
