#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

TEST_CASE("GPIO Threshold and Hysteresis Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    constexpr auto pinb_addr = static_cast<u16>(0x23U);
    constexpr auto ddrb_addr = static_cast<u16>(0x24U);
    constexpr auto portb_addr = static_cast<u16>(0x25U);

    MemoryBus bus {atmega328p};
    vioavr::core::PinMux pm_port_b { 10 }; GpioPort port_b { "PORTB", pinb_addr, ddrb_addr, portb_addr, pm_port_b };
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

    SUBCASE("Dynamic Datasheet Thresholds based on DeviceDescriptor") {
        // Change factors on the device to: VIL = 0.4, VIH = 0.8
        bus.device().vil_factor = 0.4;
        bus.device().vih_factor = 0.8;

        // Initial state: Low (0.2V)
        port_b.set_input_voltage(0U, 0.2);
        CHECK((bus.read_data(pinb_addr) & 0x01U) == 0U);

        // Transition towards High, below the new upper threshold (0.8V) - e.g., 0.7V
        port_b.set_input_voltage(0U, 0.7);
        CHECK((bus.read_data(pinb_addr) & 0x01U) == 0U); // Should still be Low because VIH is now 0.8

        // Past new upper threshold -> High (0.85V)
        port_b.set_input_voltage(0U, 0.85);
        CHECK((bus.read_data(pinb_addr) & 0x01U) != 0U); // Now it's High

        // Transition towards Low, above the new lower threshold (0.4V) - e.g., 0.5V
        port_b.set_input_voltage(0U, 0.5);
        CHECK((bus.read_data(pinb_addr) & 0x01U) != 0U); // Should still be High because VIL is now 0.4

        // Below new lower threshold -> Low (0.35V)
        port_b.set_input_voltage(0U, 0.35);
        CHECK((bus.read_data(pinb_addr) & 0x01U) == 0U); // Now it's Low
    }
}
