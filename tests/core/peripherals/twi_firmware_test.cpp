#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/hex_image.hpp"
#include <filesystem>

using namespace vioavr::core;

static constexpr u16 TWBR = 0xB8;
static constexpr u16 TWSR = 0xB9;
static constexpr u16 TWCR = 0xBC;

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

TEST_CASE("TWI firmware — register initialisation")
{
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);

    std::string hex = find_hex("twi_eeprom_test");
    if (hex.empty()) FAIL("twi_eeprom_test.hex not found");
    auto image = HexImageLoader::load_file(hex, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    machine->run(5000);

    CHECK_MESSAGE(machine->bus().read_data(TWBR) == 72,
                  "TWBR = " << (int)machine->bus().read_data(TWBR)
                  << " (expected 72)");

    CHECK_MESSAGE((machine->bus().read_data(TWSR) & 0x03) == 0,
                  "TWSR prescaler = " << (machine->bus().read_data(TWSR) & 0x03)
                  << " (expected 0, prescaler=1)");

    u8 twcr = machine->bus().read_data(TWCR);
    CHECK_MESSAGE((twcr & 0x04) != 0,
                  "TWCR TWEN = " << ((twcr >> 2) & 1) << " (expected 1)");
}
