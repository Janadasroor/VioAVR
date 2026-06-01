#include "doctest.h"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/devices/atmega32u4.hpp"

using namespace vioavr::core;

TEST_CASE("ADC: Differential Channels and Gain") {
    MemoryBus bus {devices::atmega32u4};
    PinMux pin_mux {5};
    Adc adc {"ADC", devices::atmega32u4.adcs[0], pin_mux, 0, 13};
    
    // CRITICAL: Attach to bus
    bus.attach_peripheral(adc);
    
    AnalogSignalBank signals;
    adc.bind_signal_bank(signals);
    
    SUBCASE("ADC0 - ADC1 with 10x Gain") {
        // MUX 0x08: ADC0-ADC1 10x
        bus.write_data(devices::atmega32u4.adcs[0].admux_address, 0x08);
        
        // (pos - neg) * gain / VREF * 512 = 511 (max 10-bit differential)
        // (0.5 - 0.0) * 10 / 5.0 * 512 = 512 -> clamp to 511
        signals.set_voltage(0, 0.5);
        signals.set_voltage(1, 0.0);
        
        // Start conversion: ADEN=1, ADSC=1
        bus.write_data(devices::atmega32u4.adcs[0].adcsra_address, 0xC0);
        
        // Tick enough cycles via bus to trigger scheduler callbacks
        bus.tick_peripherals(30);
        
        u16 result = bus.read_data(devices::atmega32u4.adcs[0].adcl_address);
        result |= (static_cast<u16>(bus.read_data(devices::atmega32u4.adcs[0].adch_address)) << 8);
        
        // (0.5 - 0.0) * 10 / 5.0 * 512 = 512
        // result = clamp(512, -512, 511) = 511 (0x1FF)
        CHECK(result == 0x1FF);
    }
}
