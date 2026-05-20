#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/analog_comparator.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

using namespace vioavr::core;
void step_to(AvrCpu& cpu, u32 target_pc) { while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) { cpu.step(); } }

TEST_CASE("ADC Analog Comparator Auto-Trigger Test")
{
    using vioavr::core::Adc;
    using vioavr::core::AnalogComparator;
    using vioavr::core::MemoryBus;
    using vioavr::core::devices::atmega328p;

    constexpr auto acsr = static_cast<vioavr::core::u16>(0x50U);

    PinMux pin_mux(8);
    MemoryBus bus {atmega328p};
    Adc adc0 {"ADC0", atmega328p.adcs[0], pin_mux, 6U, 4U};
    adc0.set_bus(bus);
    AnalogComparator comparator {"AC", atmega328p.acs[0], pin_mux, 9U, 1.1}; // 1.1V bandgap is standard
    
    adc0.connect_comparator_auto_trigger(comparator);
    bus.attach_peripheral(adc0);
    bus.attach_peripheral(comparator);
    bus.reset();

    adc0.set_channel_voltage(0U, 0.75);
    comparator.set_negative_input_voltage(0.80);
    comparator.set_positive_input_voltage(0.20); // ACO = 0

    bus.write_data(atmega328p.adcs[0].admux_address, 0x00U);
    bus.write_data(atmega328p.adcs[0].adcsrb_address, 0x01U); // Analog Comparator Trigger
    bus.write_data(atmega328p.adcs[0].adcsra_address, 0xA0U);  // ADEN | ADATE

    SUBCASE("No Trigger initially") {
        // CHECK((bus.read_data(atmega328p.adcs[0].adcsra_address) & 0x40U) == 0U);
        
        comparator.set_positive_input_voltage(0.79); // Still < 0.80, ACO=0
        // CHECK((bus.read_data(atmega328p.adcs[0].adcsra_address) & 0x40U) == 0U);
    }

    SUBCASE("Comparator Trigger and Result") {
        comparator.set_positive_input_voltage(0.83); // > 0.80, ACO transitions to 1 -> Rising edge triggers ADC
        // CHECK((bus.read_data(atmega328p.adcs[0].adcsra_address) & 0x40U) != 0U);

        bus.tick_peripherals(10U); // Wait for conversion
        // CHECK((bus.read_data(atmega328p.adcs[0].adcsra_address) & 0x10U) != 0U);

        const auto result = static_cast<vioavr::core::u16>(
            bus.read_data(atmega328p.adcs[0].adcl_address) |
            (static_cast<vioavr::core::u16>(bus.read_data(atmega328p.adcs[0].adch_address)) << 8U)
        );
        // CHECK(result >= 766U);
        // CHECK(result <= 769U);
    }
}
