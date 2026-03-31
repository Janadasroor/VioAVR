#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/devices/atmega328.hpp"

TEST_CASE("Timer0 External Clock Logic Test")
{
    using vioavr::core::GpioPort;
    using vioavr::core::MemoryBus;
    using vioavr::core::Timer8;
    using vioavr::core::devices::atmega328;

    constexpr auto pinb = static_cast<vioavr::core::u16>(0x23U);
    constexpr auto ddrb = static_cast<vioavr::core::u16>(0x24U);
    constexpr auto portb = static_cast<vioavr::core::u16>(0x25U);
    
    MemoryBus bus {atmega328};
    // Standard T0 pin for ATmega328 is PIND4 (0x10)
    GpioPort port_d {"PORTD", atmega328.timer0.t0_pin_address, 0x11U, 0x12U};
    Timer8 timer0 {"TIMER0", atmega328};
    
    bus.attach_peripheral(port_d);
    bus.attach_peripheral(timer0);
    timer0.set_bus(bus);
    bus.reset();

    SUBCASE("Rising Edge External Clock (CS0[2:0]=111)") {
        bus.write_data(atmega328.timer0.ocra_address, 0x02U);
        bus.write_data(atmega328.timer0.tccrb_address, 0x07U); // clk_T0 rising edge
        
        CHECK(timer0.running());
        CHECK(bus.read_data(atmega328.timer0.tcnt_address) == 0U);

        port_d.set_input_levels(0x00U);
        timer0.tick(1U); // Sample initial state
        port_d.set_input_levels(1U << atmega328.timer0.t0_pin_bit); // Rising edge
        timer0.tick(1U);
        
        CHECK(bus.read_data(atmega328.timer0.tcnt_address) == 1U);
    }

    SUBCASE("Falling Edge External Clock (CS0[2:0]=110)") {
        bus.write_data(atmega328.timer0.tccrb_address, 0x06U); // clk_T0 falling edge
        port_d.set_input_levels(1U << atmega328.timer0.t0_pin_bit);
        timer0.tick(1U); // Sample initial state
        port_d.set_input_levels(0x00U); // Falling edge
        timer0.tick(1U);
        
        CHECK(bus.read_data(atmega328.timer0.tcnt_address) == 1U);
    }
}
