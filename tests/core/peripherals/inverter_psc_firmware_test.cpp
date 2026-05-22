#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "signal_tracer.hpp"
#include <filesystem>

using namespace vioavr::core;

static constexpr u16 PSOC0  = 0xD0;
static constexpr u16 PCNF0  = 0xDA;
static constexpr u16 PFRC0A = 0xDC;
static constexpr u16 PFRC0B = 0xDD;
static constexpr u16 PCTL0  = 0xDB;
static constexpr u16 OCR0SA = 0xD2;
static constexpr u16 OCR0RA = 0xD4;
static constexpr u16 OCR0SB = 0xD6;
static constexpr u16 OCR0RB = 0xD8;
static constexpr u16 TCCR0A = 0x44;
static constexpr u16 TCCR0B = 0x45;
static constexpr u16 OCR0A  = 0x47;
static constexpr u16 TIMSK0 = 0x6E;
static constexpr u16 ADMUX  = 0x7C;
static constexpr u16 ADCSRA = 0x7A;
static constexpr u16 DDRB   = 0x24;
static constexpr u16 PORTB  = 0x25;
static constexpr u16 DDRD   = 0x2A;
static constexpr u16 PORTD  = 0x2B;
static constexpr u16 PORTD_PIN = 0x29;

static constexpr int TOP_VAL   = 800;
static constexpr int DEAD_TIME = 8;
static constexpr int HALF_TOP  = 400;
static constexpr int OUTA_END  = HALF_TOP - DEAD_TIME;
static constexpr int OUTB_START = HALF_TOP + DEAD_TIME;

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

