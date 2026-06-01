#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/hex_image.hpp"
#include <filesystem>

namespace {
using namespace vioavr::core;
}

TEST_CASE("test_interrupt firmware - Timer0 overflow ISR") {
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);
    machine->reset();

    std::string hex_path = "tests/test_interrupt.hex";
    if (!std::filesystem::exists(hex_path)) hex_path = "../test_interrupt.hex";
    if (!std::filesystem::exists(hex_path)) hex_path = "../../test_interrupt.hex";
    if (!std::filesystem::exists(hex_path)) hex_path = "build/tests/test_interrupt.hex";
    if (!std::filesystem::exists(hex_path)) FAIL("test_interrupt.hex not found");

    auto image = HexImageLoader::load_file(hex_path, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    constexpr u8 PORTB_ADDR = 0x25;

    // Firmware: Timer0 normal mode, prescaler=1 (CS00), overflows every 256 cycles.
    // ISR increments 'count' and writes to PORTB. After N overflows, PORTB = N & 0xFF.
    //
    // Timer0 counts 0→255→0. Each overflow takes 256 cycles.
    // ISR entry/exit overhead ~10 cycles.
    // So each PORTB update every ~266 cycles.

    // Timer0 counts 0→255→0, overflow every 256 timer cycles.
    // Timer starts at ~cycle 29 from reset. ISR entry+body to PORTB write ~21 cycles.
    // PORTB = N written at approximately: 29 + N*256 + 21.
    //
    // PORTB=3 at ~818, PORTB=4 at ~1074 → check at 1000
    // PORTB=13 at ~3378, PORTB=14 at ~3634 → check at 3500
    // PORTB=113 at ~28978, PORTB=114 at ~29234 → check at 29000

    machine->run(1000);
    uint8_t pb = machine->bus().read_data(PORTB_ADDR);
    CHECK(pb == 3);

    machine->run(2500);
    pb = machine->bus().read_data(PORTB_ADDR);
    CHECK(pb == 13);

    machine->run(25500);
    pb = machine->bus().read_data(PORTB_ADDR);
    CHECK(pb == 113);
}
