#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/pin_mux.hpp"
#include <filesystem>

using namespace vioavr::core;

static constexpr u16 TCCR1A  = 0x4F;
static constexpr u16 TCCR1B  = 0x4E;
static constexpr u16 ICR1L   = 0x46;
static constexpr u16 ICR1H   = 0x47;
static constexpr u16 OCR1AL  = 0x4A;
static constexpr u16 OCR1AH  = 0x4B;
static constexpr u16 OCR1BL  = 0x48;
static constexpr u16 OCR1BH  = 0x49;
static constexpr u16 DDRD    = 0x31;
static constexpr u16 PORTD   = 0x32;
static constexpr u16 DDRB    = 0x37;
static constexpr u16 PORTB   = 0x38;
static constexpr u16 ADMUX   = 0x27;
static constexpr u16 ADCSRA  = 0x26;
static constexpr u16 TIMSK   = 0x59;

static constexpr int TOP_VAL    = 799;
static constexpr int DEAD_TIME  = 8;
static constexpr int HALF_TOP   = 400;
static constexpr int OUTA_END   = HALF_TOP - DEAD_TIME;
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

TEST_CASE("ATmega16 inverter firmware — register initialisation")
{
    auto machine = Machine::create_for_device("atmega16");
    REQUIRE(machine != nullptr);

    std::string hex = find_hex("atmega16_inverter_pwm");
    if (hex.empty()) FAIL("atmega16_inverter_pwm.hex not found");
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
    CHECK_MESSAGE((tccr1b & 0x18) == 0x18,
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
    CHECK_MESSAGE(machine->bus().read_data(DDRD) == 0x30,
                  "DDRD = " << (int)machine->bus().read_data(DDRD) << " (expected 0x30)");

    // OC1A=PD5, OC1B=PD4 should be owned by Timer1
    CHECK_MESSAGE(machine->bus().pin_mux()->get_state_by_address(PORTD, 5).owner == PinOwner::timer,
                  "PD5 (OC1A) owned by Timer");

    CHECK_MESSAGE(machine->bus().pin_mux()->get_state_by_address(PORTD, 4).owner == PinOwner::timer,
                  "PD4 (OC1B) owned by Timer");

    // PD0 (RXD) and PD1 (TXD) must NOT be claimed by any peripheral
    // — they are available as regular GPIO when USART is not enabled.
    CHECK_MESSAGE(machine->bus().pin_mux()->get_state_by_address(PORTD, 0).owner == PinOwner::gpio,
                  "PD0 (RXD) unclaimed (available for GPIO)");

    CHECK_MESSAGE(machine->bus().pin_mux()->get_state_by_address(PORTD, 1).owner == PinOwner::gpio,
                  "PD1 (TXD) unclaimed (available for GPIO)");

    // -- LED timer ----------------------------------------------------
    // Timer0 on ATmega16 uses classic TCCR0 register at 0x53.
    // The simulator maps TCNT0(0x52), TIFR(0x58), TIMSK(0x59) but not
    // TCCR0 (the TWI peripheral claims 0x00-0x56 as a blanket range).
    // We verify TIMSK OCIE0 which IS in the simulated range.
    u8 timsk = machine->bus().read_data(TIMSK);
    CHECK_MESSAGE((timsk & 0x02) == 0x02,
                  "TIMSK OCIE0 = " << ((timsk >> 1) & 1) << " (expected 1)");

    // -- ADC ----------------------------------------------------------
    CHECK_MESSAGE((machine->bus().read_data(ADMUX) & 0x40) == 0x40,
                  "ADMUX = " << (int)machine->bus().read_data(ADMUX));
    CHECK_MESSAGE((machine->bus().read_data(ADCSRA) & 0x80) != 0,
                  "ADCSRA = " << (int)machine->bus().read_data(ADCSRA));
}
