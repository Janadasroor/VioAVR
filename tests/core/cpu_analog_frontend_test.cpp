#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/analog_comparator.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega328.hpp"

TEST_CASE("Analog Frontend ADC and Comparator Integration Test")
{
    using vioavr::core::Adc;
    using vioavr::core::AnalogComparator;
    using vioavr::core::AnalogSignalBank;
    using vioavr::core::InterruptRequest;
    using vioavr::core::MemoryBus;
    using vioavr::core::devices::atmega328;

    constexpr auto acsr = static_cast<vioavr::core::u16>(0x50U);
    constexpr auto comparator_vector = static_cast<vioavr::core::u8>(8U);

    AnalogSignalBank signals;
    signals.set_voltage(0U, 0.25);
    signals.set_voltage(1U, 0.70);

    MemoryBus bus {atmega328};
    Adc adc0 {"ADC0", atmega328, 6U, 4U};
    adc0.bind_signal_bank(signals);
    
    AnalogComparator comparator {"AC", acsr, comparator_vector, 9U, 1.1}; // Refactored bandgap
    comparator.bind_signal_bank(signals, 0U, 1U);
    
    bus.attach_peripheral(adc0);
    bus.attach_peripheral(comparator);
    bus.reset();

    SUBCASE("ADC Conversion of Signal Bank") {
        bus.write_data(atmega328.adc.admux_address, 0x00U);
        bus.write_data(atmega328.adc.adcsra_address, 0xC0U); // ADEN | ADSC
        bus.tick_peripherals(10U);
        
        const auto result = static_cast<vioavr::core::u16>(
            bus.read_data(atmega328.adc.adcl_address) |
            (static_cast<vioavr::core::u16>(bus.read_data(atmega328.adc.adch_address)) << 8U)
        );
        // 0.25V -> 1024 * 0.25 = 256
        CHECK(result >= 254U);
        CHECK(result <= 258U);
    }

    SUBCASE("Analog Comparator Level and Flags") {
        bus.write_data(acsr, 0x0BU); // ACIE | ACIS1 | ACIS0
        CHECK((bus.read_data(acsr) & 0x20U) == 0U); // ACO=0 (0.25 < 0.70)

        signals.set_voltage(0U, 0.69);
        bus.tick_peripherals(1U);
        CHECK((bus.read_data(acsr) & 0x20U) == 0U); // ACO=0 (0.69 < 0.70)

        signals.set_voltage(0U, 0.75); // Greater than 0.70 -> ACO=1
        bus.tick_peripherals(1U);
        
        CHECK((bus.read_data(acsr) & 0x30U) == 0x30U); // ACO=1, ACI set (rising edge)
        
        InterruptRequest request {};
        CHECK(bus.pending_interrupt_request(request));
        CHECK(request.vector_index == comparator_vector);
    }
}
