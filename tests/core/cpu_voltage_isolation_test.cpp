#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_change_interrupt.hpp"
#include "vioavr/core/devices/atmega328.hpp"

using namespace vioavr::core;

TEST_CASE("Pin Change Voltage Isolation Test")
{
    using vioavr::core::GpioPort;
    using vioavr::core::MemoryBus;
    using vioavr::core::PinChangeInterrupt;
    using vioavr::core::devices::atmega328;

    constexpr auto pinb = static_cast<u16>(0x23U);
    constexpr auto ddrb = static_cast<u16>(0x24U);
    constexpr auto portb = static_cast<u16>(0x25U);

    MemoryBus bus {atmega328};
    GpioPort port_b {"PORTB", pinb, ddrb, portb};
    PinChangeInterruptDescriptor pci0_desc { .pcicr_address = 0x68U, .pcifr_address = 0x3BU, .pcmsk_address = 0x6BU, .pcicr_enable_mask = 0x01U, .pcifr_flag_mask = 0x01U, .vector_index = 3U };
    PinChangeInterrupt pci0 {"PCINT0", pci0_desc, port_b};

    bus.attach_peripheral(port_b);
    bus.attach_peripheral(pci0);
    bus.reset();

    // Setup
    bus.write_data(atmega328.pcints[0].pcmsk_address, 0x04U);
    bus.write_data(atmega328.pcints[0].pcicr_address, 0x01U);

    // Initial state: check pcifr is cleared
    CHECK(bus.read_data(atmega328.pcints[0].pcifr_address) == 0x00U);

    // Set voltage LOW, tick
    port_b.set_input_voltage(2U, 0.2);
    bus.tick_peripherals(1U);
    CHECK(bus.read_data(atmega328.pcints[0].pcifr_address) == 0x00U);

    // Set voltage still LOW (hysteresis), tick
    port_b.set_input_voltage(2U, 0.4);
    bus.tick_peripherals(1U);
    CHECK(bus.read_data(atmega328.pcints[0].pcifr_address) == 0x00U);

    // Set voltage HIGH, tick - should trigger
    port_b.set_input_voltage(2U, 0.8);
    bus.tick_peripherals(1U);
    CHECK(bus.read_data(atmega328.pcints[0].pcifr_address) == 0x01U);
}
