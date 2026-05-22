#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/pin_mux.hpp"
#include <filesystem>

using namespace vioavr::core;

static constexpr u16 TCA0_CTRLA = 0x0A00;
static constexpr u16 TCA0_CTRLB = 0x0A01;
static constexpr u16 TCA0_PER   = 0x0A26;
static constexpr u16 TCA0_CMP0  = 0x0A28;
static constexpr u16 PORTA_DIR  = 0x0400;

static constexpr u8 kEnableMask   = 0x01U;
static constexpr u8 kCmp0enMask   = 0x10U;
static constexpr u8 kWgmodeMask   = 0x07U;
static constexpr u8 kSingleslope  = 0x03U;

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

TEST_CASE("ATmega808 firmware — TCA0 single-slope PWM init")
{
    auto machine = Machine::create_for_device("ATmega808");
    REQUIRE(machine != nullptr);

    std::string hex = find_hex("atmega808_test");
    if (hex.empty()) FAIL("atmega808_test.hex not found");
    auto image = HexImageLoader::load_file(hex, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    machine->run(5000);

    u8 ctrla = machine->bus().read_data(TCA0_CTRLA);
    CHECK_MESSAGE((ctrla & kEnableMask) != 0,
                  "TCA0 CTRLA ENABLE = " << (ctrla & kEnableMask)
                  << " (expected 1)");

    u8 ctrlb = machine->bus().read_data(TCA0_CTRLB);
    CHECK_MESSAGE((ctrlb & kCmp0enMask) != 0,
                  "TCA0 CTRLB CMP0EN = " << ((ctrlb >> 4) & 1)
                  << " (expected 1)");
    CHECK_MESSAGE((ctrlb & kWgmodeMask) == kSingleslope,
                  "TCA0 CTRLB WGMODE = " << (ctrlb & kWgmodeMask)
                  << " (expected 3, single-slope)");

    auto read16 = [&machine](u16 low_addr) -> u16 {
        u8 lo = machine->bus().read_data(low_addr);
        u8 hi = machine->bus().read_data(low_addr + 1);
        return static_cast<u16>(hi) << 8 | lo;
    };

    CHECK_MESSAGE(read16(TCA0_PER) == 500,
                  "TCA0 PER = " << read16(TCA0_PER) << " (expected 500)");
    CHECK_MESSAGE(read16(TCA0_CMP0) == 250,
                  "TCA0 CMP0 = " << read16(TCA0_CMP0) << " (expected 250)");

    CHECK_MESSAGE((machine->bus().read_data(PORTA_DIR) & 0x01) != 0,
                  "PORTA_DIR bit 0 = " << (machine->bus().read_data(PORTA_DIR) & 1)
                  << " (expected 1)");

    auto* pin_mux = machine->bus().pin_mux();
    REQUIRE(pin_mux != nullptr);
    auto st_pa0 = pin_mux->get_state_by_address(0x0404, 0);
    CHECK_MESSAGE(st_pa0.owner == PinOwner::timer,
                  "PA0 owner = " << static_cast<int>(st_pa0.owner)
                  << " (expected " << static_cast<int>(PinOwner::timer) << ")");
}
