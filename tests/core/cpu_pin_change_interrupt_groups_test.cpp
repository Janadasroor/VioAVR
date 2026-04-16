#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_change_interrupt.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

using namespace vioavr::core;

TEST_CASE("Pin change interrupt groups share PCICR/PCIFR and keep separate PCMSK registers")
{
    using vioavr::core::devices::atmega328p;

    MemoryBus bus {atmega328p};
    GpioPort port_b {"PORTB", 0x23U, 0x24U, 0x25U};
    GpioPort port_c {"PORTC", 0x26U, 0x27U, 0x28U};
    GpioPort port_d {"PORTD", 0x29U, 0x2AU, 0x2BU};
    PinChangeInterruptSharedState shared {};
    PinChangeInterrupt pci0 {"PCINT0", atmega328p.pcints[0], port_b, shared, true};
    PinChangeInterrupt pci1 {"PCINT1", atmega328p.pcints[1], port_c, shared, false};
    PinChangeInterrupt pci2 {"PCINT2", atmega328p.pcints[2], port_d, shared, false};

    bus.attach_peripheral(port_b);
    bus.attach_peripheral(port_c);
    bus.attach_peripheral(port_d);
    bus.attach_peripheral(pci0);
    bus.attach_peripheral(pci1);
    bus.attach_peripheral(pci2);
    bus.reset();

    SUBCASE("shared control registers fan out to all groups")
    {
        bus.write_data(0x68U, 0x06U); // PCIE1 | PCIE2
        CHECK(bus.read_data(0x68U) == 0x06U);

        bus.write_data(0x6CU, 0x04U); // PCMSK1
        bus.write_data(0x6DU, 0x08U); // PCMSK2
        CHECK(bus.read_data(0x6CU) == 0x04U);
        CHECK(bus.read_data(0x6DU) == 0x08U);

        port_c.set_input_levels(0x00U);
        port_d.set_input_levels(0x00U);
        bus.tick_peripherals(1U);
        port_c.set_input_levels(0x04U);
        port_d.set_input_levels(0x08U);
        bus.tick_peripherals(1U);

        CHECK((bus.read_data(0x3BU) & 0x06U) == 0x06U);
    }

    SUBCASE("clearing PCIFR through the shared mapping clears group flags")
    {
        bus.write_data(0x68U, 0x02U); // PCIE1
        bus.write_data(0x6CU, 0x01U); // PCINT8 / PC0

        port_c.set_input_levels(0x00U);
        bus.tick_peripherals(1U);
        port_c.set_input_levels(0x01U);
        bus.tick_peripherals(1U);

        CHECK((bus.read_data(0x3BU) & 0x02U) == 0x02U);
        bus.write_data(0x3BU, 0x02U);
        CHECK((bus.read_data(0x3BU) & 0x02U) == 0x00U);
    }
}
