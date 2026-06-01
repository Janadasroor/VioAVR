#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/hex_image.hpp"
#include <filesystem>

using namespace vioavr::core;

static constexpr u16 SPCR  = 0x4C;
static constexpr u16 SPSR  = 0x4D;
static constexpr u16 SPDR  = 0x4E;
static constexpr u16 PORTB = 0x25;

static constexpr u8 kSpe   = 0x40U;
static constexpr u8 kMstr  = 0x10U;
static constexpr u8 kSpif  = 0x80U;
static constexpr u8 kWcol  = 0x40U;

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

TEST_CASE("SPI firmware — master mode init and transmission")
{
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);

    std::string hex = find_hex("spi_test");
    if (hex.empty()) FAIL("spi_test.hex not found");
    auto image = HexImageLoader::load_file(hex, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    machine->run(5000);

    u8 spcr = machine->bus().read_data(SPCR);
    CHECK_MESSAGE((spcr & kSpe) != 0,
                  "SPCR SPE = " << ((spcr >> 6) & 1) << " (expected 1)");
    CHECK_MESSAGE((spcr & kMstr) != 0,
                  "SPCR MSTR = " << ((spcr >> 4) & 1) << " (expected 1)");

    u8 spsr = machine->bus().read_data(SPSR);
    CHECK_MESSAGE((spsr & kSpif) == 0,
                  "SPSR SPIF = " << ((spsr >> 7) & 1)
                  << " (expected 0, cleared by SPDR read)");

    CHECK_MESSAGE((machine->bus().read_data(PORTB) & 0x01) != 0,
                  "PORTB bit 0 = " << (machine->bus().read_data(PORTB) & 1)
                  << " (expected 1, transmission complete LED)");
}
