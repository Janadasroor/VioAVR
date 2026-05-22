#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/hex_image.hpp"
#include <filesystem>
#include <string>

namespace {
using namespace vioavr::core;
}

TEST_CASE("External blinky firmware - PORTB5 toggling") {
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);
    machine->reset();

    std::string hex_path = "tests/blinky.hex";
    if (!std::filesystem::exists(hex_path)) {
        hex_path = "../tests/blinky.hex";
    }
    if (!std::filesystem::exists(hex_path)) {
        hex_path = "../../tests/blinky.hex";
    }
    if (!std::filesystem::exists(hex_path)) {
        hex_path = "../../../tests/blinky.hex";
    }
    if (!std::filesystem::exists(hex_path)) {
        hex_path = "build/tests/blinky.hex";
    }
    if (!std::filesystem::exists(hex_path)) {
        FAIL("blinky.hex not found");
    }

    auto image = HexImageLoader::load_file(hex_path, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    constexpr u8 PORTB_ADDR = 0x25;
    constexpr u8 DDRB_ADDR = 0x24;

    // After reset, PORTB5 should be input, PORTB should be 0
    machine->run(100); // let it settle
    uint8_t ddrb_val = machine->bus().read_data(DDRB_ADDR);
    CHECK((ddrb_val & 0x20) != 0); // DDRB5 set to output by firmware

    // Run for ~50ms (0.8M cycles at 16MHz) - still in first ON phase
    machine->run(800000);
    uint8_t portb_val = machine->bus().read_data(PORTB_ADDR);
    CHECK((portb_val & 0x20) != 0); // PORTB5 = 1

    // Run for another ~100ms - should toggle OFF
    machine->run(1600000);
    portb_val = machine->bus().read_data(PORTB_ADDR);
    CHECK((portb_val & 0x20) == 0); // PORTB5 = 0

    // Run for another ~100ms - should toggle ON
    machine->run(1600000);
    portb_val = machine->bus().read_data(PORTB_ADDR);
    CHECK((portb_val & 0x20) != 0); // PORTB5 = 1

    // Run for another ~100ms - should toggle OFF
    machine->run(1600000);
    portb_val = machine->bus().read_data(PORTB_ADDR);
    CHECK((portb_val & 0x20) == 0); // PORTB5 = 0
}
