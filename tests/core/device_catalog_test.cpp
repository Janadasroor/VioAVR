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
