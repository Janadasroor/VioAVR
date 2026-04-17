#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/ext_interrupt.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_change_interrupt.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/atmega328.hpp"

using namespace vioavr::core;
void step_to(AvrCpu& cpu, u32 target_pc) { while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) { cpu.step(); } }

TEST_CASE("CPU Voltage and Ext/PinChange Interrupt Test")
{
    using vioavr::core::ExtInterrupt;
    using vioavr::core::GpioPort;
    using vioavr::core::InterruptRequest;
    using vioavr::core::MemoryBus;
    using vioavr::core::PinChangeInterrupt;
    using vioavr::core::devices::atmega328;

    constexpr auto pinb = static_cast<vioavr::core::u16>(0x23U);
    constexpr auto ddrb = static_cast<vioavr::core::u16>(0x24U);
    constexpr auto portb = static_cast<vioavr::core::u16>(0x25U);
    
    PinMux pin_mux {8};
    MemoryBus bus {atmega328};
    vioavr::core::PinMux pm_port_b { 10 }; GpioPort port_b { "PORTB", pinb, ddrb, portb, pm_port_b };
    ExtInterrupt exti {"EXTINT", atmega328.ext_interrupts[0], pin_mux, 4U};
    
    // PCICR=0x68, PCIFR=0x3B, PCMSK0=0x6B (from ATmega328P datasheet)
    PinChangeInterruptDescriptor pci0_desc {
        .pcicr_address = 0x68U,
        .pcifr_address = 0x3BU,
        .pcmsk_address = 0x6BU,
        .pcicr_enable_mask = 0x01U,
        .pcifr_flag_mask = 0x01U,
        .vector_index = 3U
    };
    PinChangeInterrupt pci0 {"PCINT0", pci0_desc, port_b};
    
    bus.attach_peripheral(port_b);
    bus.attach_peripheral(exti);
    bus.attach_peripheral(pci0);
    bus.reset();

    SUBCASE("External Interrupt 0 Voltage Trigger") {
        bus.write_data(atmega328.ext_interrupts[0].eicra_address, 0x02U); // Falling edge
        bus.write_data(atmega328.ext_interrupts[0].eimsk_address, 0x01U); // Enable INT0
        
        exti.set_int0_voltage(0.8); // HIGH
        exti.set_int0_voltage(0.5); // Still above threshold for HIGH? 
        // Note: Default threshold is usually around 0.3-0.6 range. 
        // Let's assume 0.5 is still HIGH or transition point.
        
        InterruptRequest request {};
        CHECK_FALSE(bus.pending_interrupt_request(request));

        exti.set_int0_voltage(0.1); // Definitely LOW -> Falling edge
        CHECK(bus.pending_interrupt_request(request));
        CHECK(request.vector_index == atmega328.ext_interrupts[0].vector_indices[0]);
    }

    SUBCASE("Pin Change Interrupt via Voltage") {
        // PCICR=0x68, PCIFR=0x3B, PCMSK0=0x6B (from ATmega328P datasheet)
        bus.write_data(0x6BU, 0x04U); // PCMSK0: PB2
        bus.write_data(0x68U, 0x01U); // PCICR: PCIE0
        
        port_b.set_input_voltage(2U, 0.2); // Start LOW
        bus.tick_peripherals(1U);
        port_b.set_input_voltage(2U, 0.4); // Still LOW (hysteresis)
        bus.tick_peripherals(1U);
        
        CHECK(bus.read_data(0x3BU) == 0x00U); // PCIFR should be clear

        port_b.set_input_voltage(2U, 0.8); // Transition to HIGH
        bus.tick_peripherals(1U);
        
        // PCIF0 should be set
        CHECK(bus.read_data(0x3BU) == 0x01U);
    }
}
