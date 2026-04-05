#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/devices/atmega328.hpp"

TEST_CASE("Analog Signal to Timer0 External Clock Transition Test")
{
    using vioavr::core::AnalogSignalBank;
    using vioavr::core::GpioPort;
    using vioavr::core::MemoryBus;
    using vioavr::core::Timer8;
    using vioavr::core::devices::atmega328;

    constexpr auto pinb = static_cast<vioavr::core::u16>(0x23U);
    constexpr auto ddrb = static_cast<vioavr::core::u16>(0x24U);
    constexpr auto portb = static_cast<vioavr::core::u16>(0x25U);
    
    AnalogSignalBank signals;
    signals.set_voltage(0U, 0.20); // LOW

    MemoryBus bus {atmega328};
    GpioPort port_b {"PORTB", pinb, ddrb, portb};
    port_b.bind_input_signal(0U, signals, 0U); // PB0 bound to signal 0
    
    Timer8 timer0 {"TIMER0", atmega328.timer0};
    
    bus.attach_peripheral(port_b);
    bus.attach_peripheral(timer0);
    bus.reset();

    SUBCASE("Clock ticking on signal transition") {
        bus.write_data(atmega328.timer0.ocra_address, 0x02U);
        bus.write_data(atmega328.timer0.tccrb_address, 0x07U); // Rising edge clock
        
        CHECK(timer0.running());
        CHECK((port_b.sample_levels() & 0x01U) == 0U);

        // Transition LOW -> MID (0.50V) -> HIGH (0.80V)
        signals.set_voltage(0U, 0.50);
        bus.tick_peripherals(1U);
        // Hysteresis: still LOW
        CHECK((port_b.sample_levels() & 0x01U) == 0U);
        CHECK(bus.read_data(atmega328.timer0.tcnt_address) == 0x00U);

        signals.set_voltage(0U, 0.80);
        bus.tick_peripherals(1U);
        // Now HIGH -> Rising edge detected
        CHECK((port_b.sample_levels() & 0x01U) != 0U);
        
        // Note: connect_external_clock is currently a placeholder in Timer8.
        // If it were implemented, tcnt would be 1.
        // For now, we're just verifying the test compiles and runs with the new API.
        // The actual logic pass might require implementing the clock connection in Timer8.
        CHECK(bus.read_data(atmega328.timer0.tcnt_address) == 0x01U);
    }
}
