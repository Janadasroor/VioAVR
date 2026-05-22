#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/pin_mux.hpp"
#include <filesystem>

using namespace vioavr::core;

static constexpr u16 PSOC0  = 0xD0;
static constexpr u16 OCR0SA = 0xD2;
static constexpr u16 OCR0RA = 0xD4;
static constexpr u16 OCR0SB = 0xD6;
static constexpr u16 OCR0RB = 0xD8;
static constexpr u16 PCNF0  = 0xDA;
static constexpr u16 PCTL0  = 0xDB;
static constexpr u16 PFRC0A = 0xDC;
static constexpr u16 PORTD  = 0x2B;

static constexpr u16 kPrunMask   = 0x01U;
static constexpr u16 kPoenaMask  = 0x01U;
static constexpr u16 kPoenbMask  = 0x04U;
static constexpr u16 kPiselMask  = 0x04U;

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

TEST_CASE("PSC0 firmware — one-ramp mode init")
{
    auto machine = Machine::create_for_device("AT90PWM316");
    REQUIRE(machine != nullptr);

    std::string hex = find_hex("psc_test");
    if (hex.empty()) FAIL("psc_test.hex not found");
    auto image = HexImageLoader::load_file(hex, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    machine->run(5000);

    u8 psoc0 = machine->bus().read_data(PSOC0);
    CHECK_MESSAGE((psoc0 & kPoenaMask) != 0,
                  "PSOC0 POENA = " << (psoc0 & kPoenaMask) << " (expected 1)");
    CHECK_MESSAGE((psoc0 & kPoenbMask) != 0,
                  "PSOC0 POENB = " << ((psoc0 >> 2) & 1) << " (expected 1)");

    CHECK_MESSAGE(machine->bus().read_data(PCNF0) == 0,
                  "PCNF0 = " << (int)machine->bus().read_data(PCNF0)
                  << " (expected 0, one-ramp mode, sysclk)");

    u8 pfrc0a = machine->bus().read_data(PFRC0A);
    CHECK_MESSAGE((pfrc0a & kPiselMask) != 0,
                  "PFRC0A PISEL = " << ((pfrc0a >> 2) & 1) << " (expected 1)");

    u8 pctl0 = machine->bus().read_data(PCTL0);
    CHECK_MESSAGE((pctl0 & kPrunMask) != 0,
                  "PCTL0 PRUN = " << (pctl0 & kPrunMask) << " (expected 1)");

    auto read_ocr = [&machine](u16 low_addr) -> u16 {
        u8 lo = machine->bus().read_data(low_addr);
        u8 hi = machine->bus().read_data(low_addr + 1);
        return static_cast<u16>(hi) << 8 | lo;
    };

    CHECK_MESSAGE(read_ocr(OCR0SA) == 0,
                  "OCR0SA = " << read_ocr(OCR0SA) << " (expected 0)");
    CHECK_MESSAGE(read_ocr(OCR0RA) == 250,
                  "OCR0RA = " << read_ocr(OCR0RA) << " (expected 250)");
    CHECK_MESSAGE(read_ocr(OCR0SB) == 260,
                  "OCR0SB = " << read_ocr(OCR0SB) << " (expected 260)");
    CHECK_MESSAGE(read_ocr(OCR0RB) == 500,
                  "OCR0RB = " << read_ocr(OCR0RB) << " (expected 500)");

    auto* pin_mux = machine->bus().pin_mux();
    REQUIRE(pin_mux != nullptr);
    auto st_pd0 = pin_mux->get_state_by_address(PORTD, 0);
    auto st_pd1 = pin_mux->get_state_by_address(PORTD, 1);
    CHECK_MESSAGE(st_pd0.owner == PinOwner::psc,
                  "PD0 owner = " << static_cast<int>(st_pd0.owner)
                  << " (expected " << static_cast<int>(PinOwner::psc) << ")");
    CHECK_MESSAGE(st_pd1.owner == PinOwner::psc,
                  "PD1 owner = " << static_cast<int>(st_pd1.owner)
                  << " (expected " << static_cast<int>(PinOwner::psc) << ")");
}