TEST_CASE("PSC inverter firmware — register initialisation")
{
    auto machine = Machine::create_for_device("AT90PWM316");
    REQUIRE(machine != nullptr);

    std::string hex = find_hex("inverter_psc_pwm");
    if (hex.empty()) FAIL("inverter_psc_pwm.hex not found");
    auto image = HexImageLoader::load_file(hex, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    machine->run(5000);

    // -- PSC registers --------------------------------------------------
    CHECK_MESSAGE(machine->bus().read_data(PSOC0) == 0x05,
                  "PSOC0 = " << (int)machine->bus().read_data(PSOC0)
                  << " (expected 0x05)");
    CHECK_MESSAGE(machine->bus().read_data(PFRC0A) == 0x04,
                  "PFRC0A = " << (int)machine->bus().read_data(PFRC0A)
                  << " (expected 0x04)");
    CHECK_MESSAGE(machine->bus().read_data(PCNF0) == 0x00,
                  "PCNF0 = " << (int)machine->bus().read_data(PCNF0)
                  << " (expected 0x00)");
    CHECK_MESSAGE((machine->bus().read_data(PCTL0) & 0x01) == 0x01,
                  "PCTL0 PRUN = " << (int)(machine->bus().read_data(PCTL0) & 0x01)
                  << " (expected 1)");

    // PSC OCR registers require low-byte read first (latches high byte)
    auto read_ocr = [&](u16 addr) -> u16 {
        u8 lo = machine->bus().read_data(addr);     // read low → latches high
        u8 hi = machine->bus().read_data(addr + 1); // read high from latch
        return (u16(hi) << 8) | lo;
    };

    u16 ocr0rb = read_ocr(OCR0RB);
    CHECK_MESSAGE(ocr0rb == TOP_VAL,
                  "OCR0RB = " << ocr0rb << " (expected " << TOP_VAL << ")");

    u16 ocr0sa = read_ocr(OCR0SA);
    CHECK_MESSAGE(ocr0sa == 0,
                  "OCR0SA = " << ocr0sa << " (expected 0)");

    u16 ocr0ra = read_ocr(OCR0RA);
    CHECK_MESSAGE(ocr0ra == OUTA_END,
                  "OCR0RA = " << ocr0ra << " (expected " << OUTA_END << ")");

    u16 ocr0sb = read_ocr(OCR0SB);
    CHECK_MESSAGE(ocr0sb == OUTB_START,
                  "OCR0SB = " << ocr0sb << " (expected " << OUTB_START << ")");

    // -- GPIO ---------------------------------------------------------
    CHECK_MESSAGE((machine->bus().read_data(DDRB) & 0x08) == 0x08,
                  "DDRB bit 3 = " << ((machine->bus().read_data(DDRB) >> 3) & 1)
                  << " (expected 1)");

    CHECK_MESSAGE(machine->bus().pin_mux()->get_state_by_address(PORTD_PIN, 0).owner == PinOwner::psc,
                  "PD0 owned by PSC");
    CHECK_MESSAGE(machine->bus().pin_mux()->get_state_by_address(PORTD_PIN, 1).owner == PinOwner::psc,
                  "PD1 owned by PSC");

    // -- LED timer ----------------------------------------------------
    CHECK_MESSAGE((machine->bus().read_data(TCCR0A) & 0x02) == 0x02,
                  "TCCR0A = " << (int)machine->bus().read_data(TCCR0A));

    u8 tccr0b = machine->bus().read_data(TCCR0B);
    CHECK_MESSAGE((tccr0b & 0x05) == 0x05,
                  "TCCR0B CS = " << (tccr0b & 0x07) << " (expected 5)");
    CHECK_MESSAGE(machine->bus().read_data(OCR0A) == 255,
                  "OCR0A = " << (int)machine->bus().read_data(OCR0A)
                  << " (expected 255)");
    CHECK_MESSAGE(machine->bus().read_data(TIMSK0) == 0x02,
                  "TIMSK0 = " << (int)machine->bus().read_data(TIMSK0)
                  << " (expected 0x02)");

    // -- ADC ----------------------------------------------------------
    CHECK_MESSAGE((machine->bus().read_data(ADMUX) & 0x40) == 0x40,
                  "ADMUX REFS0 = " << ((machine->bus().read_data(ADMUX) >> 6) & 1)
                  << " (expected 1)");
    CHECK_MESSAGE((machine->bus().read_data(ADCSRA) & 0x80) != 0,
                  "ADCSRA ADEN = " << ((machine->bus().read_data(ADCSRA) >> 7) & 1)
                  << " (expected 1)");
}

TEST_CASE("PSC inverter firmware — waveform and dead time")
{
    auto machine = Machine::create_for_device("AT90PWM316");
    REQUIRE(machine != nullptr);

    std::string hex = find_hex("inverter_psc_pwm");
    if (hex.empty()) FAIL("inverter_psc_pwm.hex not found");
    auto image = HexImageLoader::load_file(hex, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    machine->run(20000);

    // Read back PSC registers to verify firmware initialization
    auto read_ocr = [&](u16 addr) -> u16 {
        u8 lo = machine->bus().read_data(addr);
        u8 hi = machine->bus().read_data(addr + 1);
        return (u16(hi) << 8) | lo;
    };
    MESSAGE("OCR0SA=" << read_ocr(OCR0SA)
            << " OCR0RA=" << read_ocr(OCR0RA)
            << " OCR0SB=" << read_ocr(OCR0SB)
            << " OCR0RB=" << read_ocr(OCR0RB)
            << " PFRC0B=" << (int)machine->bus().read_data(PFRC0B)
            << " PSOC0=" << (int)machine->bus().read_data(PSOC0)
            << " PCNF0=" << (int)machine->bus().read_data(PCNF0)
            << " PCTL0=" << (int)machine->bus().read_data(PCTL0));

    // After set_duty(5): pulse_width = 5 * 384 / 100 = 19, start_a = 392 - 19 = 373
    CHECK(read_ocr(OCR0RA) == OUTA_END);
    CHECK(read_ocr(OCR0SB) == OUTB_START);
    CHECK(read_ocr(OCR0RB) == TOP_VAL);
}
