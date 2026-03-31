#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/devices/atmega328.hpp"

TEST_CASE("ADC Descriptor and Trigger Logic Test")
{
    using vioavr::core::Adc;
    using vioavr::core::devices::atmega328;

    Adc adc0 {"ADC0", atmega328, 6U, 4U};
    adc0.reset();

    SUBCASE("Trigger Select Register") {
        adc0.write(atmega328.adc.adcsrb_address, 0x01U);
        CHECK(adc0.trigger_select_register() == 0x01U);
    }

    SUBCASE("Auto Trigger - Comparator") {
        adc0.write(atmega328.adc.adcsrb_address, 0x01U); // Comparator trigger select
        adc0.write(atmega328.adc.adcsra_address, 0xA0U); // ADEN | ADATE
        
        adc0.notify_auto_trigger(Adc::AutoTriggerSource::comparator);
        
        // ADSC (bit 6) should be set
        CHECK((adc0.adcsra() & 0x40U) != 0U);
    }

    SUBCASE("Auto Trigger - Timer Overflow") {
        adc0.reset();
        adc0.write(atmega328.adc.adcsrb_address, 0x04U); // Timer0 Overflow trigger select
        adc0.write(atmega328.adc.adcsra_address, 0xA0U);
        
        adc0.notify_auto_trigger(Adc::AutoTriggerSource::timer_overflow);
        CHECK((adc0.adcsra() & 0x40U) != 0U);
    }

    SUBCASE("Auto Trigger - Mismatched Source") {
        adc0.reset();
        adc0.write(atmega328.adc.adcsrb_address, 0x06U); // DIFFERENT trigger select
        adc0.write(atmega328.adc.adcsra_address, 0xA0U);
        
        adc0.notify_auto_trigger(Adc::AutoTriggerSource::timer_overflow);
        // ADSC should NOT be set because the trigger source doesn't match the selection
        CHECK((adc0.adcsra() & 0x40U) == 0U);
    }
}
