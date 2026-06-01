#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/pin_mux.hpp"
#include <filesystem>

using namespace vioavr::core;

static constexpr u16 TCCR1A  = 0x80;
static constexpr u16 TCCR1B  = 0x81;
static constexpr u16 ICR1L   = 0x86;
static constexpr u16 ICR1H   = 0x87;
static constexpr u16 OCR1AL  = 0x88;
static constexpr u16 OCR1AH  = 0x89;
static constexpr u16 OCR1BL  = 0x8A;
static constexpr u16 OCR1BH  = 0x8B;
static constexpr u16 PORTB_DDR = 0x24;
static constexpr u16 PORTB_PORT = 0x25;
static constexpr u16 TCCR0A  = 0x44;
static constexpr u16 TCCR0B  = 0x45;
static constexpr u16 OCR0A   = 0x47;
static constexpr u16 TIMSK0  = 0x6E;
static constexpr u16 ADMUX   = 0x7C;
static constexpr u16 ADCSRA  = 0x7A;

static constexpr int TOP_VAL    = 799;
static constexpr int DEAD_TIME  = 8;
static constexpr int HALF_TOP   = 400;
static constexpr int OUTA_END   = HALF_TOP - DEAD_TIME;  // 392
static constexpr int OUTB_START = HALF_TOP + DEAD_TIME;  // 408

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

TEST_CASE("ATmega328P inverter firmware — register initialisation")
{
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);

    std::string hex = find_hex("atmega328p_inverter_pwm");
    if (hex.empty()) FAIL("atmega328p_inverter_pwm.hex not found");
    auto image = HexImageLoader::load_file(hex, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    // Run enough CPU cycles for init functions to execute
    machine->run(5000);

    // -- PWM registers ------------------------------------------------
    CHECK_MESSAGE(machine->bus().read_data(TCCR1A) == 0xB2,
                  "TCCR1A = " << (int)machine->bus().read_data(TCCR1A)
                  << " (expected 0xB2)");

    u8 tccr1b = machine->bus().read_data(TCCR1B);
    CHECK_MESSAGE((tccr1b & 0x07) == 1,
                  "TCCR1B CS bits = " << (tccr1b & 0x07) << " (expected 1)");
    CHECK_MESSAGE((tccr1b & 0x18) == 0x18,   // WGM12=1, WGM13=1
                  "TCCR1B WGM bits = " << (tccr1b & 0x18) << " (expected 0x18)");

    u8 lo = machine->bus().read_data(ICR1L);
    u8 hi = machine->bus().read_data(ICR1H);
    u16 icr1 = (hi << 8) | lo;
    CHECK_MESSAGE(icr1 == TOP_VAL, "ICR1 = " << icr1 << " (expected " << TOP_VAL << ")");

    lo = machine->bus().read_data(OCR1AL);
    hi = machine->bus().read_data(OCR1AH);
    u16 ocr1a = (hi << 8) | lo;
    CHECK_MESSAGE(ocr1a == OUTA_END, "OCR1A = " << ocr1a << " (expected " << OUTA_END << ")");

    lo = machine->bus().read_data(OCR1BL);
    hi = machine->bus().read_data(OCR1BH);
    u16 ocr1b = (hi << 8) | lo;
    CHECK_MESSAGE(ocr1b == OUTB_START, "OCR1B = " << ocr1b << " (expected " << OUTB_START << ")");

    // -- GPIO ---------------------------------------------------------
    CHECK_MESSAGE(machine->bus().read_data(PORTB_DDR) == 0x07,
                  "DDRB = " << (int)machine->bus().read_data(PORTB_DDR) << " (expected 0x07)");

    // -- LED timer ----------------------------------------------------
    CHECK_MESSAGE((machine->bus().read_data(TCCR0A) & 0x02) == 0x02,  // CTC via WGM01
                  "TCCR0A = " << (int)machine->bus().read_data(TCCR0A));

    u8 tccr0b = machine->bus().read_data(TCCR0B);
    CHECK_MESSAGE((tccr0b & 0x05) == 0x05,   // CS02=1, CS00=1 → /1024
                  "TCCR0B CS = " << (tccr0b & 0x07) << " (expected 5)");

    CHECK_MESSAGE(machine->bus().read_data(OCR0A) == 255,
                  "OCR0A = " << (int)machine->bus().read_data(OCR0A) << " (expected 255)");
    CHECK_MESSAGE(machine->bus().read_data(TIMSK0) == 0x02,  // OCIE0A
                  "TIMSK0 = " << (int)machine->bus().read_data(TIMSK0) << " (expected 0x02)");

    // -- ADC ----------------------------------------------------------
    CHECK_MESSAGE((machine->bus().read_data(ADMUX) & 0x40) == 0x40,  // REFS0=1
                  "ADMUX = " << (int)machine->bus().read_data(ADMUX));
    CHECK_MESSAGE((machine->bus().read_data(ADCSRA) & 0x80) != 0,   // ADEN=1
                  "ADCSRA = " << (int)machine->bus().read_data(ADCSRA));
}
