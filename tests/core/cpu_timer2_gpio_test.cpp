#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

TEST_CASE("Timer2 compare output can be wired from descriptor pin metadata")
{
    using vioavr::core::GpioPort;
    using vioavr::core::MemoryBus;
    using vioavr::core::Timer8;
    using vioavr::core::devices::atmega328p;

    GpioPort port_b {"PORTB", 0x23U, 0x24U, 0x25U};
    GpioPort port_d {"PORTD", 0x29U, 0x2AU, 0x2BU};
    MemoryBus bus {atmega328p};
    Timer8 timer2 {"TIMER2", atmega328p.timers8[1]};

    timer2.set_bus(bus);
    timer2.connect_compare_output_a(port_b, atmega328p.timers8[1].ocra_pin_bit);
    timer2.connect_compare_output_b(port_d, atmega328p.timers8[1].ocrb_pin_bit);

    bus.attach_peripheral(port_b);
    bus.attach_peripheral(port_d);
    bus.attach_peripheral(timer2);
    bus.reset();

    const auto oc2a_mask = static_cast<vioavr::core::u8>(1U << atmega328p.timers8[1].ocra_pin_bit);
    bus.write_data(port_b.port_address() - 1U, oc2a_mask); // DDRB
    bus.write_data(atmega328p.timers8[1].ocra_address, 0x02U);
    bus.write_data(atmega328p.timers8[1].tccra_address, 0x42U); // COM2A toggle + WGM21 CTC
    bus.write_data(atmega328p.timers8[1].tccrb_address, 0x01U); // clk/1

    CHECK((bus.read_data(port_b.port_address()) & oc2a_mask) == 0U);
    bus.tick_peripherals(2U);
    CHECK((bus.read_data(port_b.port_address()) & oc2a_mask) != 0U);
}
