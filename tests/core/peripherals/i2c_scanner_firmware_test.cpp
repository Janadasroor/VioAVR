#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/hex_image.hpp"
#include <filesystem>

using namespace vioavr::core;

static constexpr u16 TWBR   = 0xB8;
static constexpr u16 TWSR   = 0xB9;
static constexpr u16 TWCR   = 0xBC;
static constexpr u16 UBRRL  = 0xC4;
static constexpr u16 UCSRB  = 0xC1;
static constexpr u16 UCSRC  = 0xC2;
static constexpr u16 DDRB   = 0x24;

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

TEST_CASE("I2C Scanner firmware — register initialisation")
{
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);

    std::string hex = find_hex("i2c_scanner_loopback");
    if (hex.empty()) FAIL("i2c_scanner_loopback.hex not found");
    auto image = HexImageLoader::load_file(hex, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    machine->run(5000);

    // TWI
    CHECK_MESSAGE(machine->bus().read_data(TWBR) == 72,
                  "TWBR = " << (int)machine->bus().read_data(TWBR)
                  << " (expected 72)");
    CHECK_MESSAGE((machine->bus().read_data(TWCR) & 0x04) != 0,
                  "TWCR TWEN = " << ((machine->bus().read_data(TWCR) >> 2) & 1)
                  << " (expected 1)");

    // USART
    CHECK_MESSAGE(machine->bus().read_data(UBRRL) == 103,
                  "UBRRL = " << (int)machine->bus().read_data(UBRRL)
                  << " (expected 103)");
    CHECK_MESSAGE((machine->bus().read_data(UCSRB) & 0x08) != 0,
                  "UCSRB TXEN = " << ((machine->bus().read_data(UCSRB) >> 3) & 1)
                  << " (expected 1)");
    CHECK_MESSAGE((machine->bus().read_data(UCSRC) & 0x06) == 0x06,
                  "UCSRC = " << (int)machine->bus().read_data(UCSRC)
                  << " (expected 0x06, 8-bit)");
}
