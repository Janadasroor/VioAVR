#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

using namespace vioavr::core;
void step_to(AvrCpu& cpu, u32 target_pc) { while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) { cpu.step(); } }

TEST_CASE("ADC Timer0 Overflow Auto-Trigger Test")
{
    using vioavr::core::Adc;
    using vioavr::core::MemoryBus;
    using vioavr::core::Timer8;
    using vioavr::core::devices::atmega328p;

    PinMux pin_mux(8);
    MemoryBus bus {atmega328p};
    Adc adc0 {"ADC0", atmega328p.adcs[0], pin_mux, 6U, 4U};
    adc0.set_bus(bus);
    Timer8 timer0 {"TIMER0", atmega328p.timers8[0]};

    adc0.connect_timer_overflow_auto_trigger(timer0);
    adc0.set_channel_voltage(0U, 0.33); // Expected: 338
    
    bus.attach_peripheral(adc0);
    bus.attach_peripheral(timer0);
    bus.reset();

    SUBCASE("Trigger from Timer0 Overflow") {
        bus.write_data(atmega328p.adcs[0].admux_address, 0x00U);
        bus.write_data(atmega328p.adcs[0].adcsrb_address, 0x04U); // Timer0 Overflow trigger
        bus.write_data(atmega328p.timers8[0].tcnt_address, 0xFFU);
        bus.write_data(atmega328p.adcs[0].adcsra_address, 0xA0U);  // ADEN | ADATE
        
        // Start timer at clk/1
        bus.write_data(atmega328p.timers8[0].tccrb_address, 0x01U);
        
        bus.tick_peripherals(1U);
        // TCNT should be 0x00, Overflow flag should be set
        CHECK(bus.read_data(atmega328p.timers8[0].tcnt_address) == 0x00U);
        CHECK((timer0.interrupt_flags() & 0x01U) != 0U);
        
        // ADC ADSC should be set (auto-triggered)
        CHECK((bus.read_data(atmega328p.adcs[0].adcsra_address) & 0x40U) != 0U);

        bus.tick_peripherals(4U); // Complete conversion
        CHECK((bus.read_data(atmega328p.adcs[0].adcsra_address) & 0x10U) != 0U); // ADIF set

        const auto result = static_cast<vioavr::core::u16>(
            bus.read_data(atmega328p.adcs[0].adcl_address) |
            (static_cast<vioavr::core::u16>(bus.read_data(atmega328p.adcs[0].adch_address)) << 8U)
        );
        CHECK(result >= 336U);
        CHECK(result <= 339U);
    }
}
