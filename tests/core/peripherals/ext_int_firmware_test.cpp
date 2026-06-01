#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/hex_image.hpp"
#include <filesystem>

using namespace vioavr::core;

static constexpr u16 EICRA = 0x69;
static constexpr u16 EIMSK = 0x3D;
static constexpr u16 DDRB  = 0x24;
static constexpr u16 PORTB = 0x25;

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

TEST_CASE("External Interrupt firmware — register initialisation")
{
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);

    std::string hex = find_hex("ext_int_test");
    if (hex.empty()) FAIL("ext_int_test.hex not found");
    auto image = HexImageLoader::load_file(hex, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    machine->run(5000);

    CHECK_MESSAGE(machine->bus().read_data(EICRA) == 0x02,
                  "EICRA = " << (int)machine->bus().read_data(EICRA)
                  << " (expected 0x02, ISC01=1)");

    CHECK_MESSAGE(machine->bus().read_data(EIMSK) == 0x01,
                  "EIMSK = " << (int)machine->bus().read_data(EIMSK)
                  << " (expected 0x01, INT0=1)");

    CHECK_MESSAGE((machine->bus().read_data(DDRB) & 0x01) == 0x01,
                  "DDRB bit 0 = " << (machine->bus().read_data(DDRB) & 1)
                  << " (expected 1, LED output)");
}
