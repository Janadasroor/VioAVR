#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/hex_image.hpp"
#include <filesystem>

using namespace vioavr::core;

static constexpr u16 ACSR = 0x50;
static constexpr u16 DDRB = 0x24;

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

TEST_CASE("Analog Comparator firmware — register initialisation")
{
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);

    std::string hex = find_hex("analog_comparator_test");
    if (hex.empty()) FAIL("analog_comparator_test.hex not found");
    auto image = HexImageLoader::load_file(hex, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    machine->run(5000);

    u8 acsr = machine->bus().read_data(ACSR);
    CHECK_MESSAGE((acsr & 0x40) != 0,   // ACBG
                  "ACSR ACBG = " << ((acsr >> 6) & 1) << " (expected 1)");
    CHECK_MESSAGE((acsr & 0x08) != 0,   // ACIE
                  "ACSR ACIE = " << ((acsr >> 3) & 1) << " (expected 1)");
    CHECK_MESSAGE((acsr & 0x03) == 0x03, // ACIS1|ACIS0
                  "ACSR ACIS = " << (acsr & 0x03) << " (expected 0x03, toggle on change)");
}
