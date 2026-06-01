#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/hex_image.hpp"
#include <filesystem>

using namespace vioavr::core;

static constexpr u16 PCICR  = 0x68;
static constexpr u16 PCMSK1 = 0x6C;
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

TEST_CASE("Pin Change Interrupt firmware — register initialisation")
{
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);

    std::string hex = find_hex("pin_change_test");
    if (hex.empty()) FAIL("pin_change_test.hex not found");
    auto image = HexImageLoader::load_file(hex, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    machine->run(5000);

    CHECK_MESSAGE(machine->bus().read_data(PCICR) == 0x02,
                  "PCICR = " << (int)machine->bus().read_data(PCICR)
                  << " (expected 0x02, PCIE1=1)");

    CHECK_MESSAGE(machine->bus().read_data(PCMSK1) == 0x01,
                  "PCMSK1 = " << (int)machine->bus().read_data(PCMSK1)
                  << " (expected 0x01, PCINT8=1)");

    CHECK_MESSAGE((machine->bus().read_data(DDRB) & 0x01) == 0x01,
                  "DDRB bit 0 = " << (machine->bus().read_data(DDRB) & 1)
                  << " (expected 1, LED output)");
}
