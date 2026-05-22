#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/hex_image.hpp"
#include <filesystem>

namespace {
using namespace vioavr::core;
}

TEST_CASE("wdt_test firmware - Watchdog timeout triggers CPU reset") {
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);
    machine->reset();

    std::string hex_path = "tests/wdt_test.hex";
    if (!std::filesystem::exists(hex_path)) hex_path = "../wdt_test.hex";
    if (!std::filesystem::exists(hex_path)) hex_path = "../../wdt_test.hex";
    if (!std::filesystem::exists(hex_path)) hex_path = "build/tests/wdt_test.hex";
    if (!std::filesystem::exists(hex_path)) FAIL("wdt_test.hex not found");

    auto image = HexImageLoader::load_file(hex_path, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    constexpr u8 PORTB_ADDR = 0x25;
    constexpr u8 MCUSR_ADDR = 0x54;

    // Step past boot - PORTB should be 0x55 (power-on reset path)
    for (int i = 0; i < 5000; ++i) machine->step();
    CHECK(machine->bus().read_data(PORTB_ADDR) == 0x55);

    // Step past WDT timeout (~256k CPU cycles, ~128k steps of rjmp)
    // After WDT fires: CPU resets → firmware reboots → detects WDRF → PORTB = 0xAA
    for (int i = 0; i < 300000; ++i) machine->step();

    uint8_t pb = machine->bus().read_data(PORTB_ADDR);
    CHECK(pb == 0xAA);

    // MCUSR WDRF persists (write-0 doesn't clear write-1-to-clear flags)
    CHECK(machine->bus().read_data(MCUSR_ADDR) == 0x08);
}
