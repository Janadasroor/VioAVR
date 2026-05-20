#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

using namespace vioavr::core;

TEST_CASE("DeviceCatalog find existing devices") {
    auto* atmega328p = DeviceCatalog::find("ATmega328P");
    REQUIRE(atmega328p != nullptr);
    CHECK(atmega328p->name == "ATmega328P");
    CHECK(atmega328p->flash_words == 16384U);
    CHECK(atmega328p->sram_bytes == 2048U);

    auto* atmega2560 = DeviceCatalog::find("ATmega2560");
    REQUIRE(atmega2560 != nullptr);
    CHECK(atmega2560->name == "ATmega2560");
    CHECK(atmega2560->flash_words == 128U * 1024U);
}

TEST_CASE("DeviceCatalog find non-existent device") {
    auto* nonexistent = DeviceCatalog::find("ATmegaNoSuchChip");
    CHECK(nonexistent == nullptr);
}

TEST_CASE("DeviceCatalog list devices") {
    auto devices = DeviceCatalog::list_devices();
    CHECK(devices.size() > 100U);
    
    bool found_328 = false;
    for (const auto& name : devices) {
        if (name == "ATmega328P") found_328 = true;
    }
    CHECK(found_328);
}

TEST_CASE("ATmega328P descriptor exposes Timer2 and all PCINT groups") {
    auto* atmega328p = DeviceCatalog::find("ATmega328P");
    REQUIRE(atmega328p != nullptr);

    CHECK(atmega328p->adc_count == 1);
    CHECK(atmega328p->adcs[0].adcsra_address == 0x7AU);
    
    CHECK(atmega328p->timer8_count == 2);
    CHECK(atmega328p->timers8[0].tcnt_address == 0x46U);  // Timer0
    CHECK(atmega328p->timers8[1].tcnt_address == 0xB2U);  // Timer2
    
    CHECK(atmega328p->timer16_count == 1);
    CHECK(atmega328p->timers16[0].tcnt_address == 0x84U); // Timer1

    const auto& timer2 = atmega328p->timers8[1];
    CHECK(timer2.tccra_address == 0xB0U);
    CHECK(timer2.tccrb_address == 0xB1U);
    CHECK(timer2.assr_address == 0xB6U);
    CHECK(timer2.timsk_address == 0x70U);
    CHECK(timer2.tifr_address == 0x37U);
    CHECK(timer2.compare_a_vector_index == 7U);
    CHECK(timer2.compare_b_vector_index == 8U);
    CHECK(timer2.overflow_vector_index == 9U);

    REQUIRE(atmega328p->pcint_count >= 3);
    CHECK(atmega328p->pcints[0].pcicr_address == 0x68U);
    CHECK(atmega328p->pcints[0].pcifr_address == 0x3BU);
    CHECK(atmega328p->pcints[0].pcmsk_address == 0x6BU);
    CHECK(atmega328p->pcints[0].pcicr_enable_mask == 0x01U);
    CHECK(atmega328p->pcints[0].vector_index == 3U);

    CHECK(atmega328p->pcints[1].pcmsk_address == 0x6CU);
    CHECK(atmega328p->pcints[1].pcicr_enable_mask == 0x02U);
    CHECK(atmega328p->pcints[1].vector_index == 4U);

    CHECK(atmega328p->pcints[2].pcmsk_address == 0x6DU);
    CHECK(atmega328p->pcints[2].pcicr_enable_mask == 0x04U);
    CHECK(atmega328p->pcints[2].vector_index == 5U);

    const auto& timer0 = atmega328p->timers8[0];
    CHECK(timer0.t_pin_address == 0x29U); // PIND
    CHECK(timer0.t_pin_bit == 4U);
    CHECK(timer0.ocra_pin_address == 0x2BU); // PORTD
    CHECK(timer0.ocra_pin_bit == 6U);
    CHECK(timer0.ocrb_pin_address == 0x2BU); // PORTD
    CHECK(timer0.ocrb_pin_bit == 5U);

    CHECK(timer2.ocra_pin_address == 0x25U); // PORTB
    CHECK(timer2.ocra_pin_bit == 3U);
    CHECK(timer2.ocrb_pin_address == 0x2BU); // PORTD
    CHECK(timer2.ocrb_pin_bit == 3U);
    CHECK(timer2.tosc1_pin_address == 0x23U); // PINB
    CHECK(timer2.tosc1_pin_bit == 6U);
    CHECK(timer2.tosc2_pin_address == 0x23U); // PINB
    CHECK(timer2.tosc2_pin_bit == 7U);
}
