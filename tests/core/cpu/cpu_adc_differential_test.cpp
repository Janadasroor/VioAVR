#include "doctest.h"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/atmega32u4.hpp"

namespace {
using namespace vioavr::core;
using namespace vioavr::core::devices;
}

TEST_CASE("ADC Differential Channels and Gain")
{
    PinMux pm {8}; // Large enough for all ports
    Adc adc {"ADC", atmega32u4.adcs[0], pm, 0, 13};
    MemoryBus bus {atmega32u4};
    adc.set_bus(bus);
    bus.attach_peripheral(adc);
    
    adc.reset();

    SUBCASE("Single-Ended ADC0") {
        adc.set_channel_voltage(0, 2.5); // 2.5V input, VREF=5.0V
        bus.write_data(atmega32u4.adcs[0].admux_address, 0x00); // MUX=0
        bus.write_data(atmega32u4.adcs[0].adcsra_address, 0xC0); // ADEN=1, ADSC=1
        
        adc.tick(13);
        CHECK(adc.result() == 512); // 2.5V / 5.0V * 1024 = 512
    }

    SUBCASE("Differential ADC0(+) vs ADC1(-) with 10x Gain") {
        adc.set_channel_voltage(0, 2.0); 
        adc.set_channel_voltage(1, 1.75); // Delta = 0.25V
        // Gain 10x -> VREF=5.0V -> (0.25V * 10) / 5.0V = 0.5
        // Result = 0.5 * 512 = 256
        
        bus.write_data(atmega32u4.adcs[0].admux_address, 0x08); // MUX=8
        bus.write_data(atmega32u4.adcs[0].adcsra_address, 0xC0); // Start
        
        adc.tick(13);
        CHECK(static_cast<i16>(adc.result()) == 256);
    }

    SUBCASE("Differential ADC0(+) vs ADC1(-) with 40x Gain (Negative Result)") {
        adc.set_channel_voltage(0, 1.5); 
        adc.set_channel_voltage(1, 1.55); // Delta = -0.05V
        // Gain 40x -> VREF=5.0V -> (-0.05V * 40) / 5.0V = -0.4
        // Result = -0.4 * 512 = -204.8 -> -204 (truncation)
        
        bus.write_data(atmega32u4.adcs[0].admux_address, 0x09); // MUX=9
        bus.write_data(atmega32u4.adcs[0].adcsra_address, 0xC0); // Start
        
        adc.tick(13);
        i16 result = static_cast<i16>(adc.result());
        if (result & 0x200) result |= 0xFC00; // Sign extend 10-bit
        CHECK(result == -204);
    }

    SUBCASE("High-Side Offset (Clipping)") {
        adc.set_channel_voltage(0, 3.0); 
        adc.set_channel_voltage(1, 2.5); // Delta = 0.5V
        // Gain 10x -> VREF=5.0V -> (0.5V * 10) / 5.0V = 1.0 (Full scale)
        
        bus.write_data(atmega32u4.adcs[0].admux_address, 0x08); 
        bus.write_data(atmega32u4.adcs[0].adcsra_address, 0xC0); 
        
        adc.tick(13);
        CHECK(static_cast<i16>(adc.result()) == 511); // Max positive 10-bit diff
    }
}
