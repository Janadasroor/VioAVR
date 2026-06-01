#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/pin_mux.hpp"
#include <filesystem>

using namespace vioavr::core;

static constexpr u16 TCCR1A = 0x4F;
static constexpr u16 TCCR1B = 0x4E;
static constexpr u16 ICR1L  = 0x46;
static constexpr u16 OCR1AL = 0x4A;
static constexpr u16 OCR1BL = 0x48;
static constexpr u16 DDRB   = 0x37;

static constexpr u8 kCom1a1 = 0x80U;
static constexpr u8 kCom1b1 = 0x20U;
static constexpr u8 kCom1b0 = 0x10U;
static constexpr u8 kWgm11  = 0x02U;
static constexpr u8 kWgm13  = 0x10U;
static constexpr u8 kWgm12  = 0x08U;
static constexpr u8 kCs10   = 0x01U;

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

TEST_CASE("ATmega8 firmware — Timer1 Fast PWM with dead-time")
{
    auto machine = Machine::create_for_device("atmega8");
    REQUIRE(machine != nullptr);

    std::string hex = find_hex("atmega8_test");
    if (hex.empty()) FAIL("atmega8_test.hex not found");
    auto image = HexImageLoader::load_file(hex, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    machine->run(5000);

    u8 tccr1a = machine->bus().read_data(TCCR1A);
    CHECK_MESSAGE((tccr1a & (kCom1a1 | kCom1b1 | kCom1b0 | kWgm11)) ==
                  (kCom1a1 | kCom1b1 | kCom1b0 | kWgm11),
                  "TCCR1A = " << (int)tccr1a
                  << " (expected 0xB2, COM1A1|COM1B1|COM1B0|WGM11)");

    u8 tccr1b = machine->bus().read_data(TCCR1B);
    CHECK_MESSAGE((tccr1b & (kWgm13 | kWgm12 | kCs10)) == (kWgm13 | kWgm12 | kCs10),
                  "TCCR1B = " << (int)tccr1b
                  << " (expected 0x19, WGM13|WGM12|CS10)");

    auto read_ocr = [&machine](u16 low_addr) -> u16 {
        u8 lo = machine->bus().read_data(low_addr);
        u8 hi = machine->bus().read_data(low_addr + 1);
        return static_cast<u16>(hi) << 8 | lo;
    };

    CHECK_MESSAGE(read_ocr(ICR1L) == 799,
                  "ICR1 = " << read_ocr(ICR1L) << " (expected 799)");
    CHECK_MESSAGE(read_ocr(OCR1AL) == 392,
                  "OCR1A = " << read_ocr(OCR1AL) << " (expected 392)");
    CHECK_MESSAGE(read_ocr(OCR1BL) == 408,
                  "OCR1B = " << read_ocr(OCR1BL) << " (expected 408)");

    auto* pin_mux = machine->bus().pin_mux();
    REQUIRE(pin_mux != nullptr);
    auto st_pb1 = pin_mux->get_state_by_address(0x38, 1);
    auto st_pb2 = pin_mux->get_state_by_address(0x38, 2);
    CHECK_MESSAGE(st_pb1.owner == PinOwner::timer,
                  "PB1 owner = " << static_cast<int>(st_pb1.owner)
                  << " (expected " << static_cast<int>(PinOwner::timer) << ")");
    CHECK_MESSAGE(st_pb2.owner == PinOwner::timer,
                  "PB2 owner = " << static_cast<int>(st_pb2.owner)
                  << " (expected " << static_cast<int>(PinOwner::timer) << ")");
}
