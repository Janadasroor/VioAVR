#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/ext_interrupt.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega328.hpp"

using namespace vioavr::core;
void step_to(AvrCpu& cpu, u32 target_pc) { while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) { cpu.step(); } }

TEST_CASE("ADC External Interrupt Auto-Trigger Test")
{
    using vioavr::core::Adc;
    using vioavr::core::ExtInterrupt;
    using vioavr::core::MemoryBus;
    using vioavr::core::devices::atmega328;

    MemoryBus bus {atmega328};
    Adc adc0 {"ADC0", atmega328, 6U, 4U};
    ExtInterrupt exti {"EXTINT", atmega328, 4U};
    
    adc0.connect_external_interrupt_0_auto_trigger(exti);
    bus.attach_peripheral(adc0);
    bus.attach_peripheral(exti);
    bus.reset();

    adc0.set_channel_voltage(0U, 0.42);
    bus.write_data(atmega328.adc.admux_address, 0x00U);
    bus.write_data(atmega328.adc.adcsrb_address, 0x02U); // External Interrupt 0 Trigger
    bus.write_data(atmega328.ext_interrupt.eicra_address, 0x02U); // Falling edge
    bus.write_data(atmega328.adc.adcsra_address, 0xA0U);  // ADEN | ADATE

    SUBCASE("Trigger ADC via INT0 falling edge") {
        exti.set_int0_level(true);
        exti.set_int0_level(false); // Falling edge -> Trigger
        
        // ADSC (bit 6) should be set
        CHECK((bus.read_data(atmega328.adc.adcsra_address) & 0x40U) != 0U);
        // EIFR (bit 0) should be set
        CHECK(bus.read_data(atmega328.ext_interrupt.eifr_address) == 0x01U);

        bus.tick_peripherals(10U); // Wait for conversion
        
        // ADIF (bit 4) should be set
        CHECK((bus.read_data(atmega328.adc.adcsra_address) & 0x10U) != 0U);

        const auto result = static_cast<vioavr::core::u16>(
            bus.read_data(atmega328.adc.adcl_address) |
            (static_cast<vioavr::core::u16>(bus.read_data(atmega328.adc.adch_address)) << 8U)
        );
        CHECK(result >= 429U);
        CHECK(result <= 431U);
    }
}
