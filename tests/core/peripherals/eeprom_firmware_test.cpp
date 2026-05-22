#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/hex_image.hpp"
#include <filesystem>

using namespace vioavr::core;

static constexpr u16 EECR  = 0x3F;
static constexpr u16 EEDR  = 0x40;
static constexpr u16 EEARL = 0x41;
static constexpr u16 EEARH = 0x42;
static constexpr u16 DDRB  = 0x24;
static constexpr u16 PORTB = 0x25;

static constexpr u8 kEempe = 0x04U;
static constexpr u8 kEepe  = 0x02U;
static constexpr u8 kEere  = 0x01U;

static std::string find_hex(const std::string& name) {
    std::string paths[] = {
        "build/tests/" + name + ".hex",
        "../" + name + ".hex",
        "../../build/tests/" + name + ".hex",
    };
    for (auto& p : paths) {
        if (std::filesystem::exists(p)) return p;
    }
    return "";
}

TEST_CASE("EEPROM firmware — write/read cycle")
{
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);

    std::string hex = find_hex("eeprom_test");
    if (hex.empty()) FAIL("eeprom_test.hex not found");
    auto image = HexImageLoader::load_file(hex, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    machine->run(100000);

    u8 eecr = machine->bus().read_data(EECR);
    CHECK_MESSAGE((eecr & kEepe) == 0,
                  "EECR EEPE = " << (eecr & kEepe) << " (expected 0, write complete)");

    CHECK_MESSAGE(machine->bus().read_data(EEARL) == 0x10,
                  "EEARL = " << (int)machine->bus().read_data(EEARL)
                  << " (expected 0x10)");

    CHECK_MESSAGE(machine->bus().read_data(EEARH) == 0x00,
                  "EEARH = " << (int)machine->bus().read_data(EEARH)
                  << " (expected 0x00)");

    CHECK_MESSAGE(machine->bus().read_data(EEDR) == 0xA5,
                  "EEDR = " << (int)machine->bus().read_data(EEDR)
                  << " (expected 0xA5, read back)");

    CHECK_MESSAGE((machine->bus().read_data(PORTB) & 0x01) == 0x01,
                  "PORTB bit 0 = " << (machine->bus().read_data(PORTB) & 1)
                  << " (expected 1, match LED)");
}
