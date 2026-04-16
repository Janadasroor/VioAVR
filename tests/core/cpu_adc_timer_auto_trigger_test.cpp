#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/atmega328.hpp"

using namespace vioavr::core;
void step_to(AvrCpu& cpu, u32 target_pc) { while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) { cpu.step(); } }

TEST_CASE("ADC Timer Auto-Trigger Test")
{
    using vioavr::core::Adc;
    using vioavr::core::MemoryBus;
    using vioavr::core::Timer8;
    using vioavr::core::devices::atmega328;

    PinMux pin_mux(8);
    MemoryBus bus {atmega328};
    Adc adc0 {"ADC0", atmega328.adcs[0], pin_mux, 6U, 4U};
    adc0.set_bus(bus);
    Timer8 timer0 {"TIMER0", atmega328.timers8[0]};
    
    adc0.connect_timer_compare_auto_trigger(timer0);
    bus.attach_peripheral(adc0);
    bus.attach_peripheral(timer0);
    bus.reset();

    adc0.set_channel_voltage(0U, 0.60);
    bus.write_data(atmega328.adcs[0].admux_address, 0x00U);
    bus.write_data(atmega328.adcs[0].adcsrb_address, 0x03U); // Timer0 Compare Match A
    bus.write_data(atmega328.timers8[0].ocra_address, 0x02U);
    bus.write_data(atmega328.timers8[0].tccrb_address, 0x01U); // Start timer clk/1
    bus.write_data(atmega328.adcs[0].adcsra_address, 0xA0U);  // ADEN | ADATE

    SUBCASE("Timer increment and Trigger") {
        bus.tick_peripherals(1U);
        CHECK(bus.read_data(atmega328.timers8[0].tcnt_address) == 0x01U);
        CHECK((bus.read_data(atmega328.adcs[0].adcsra_address) & 0x40U) == 0U);

        bus.tick_peripherals(1U);
        CHECK(bus.read_data(atmega328.timers8[0].tcnt_address) == 0x02U);
        CHECK(timer0.interrupt_flags() == 0x02U);
        
        // ADSC (bit 6) should be set by auto-trigger
        CHECK((bus.read_data(atmega328.adcs[0].adcsra_address) & 0x40U) != 0U);
    }

    SUBCASE("Conversion Result") {
        bus.tick_peripherals(2U); // Trigger at 2 ticks
        bus.tick_peripherals(4U); // Wait for conversion (placeholder logic usually takes some cycles)
        
        // Check ADIF (bit 4)
        CHECK((bus.read_data(atmega328.adcs[0].adcsra_address) & 0x10U) != 0U);

        const auto result = static_cast<vioavr::core::u16>(
            bus.read_data(atmega328.adcs[0].adcl_address) |
            (static_cast<vioavr::core::u16>(bus.read_data(atmega328.adcs[0].adch_address)) << 8U)
        );
        CHECK(result >= 612U);
        CHECK(result <= 615U);
    }
}
