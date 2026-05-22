#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/hex_image.hpp"
#include <filesystem>

namespace {
using namespace vioavr::core;
}

TEST_CASE("test0 firmware - fast PORTB toggle") {
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);
    machine->reset();

    std::string hex_path = "tests/test0.hex";
    if (!std::filesystem::exists(hex_path)) hex_path = "../test0.hex";
    if (!std::filesystem::exists(hex_path)) hex_path = "../../test0.hex";
    if (!std::filesystem::exists(hex_path)) hex_path = "build/tests/test0.hex";
    if (!std::filesystem::exists(hex_path)) FAIL("test0.hex not found");

    auto image = HexImageLoader::load_file(hex_path, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    constexpr u8 PORTB_ADDR = 0x25;
    constexpr u8 DDRB_ADDR = 0x24;

    machine->run(50);
    CHECK(machine->bus().read_data(DDRB_ADDR) == 0xFF);
    CHECK(machine->bus().read_data(PORTB_ADDR) == 0x55);

    // Toggle loop: 5 cycles per toggle (out+in+com+rjmp)
    // After an odd number of toggles, value flips
    // Run 4 toggles worth of cycles (even) → still 0x55
    machine->run(20);
    CHECK(machine->bus().read_data(PORTB_ADDR) == 0x55);

    // Run 1 more toggle (5 cycles) → odd count → 0xAA
    machine->run(5);
    CHECK(machine->bus().read_data(PORTB_ADDR) == 0xAA);

    // Run 2 toggles (10 cycles) → 0xAA again (even from current)
    machine->run(10);
    CHECK(machine->bus().read_data(PORTB_ADDR) == 0xAA);

    // Run 1 toggle → 0x55
    machine->run(5);
    CHECK(machine->bus().read_data(PORTB_ADDR) == 0x55);
}
