#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/ext_interrupt.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_change_interrupt.hpp"
#include "vioavr/core/devices/atmega328.hpp"

using namespace vioavr::core;
void step_to(AvrCpu& cpu, u32 target_pc) { while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) { cpu.step(); } }
TEST_CASE("Analog-Digital Frontend Integration Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    constexpr auto pinb = static_cast<u16>(0x23U);
    constexpr auto ddrb = static_cast<u16>(0x24U);
    constexpr auto portb = static_cast<u16>(0x25U);
    
    AnalogSignalBank signals;
    signals.set_voltage(0U, 0.20); // LOW
    signals.set_voltage(1U, 0.80); // HIGH

    MemoryBus bus {atmega328};
    GpioPort port_b {"PORTB", pinb, ddrb, portb};
    port_b.bind_input_signal(2U, signals, 0U); // PB2 bound to signal 0 (0.20V)
    
    ExtInterrupt exti {"EXTINT", atmega328, 4U};
    exti.bind_int0_signal(signals, 1U); // INT0 bound to signal 1 (0.80V)
    
    PinChangeInterruptDescriptor pci0_desc { .pcicr_address = 0x68U, .pcifr_address = 0x3BU, .pcmsk_address = 0x6BU, .pcicr_enable_mask = 0x01U, .pcifr_flag_mask = 0x01U, .vector_index = 3U };
    PinChangeInterrupt pci0 {"PCINT0", pci0_desc, port_b};

    bus.attach_peripheral(port_b);
    bus.attach_peripheral(exti);
    bus.attach_peripheral(pci0);
    bus.reset();

    // Configure interrupts
    bus.write_data(atmega328.ext_interrupt.eicra_address, 0x02U);          // INT0 falling edge
    bus.write_data(atmega328.ext_interrupt.eimsk_address, 0x01U);          // INT0 enable
    bus.write_data(atmega328.pin_change_interrupt_0.pcicr_address, 0x01U); // PCIE0 enable
    bus.write_data(atmega328.pin_change_interrupt_0.pcmsk_address, 0x04U); // PCINT2 (PB2) enable

    SUBCASE("Initial State") {
        CHECK((port_b.sample_levels() & 0x04U) == 0U);
        InterruptRequest request {};
        CHECK_FALSE(bus.pending_interrupt_request(request));
    }

    SUBCASE("Signal transitions and triggers") {
        // Step 2: Set signals to 0.50 (intermediate/hysteresis)
        signals.set_voltage(0U, 0.50);
        signals.set_voltage(1U, 0.50);
        bus.tick_peripherals(1U);
        CHECK((port_b.sample_levels() & 0x04U) == 0U); // Still LOW (hysteresis)

        // Step 3: Set signal 0 to 0.80 (HIGH -> PCINT expected)
        signals.set_voltage(0U, 0.80);
        bus.tick_peripherals(1U);
        CHECK((port_b.sample_levels() & 0x04U) != 0U);
        CHECK(bus.read_data(atmega328.pin_change_interrupt_0.pcifr_address) == 0x01U);

        // Step 4: Clear PCIF
        bus.write_data(atmega328.pin_change_interrupt_0.pcifr_address, 0x01U);
        InterruptRequest request {};
        CHECK_FALSE(bus.pending_interrupt_request(request));

        // Step 5: Set signal 1 to 0.20 (HIGH -> LOW -> INT0 falling edge expected)
        signals.set_voltage(1U, 0.20);
        bus.tick_peripherals(1U);
        
        CHECK(bus.pending_interrupt_request(request));
        CHECK(request.vector_index == atmega328.ext_interrupt.int0_vector_index);
        CHECK(bus.read_data(atmega328.ext_interrupt.eifr_address) == 0x01U);
    }
}
