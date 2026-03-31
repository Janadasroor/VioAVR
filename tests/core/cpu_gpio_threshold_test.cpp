#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega328.hpp"

TEST_CASE("GPIO Threshold and Hysteresis Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    constexpr auto pinb_addr = static_cast<u16>(0x23U);
    constexpr auto ddrb_addr = static_cast<u16>(0x24U);
    constexpr auto portb_addr = static_cast<u16>(0x25U);

    MemoryBus bus {atmega328};
    GpioPort port_b {"PORTB", pinb_addr, ddrb_addr, portb_addr};
    bus.attach_peripheral(port_b);
    bus.reset();

    SUBCASE("Hysteresis logic for PB0") {
        // Initial state: Low (0.2V)
        port_b.set_input_voltage(0U, 0.2);
        CHECK((bus.read_data(pinb_addr) & 0x01U) == 0U);

        // Transition towards High, but below upper threshold (0.6V)
        port_b.set_input_voltage(0U, 0.5);
        CHECK((bus.read_data(pinb_addr) & 0x01U) == 0U);

        // Past upper threshold -> High
        port_b.set_input_voltage(0U, 0.7);
        CHECK((bus.read_data(pinb_addr) & 0x01U) != 0U);

        // Transition towards Low, but above lower threshold (0.3V)
        port_b.set_input_voltage(0U, 0.5);
        CHECK((bus.read_data(pinb_addr) & 0x01U) != 0U);

        // Below lower threshold -> Low
        port_b.set_input_voltage(0U, 0.2);
        CHECK((bus.read_data(pinb_addr) & 0x01U) == 0U);
    }
}
