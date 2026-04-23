#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/analog_comparator.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/atmega32u4.hpp"
#include <iostream>

using namespace vioavr::core;

TEST_CASE("SoC: Analog Comparator Triggers Timer1 Capture") {
    MemoryBus bus {devices::atmega32u4};
    PinMux pin_mux {5};
    
    Timer16 timer1 ("TIMER1", devices::atmega32u4.timers16[0]);
    timer1.set_bus(bus);
    bus.attach_peripheral(timer1);
    
    AnalogComparator ac ("AC", devices::atmega32u4.acs[0], pin_mux, 0);
    bus.attach_peripheral(ac);
    
    ac.connect_timer_input_capture(timer1);
    
    bus.write_data(devices::atmega32u4.timers16[0].tccrb_address, 0x41); 
    bus.write_data(devices::atmega32u4.acs[0].acsr_address, 0x04); 
    
    bus.write_data(devices::atmega32u4.timers16[0].tcnt_address + 1, 0x12);
    bus.write_data(devices::atmega32u4.timers16[0].tcnt_address, 0x34); 
    
    ac.set_negative_input_voltage(0.1);
    ac.set_positive_input_voltage(0.2); 
    ac.tick(1);
    
    u16 icr_l = bus.read_data(devices::atmega32u4.timers16[0].icr_address);
    u16 icr_h = bus.read_data(devices::atmega32u4.timers16[0].icr_address + 1);
    u16 icr = icr_l | (icr_h << 8);
    
    CHECK(icr == 0x1234);
    
    u8 tifr = bus.read_data(devices::atmega32u4.timers16[0].tifr_address);
    u8 icf_mask = devices::atmega32u4.timers16[0].capture_enable_mask;
    CHECK((tifr & icf_mask) != 0);
}

TEST_CASE("SoC: Analog Comparator Triggers ADC") {
    MemoryBus bus {devices::atmega32u4};
    PinMux pin_mux {5};
    
    Adc adc ("ADC", devices::atmega32u4.adcs[0], pin_mux, 0);
    bus.attach_peripheral(adc);
    AnalogComparator ac ("AC", devices::atmega32u4.acs[0], pin_mux, 0);
    bus.attach_peripheral(ac);
    
    ac.connect_adc_auto_trigger(adc);
    adc.select_auto_trigger_source(Adc::AutoTriggerSource::analog_comparator);
    
    bus.write_data(devices::atmega32u4.adcs[0].adcsra_address, 0xA0); 
    bus.write_data(devices::atmega32u4.acs[0].acsr_address, 0x04); 
    
    ac.set_negative_input_voltage(0.1);
    ac.set_positive_input_voltage(0.2);
    ac.tick(1);
    
    u8 adcsra = bus.read_data(devices::atmega32u4.adcs[0].adcsra_address);
    CHECK((adcsra & 0x40) != 0); // ADSC
}

TEST_CASE("SoC: Timer1 Triggers ADC") {
    MemoryBus bus {devices::atmega32u4};
    PinMux pin_mux {5};
    
    Adc adc ("ADC", devices::atmega32u4.adcs[0], pin_mux, 0);
    bus.attach_peripheral(adc);
    Timer16 timer1 ("TIMER1", devices::atmega32u4.timers16[0]);
    timer1.set_bus(bus);
    bus.attach_peripheral(timer1);
    
    timer1.connect_adc_auto_trigger(adc);
    
    // Test 1: Overflow trigger
    adc.select_auto_trigger_source(Adc::AutoTriggerSource::timer1_overflow);
    bus.write_data(devices::atmega32u4.adcs[0].adcsra_address, 0xA0); // ADEN, ADATE
    
    // Wrap Timer1 to overflow
    bus.write_data(devices::atmega32u4.timers16[0].tcnt_address + 1, 0xFF);
    bus.write_data(devices::atmega32u4.timers16[0].tcnt_address, 0xFF);
    
    // Set Clock (CS=1) and Mode (Normal)
    bus.write_data(devices::atmega32u4.timers16[0].tccrb_address, 0x01);
    
    timer1.tick(1); 
    
    u8 tifr = bus.read_data(devices::atmega32u4.timers16[0].tifr_address);
    CHECK((tifr & devices::atmega32u4.timers16[0].overflow_enable_mask) != 0);
    
    u8 adcsra = bus.read_data(devices::atmega32u4.adcs[0].adcsra_address);
    CHECK((adcsra & 0x40) != 0);
    
    // Clear ADSC and TIFR
    // Reset ADC state by disabling and re-enabling
    bus.write_data(devices::atmega32u4.adcs[0].adcsra_address, 0x00); 
    bus.write_data(devices::atmega32u4.adcs[0].adcsra_address, 0xA0); 
    bus.write_data(devices::atmega32u4.timers16[0].tifr_address, 0xFF);
    
    // Test 2: Compare Match B trigger
    adc.select_auto_trigger_source(Adc::AutoTriggerSource::timer1_compare_b);
    
    // Set OCR1B = 0x0010
    bus.write_data(devices::atmega32u4.timers16[0].ocrb_address + 1, 0x00);
    bus.write_data(devices::atmega32u4.timers16[0].ocrb_address, 0x10);
    
    // Set TCNT = 0x000F (one before compare value)
    bus.write_data(devices::atmega32u4.timers16[0].tcnt_address + 1, 0x00);
    bus.write_data(devices::atmega32u4.timers16[0].tcnt_address, 0x0F);
    
    timer1.tick(2); // First tick: 0x0F->0x10 (match), Second tick: 0x10->0x11
    
    tifr = bus.read_data(devices::atmega32u4.timers16[0].tifr_address);
    CHECK((tifr & devices::atmega32u4.timers16[0].compare_b_enable_mask) != 0);
    
    adcsra = bus.read_data(devices::atmega32u4.adcs[0].adcsra_address);
    CHECK((adcsra & 0x40) != 0);
}
