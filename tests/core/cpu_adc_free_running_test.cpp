#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

using namespace vioavr::core;

TEST_CASE("ADC Free Running Mode Test")
{
    using vioavr::core::Adc;
    using vioavr::core::MemoryBus;
    using vioavr::core::devices::atmega328p;

    PinMux pin_mux(8);
    MemoryBus bus {atmega328p};
    Adc adc0 {"ADC0", atmega328p.adcs[0], pin_mux, 6U, 4U};
    adc0.set_bus(bus);
    bus.attach_peripheral(adc0);
    bus.reset();

    adc0.set_channel_voltage(0U, 0.50);
    bus.write_data(atmega328p.adcs[0].admux_address, 0x00U);
    bus.write_data(atmega328p.adcs[0].adcsrb_address, 0x00U);   // ADTS = free running
    bus.write_data(atmega328p.adcs[0].adcsra_address, 0xE0U);   // ADEN | ADSC | ADATE

    SUBCASE("First conversion") {
        bus.tick_peripherals(8U); // Default conversion cycles
        const auto status = bus.read_data(atmega328p.adcs[0].adcsra_address);
        
        // ADSC (bit 6) should stay high in free-running, ADIF (bit 4) should be set
        CHECK((status & 0x50U) == 0x50U);

        const auto result = static_cast<vioavr::core::u16>(
            bus.read_data(atmega328p.adcs[0].adcl_address) |
            (static_cast<vioavr::core::u16>(bus.read_data(atmega328p.adcs[0].adch_address)) << 8U)
        );
        CHECK(result >= 510U);
        CHECK(result <= 513U);
    }

    SUBCASE("Continuous conversion") {
        bus.tick_peripherals(8U);
        bus.write_data(atmega328p.adcs[0].adcsra_address, 0xF0U); // Clear ADIF
        
        CHECK((bus.read_data(atmega328p.adcs[0].adcsra_address) & 0x10U) == 0U);
        
        adc0.set_channel_voltage(0U, 0.75);
        bus.tick_peripherals(8U); // Next conversion
        
        CHECK((bus.read_data(atmega328p.adcs[0].adcsra_address) & 0x10U) != 0U);
        
        const auto result = static_cast<vioavr::core::u16>(
            bus.read_data(atmega328p.adcs[0].adcl_address) |
            (static_cast<vioavr::core::u16>(bus.read_data(atmega328p.adcs[0].adch_address)) << 8U)
        );
        CHECK(result >= 766U);
        CHECK(result <= 769U);
    }
}
