#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/hex_image.hpp"
#include <filesystem>

namespace {
using namespace vioavr::core;
}

TEST_CASE("spi_loopback firmware - SPI master transfer") {
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);
    machine->reset();

    std::string hex_path = "tests/spi_loopback.hex";
    if (!std::filesystem::exists(hex_path)) hex_path = "../spi_loopback.hex";
    if (!std::filesystem::exists(hex_path)) hex_path = "../../spi_loopback.hex";
    if (!std::filesystem::exists(hex_path)) hex_path = "build/tests/spi_loopback.hex";
    if (!std::filesystem::exists(hex_path)) FAIL("spi_loopback.hex not found");

    auto image = HexImageLoader::load_file(hex_path, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    constexpr u8 PORTB_ADDR = 0x25;
    constexpr u8 DDRB_ADDR = 0x24;
    constexpr u8 SPCR_ADDR = 0x4C;

    // Run boot + SPI init + 8-byte transfer
    machine->run(50000);

    // PB0 should be 1 (completion flag set after all transfers)
    uint8_t portb = machine->bus().read_data(PORTB_ADDR);
    CHECK((portb & 0x01) != 0);

    // SPI was initialized in master mode, fclk/4
    uint8_t spcr = machine->bus().read_data(SPCR_ADDR);
    CHECK((spcr & (1 << 6)) != 0);   // SPE=6: SPI enabled
    CHECK((spcr & (1 << 4)) != 0);   // MSTR=4: Master mode

    // DDRB configured: SCK(5), MOSI(3), SS(2) as outputs
    uint8_t ddrb = machine->bus().read_data(DDRB_ADDR);
    CHECK((ddrb & (1 << 5)) != 0);
    CHECK((ddrb & (1 << 3)) != 0);
    CHECK((ddrb & (1 << 2)) != 0);
}
