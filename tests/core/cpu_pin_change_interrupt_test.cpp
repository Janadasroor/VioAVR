#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_change_interrupt.hpp"
#include "vioavr/core/devices/atmega328.hpp"

using namespace vioavr::core;
void step_to(AvrCpu& cpu, u32 target_pc) { while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) { cpu.step(); } }

TEST_CASE("Pin Change Interrupt Logic Test")
{
    using vioavr::core::GpioPort;
    using vioavr::core::InterruptRequest;
    using vioavr::core::MemoryBus;
    using vioavr::core::PinChangeInterrupt;
    using vioavr::core::devices::atmega328;

    constexpr auto pinb = static_cast<vioavr::core::u16>(0x23U);
    constexpr auto ddrb = static_cast<vioavr::core::u16>(0x24U);
    constexpr auto portb = static_cast<vioavr::core::u16>(0x25U);
    
    MemoryBus bus {atmega328};
    GpioPort port_b {"PORTB", pinb, ddrb, portb};
    PinChangeInterruptDescriptor pci0_desc { .pcicr_address = 0x68U, .pcifr_address = 0x3BU, .pcmsk_address = 0x6BU, .pcicr_enable_mask = 0x01U, .pcifr_flag_mask = 0x01U, .vector_index = 3U };
    PinChangeInterrupt pci0 {"PCINT0", pci0_desc, port_b};
    
    bus.attach_peripheral(port_b);
    bus.attach_peripheral(pci0);
    bus.reset();

    InterruptRequest request {};

    SUBCASE("Toggle pin and check interrupt") {
        bus.write_data(0x6BU, 0x04U); // PB2
        bus.write_data(0x68U, 0x01U); // PCIE0
        
        port_b.set_input_levels(0x00U);
        bus.tick_peripherals(1U);
        port_b.set_input_levels(0x04U);
        bus.tick_peripherals(1U);
        
        CHECK(bus.pending_interrupt_request(request));
        CHECK(request.vector_index == 3U);
        CHECK(bus.read_data(0x3BU) == 0x01U);

        CHECK(bus.consume_interrupt_request(request));
        CHECK(bus.read_data(0x3BU) == 0x00U);
    }

    SUBCASE("Clear flag by writing 1") {
        bus.write_data(0x6BU, 0x04U);
        bus.write_data(0x68U, 0x01U);
        
        port_b.set_input_levels(0xFFU);
        bus.tick_peripherals(1U);
        port_b.set_input_levels(0x00U);
        bus.tick_peripherals(1U);
        
        CHECK(bus.read_data(0x3BU) == 0x01U);
        bus.write_data(0x3BU, 0x01U);
        CHECK(bus.read_data(0x3BU) == 0x00U);
        CHECK_FALSE(bus.pending_interrupt_request(request));
    }

    SUBCASE("Disable mask then toggle") {
        bus.write_data(0x6BU, 0x00U);
        
        port_b.set_input_levels(0x04U);
        bus.tick_peripherals(1U);
        
        CHECK_FALSE(bus.pending_interrupt_request(request));
        CHECK(bus.read_data(0x3BU) == 0x00U);
    }
}
