#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/devices/atmega328.hpp"

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

    CHECK(atmega328p->timer2.tccra_address == 0xB0U);
    CHECK(atmega328p->timer2.tccrb_address == 0xB1U);
    CHECK(atmega328p->timer2.tcnt_address == 0xB2U);
    CHECK(atmega328p->timer2.ocra_address == 0xB3U);
    CHECK(atmega328p->timer2.ocrb_address == 0xB4U);
    CHECK(atmega328p->timer2.assr_address == 0xB6U);
    CHECK(atmega328p->timer2.timsk_address == 0x70U);
    CHECK(atmega328p->timer2.tifr_address == 0x37U);
    CHECK(atmega328p->timer2.compare_a_vector_index == 7U);
    CHECK(atmega328p->timer2.compare_b_vector_index == 8U);
    CHECK(atmega328p->timer2.overflow_vector_index == 9U);

    CHECK(atmega328p->pin_change_interrupt_0.pcicr_address == 0x68U);
    CHECK(atmega328p->pin_change_interrupt_0.pcifr_address == 0x3BU);
    CHECK(atmega328p->pin_change_interrupt_0.pcmsk_address == 0x6BU);
    CHECK(atmega328p->pin_change_interrupt_0.pcicr_enable_mask == 0x01U);
    CHECK(atmega328p->pin_change_interrupt_0.pcifr_flag_mask == 0x01U);
    CHECK(atmega328p->pin_change_interrupt_0.vector_index == 3U);

    CHECK(atmega328p->pin_change_interrupt_1.pcmsk_address == 0x6CU);
    CHECK(atmega328p->pin_change_interrupt_1.pcicr_enable_mask == 0x02U);
    CHECK(atmega328p->pin_change_interrupt_1.pcifr_flag_mask == 0x02U);
    CHECK(atmega328p->pin_change_interrupt_1.vector_index == 4U);

    CHECK(atmega328p->pin_change_interrupt_2.pcmsk_address == 0x6DU);
    CHECK(atmega328p->pin_change_interrupt_2.pcicr_enable_mask == 0x04U);
    CHECK(atmega328p->pin_change_interrupt_2.pcifr_flag_mask == 0x04U);
    CHECK(atmega328p->pin_change_interrupt_2.vector_index == 5U);
}
