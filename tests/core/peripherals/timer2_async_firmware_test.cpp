#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/hex_image.hpp"
#include <filesystem>

using namespace vioavr::core;

static constexpr u16 ASSR   = 0xB6;
static constexpr u16 TCCR2B = 0xB1;
static constexpr u16 TIMSK2 = 0x70;
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

TEST_CASE("Timer2 Async firmware — register initialisation")
{
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);

    std::string hex = find_hex("timer2_async_test");
    if (hex.empty()) FAIL("timer2_async_test.hex not found");
    auto image = HexImageLoader::load_file(hex, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    machine->run(5000);

    CHECK_MESSAGE((machine->bus().read_data(ASSR) & 0x20) != 0,
                  "ASSR AS2 = " << ((machine->bus().read_data(ASSR) >> 5) & 1)
                  << " (expected 1)");

    CHECK_MESSAGE((machine->bus().read_data(TCCR2B) & 0x07) == 0x05,
                  "TCCR2B CS = " << (machine->bus().read_data(TCCR2B) & 0x07)
                  << " (expected 5, prescaler /128)");

    CHECK_MESSAGE((machine->bus().read_data(TIMSK2) & 0x01) != 0,
                  "TIMSK2 TOIE2 = " << (machine->bus().read_data(TIMSK2) & 1)
                  << " (expected 1)");
}
