#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/atmega328.hpp"

using namespace vioavr::core;

TEST_CASE("ADC Basic Functionality Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    const auto adc_vector = atmega328.adcs[0].vector_index;

    PinMux pin_mux(8);
    MemoryBus bus {atmega328};
    Adc adc0 {"ADC0", atmega328.adcs[0], pin_mux, 6U, 4U}; // Source ID 6, 4 cycles
    adc0.set_bus(bus);
    bus.attach_peripheral(adc0);
    bus.reset();

    SUBCASE("Single Conversion") {
        adc0.set_channel_voltage(0U, 0.5); // 0.5V -> Expected result 512
        bus.write_data(atmega328.adcs[0].admux_address, 0x00U);
        bus.write_data(atmega328.adcs[0].adcsra_address, 0xC8U); // ADEN | ADSC | ADIE
        
        // ADSC should be set
        CHECK((bus.read_data(atmega328.adcs[0].adcsra_address) & 0x40U) != 0U);

        bus.tick_peripherals(4U); // 4 cycles as configured in constructor
        
        // ADSC should be cleared, ADIF should be set
        const u8 status = bus.read_data(atmega328.adcs[0].adcsra_address);
        CHECK((status & 0x40U) == 0U);
        CHECK((status & 0x10U) != 0U);

        const auto result = static_cast<vioavr::core::u16>(
            bus.read_data(atmega328.adcs[0].adcl_address) |
            (static_cast<vioavr::core::u16>(bus.read_data(atmega328.adcs[0].adch_address)) << 8U)
        );
        CHECK(result >= 510U);
        CHECK(result <= 513U);

        InterruptRequest request {};
        CHECK(bus.pending_interrupt_request(request));
        CHECK(request.vector_index == adc_vector);

        // Clear ADIF
        bus.write_data(atmega328.adcs[0].adcsra_address, 0x10U);
        CHECK_FALSE(bus.pending_interrupt_request(request));
        CHECK((bus.read_data(atmega328.adcs[0].adcsra_address) & 0x10U) == 0U);
    }
}
