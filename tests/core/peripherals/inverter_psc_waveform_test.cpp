#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/psc.hpp"
#include "signal_tracer.hpp"

using namespace vioavr::core;

static constexpr u16 PSOC0  = 0xD0;
static constexpr u16 PCNF0  = 0xDA;
static constexpr u16 PFRC0A = 0xDC;
static constexpr u16 PCTL0  = 0xDB;
static constexpr u16 OCR0SA = 0xD2;
static constexpr u16 OCR0RA = 0xD4;
static constexpr u16 OCR0SB = 0xD6;
static constexpr u16 OCR0RB = 0xD8;

static constexpr u16 PORTD_PIN = 0x29;

static constexpr int TOP_VAL    = 800;
static constexpr int DEAD_TIME  = 8;
static constexpr int HALF_TOP   = TOP_VAL / 2;
static constexpr int OUTA_START = 0;
static constexpr int OUTA_END   = HALF_TOP - DEAD_TIME;
static constexpr int OUTB_START = HALF_TOP + DEAD_TIME;
static constexpr int OUTB_END   = TOP_VAL;

TEST_CASE("Inverter PSC - Initial waveform and dead time")
{
    auto machine = Machine::create_for_device("AT90PWM316");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();

    // Configure exactly like the firmware's init_psc()
    bus.write_data(PSOC0, (1 << 0) | (1 << 2));
    bus.write_data(PFRC0A, (1 << 2));

    bus.write_data(OCR0RB + 1, TOP_VAL >> 8);
    bus.write_data(OCR0RB,     TOP_VAL & 0xFF);

    bus.write_data(OCR0SA + 1, 0);
    bus.write_data(OCR0SA,     0);
    bus.write_data(OCR0RA + 1, OUTA_END >> 8);
    bus.write_data(OCR0RA,     OUTA_END & 0xFF);
    bus.write_data(OCR0SB + 1, OUTB_START >> 8);
    bus.write_data(OCR0SB,     OUTB_START & 0xFF);

    // PRUN=1 — write_data calls Psc::write() which calls update_outputs()
    // with counter still at 0. State is now valid for counter=0.
    bus.write_data(PCTL0, 1);

    CHECK(bus.pin_mux()->get_state_by_address(PORTD_PIN, 0).owner == PinOwner::psc);
    CHECK(bus.pin_mux()->get_state_by_address(PORTD_PIN, 1).owner == PinOwner::psc);

    vcd::SignalTracer tracer;
    tracer.add_signal("OutA");
    tracer.add_signal("OutB");

    // Walk one full cycle: check state BEFORE each tick (state = counter = c)
    int last_a = -1, last_b = -1;
    int dead_a_end  = -1;
    int dead_b_start = -1;
    bool pass = true;

    for (int c = 0; c < TOP_VAL; ++c) {
        int a = bus.pin_mux()->get_state_by_address(PORTD_PIN, 0).drive_level ? 1 : 0;
        int b = bus.pin_mux()->get_state_by_address(PORTD_PIN, 1).drive_level ? 1 : 0;

        tracer.record("OutA", c, (bool)a);
        tracer.record("OutB", c, (bool)b);

        if (last_a == 1 && a == 0) dead_a_end = c;
        if (last_b == 0 && b == 1) dead_b_start = c;

        last_a = a; last_b = b;

        bool expect_a = (c >= OUTA_START && c < OUTA_END);
        bool expect_b = (c >= OUTB_START && c < OUTB_END);

        if (a != (expect_a ? 1 : 0)) {
            FAIL("Counter=" << c << " OutA: got " << a << " expected " << (expect_a ? 1 : 0));
            pass = false;
        }
        if (b != (expect_b ? 1 : 0)) {
            FAIL("Counter=" << c << " OutB: got " << b << " expected " << (expect_b ? 1 : 0));
            pass = false;
        }

        bus.tick_peripherals(1);
    }

    tracer.dump("initial_waveform.vcd");

    if (pass) {
        int measured_dead = dead_b_start - dead_a_end;
        CHECK_MESSAGE(dead_a_end == OUTA_END,
            "OutA falling at counter " << dead_a_end << " (expected " << OUTA_END << ")");
        CHECK_MESSAGE(dead_b_start == OUTB_START,
            "OutB rising at counter " << dead_b_start << " (expected " << OUTB_START << ")");
        CHECK_MESSAGE(measured_dead == 2 * DEAD_TIME,
            "Dead time: " << measured_dead << " cycles (expected " << (2 * DEAD_TIME) << ")");
    }
}

struct CycleShape {
    int start_a, end_a, start_b, end_b;
};

// Walk one full PSC period, record edge positions, and return the shape.
// If tracer is provided, records OutA/OutB at each cycle.
// check_fn(c, a_state, b_state) is called for every counter value.
static CycleShape walk_cycle(auto& bus,
                              vcd::SignalTracer* tracer = nullptr,
                              std::optional<std::function<bool(int,int,int)>> check_fn = {},
                              uint64_t* out_time = nullptr)
{
    // Initialise with state at counter=0 before any ticks
    int last_a = bus.pin_mux()->get_state_by_address(PORTD_PIN, 0).drive_level ? 1 : 0;
    int last_b = bus.pin_mux()->get_state_by_address(PORTD_PIN, 1).drive_level ? 1 : 0;
    CycleShape s{-1, -1, -1, -1};

    for (int c = 0; c < TOP_VAL; ++c) {
        int a = bus.pin_mux()->get_state_by_address(PORTD_PIN, 0).drive_level ? 1 : 0;
        int b = bus.pin_mux()->get_state_by_address(PORTD_PIN, 1).drive_level ? 1 : 0;

        if (last_a == 0 && a == 1) s.start_a = c;
        if (last_a == 1 && a == 0) s.end_a   = c;
        if (last_b == 0 && b == 1) s.start_b = c;
        if (last_b == 1 && b == 0) s.end_b   = c;

        if (tracer) {
            tracer->record("OutA", *out_time, (bool)a);
            tracer->record("OutB", *out_time, (bool)b);
            (*out_time)++;
        }

        if (check_fn) {
            bool ok = (*check_fn)(c, a, b);
            if (!ok) FAIL("Counter=" << c << " pin state check failed");
        }

        last_a = a; last_b = b;
        bus.tick_peripherals(1);
    }

    // Output still HIGH at wrap → it falls at counter = TOP_VAL
    if (s.end_b < 0 && last_b == 1) s.end_b = TOP_VAL;
    if (s.end_a < 0 && last_a == 1) s.end_a = TOP_VAL;
    return s;
}

TEST_CASE("Inverter PSC - Duty cycle modulation")
{
    auto machine = Machine::create_for_device("AT90PWM316");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();

    bus.write_data(PSOC0, (1 << 0) | (1 << 2));
    bus.write_data(PFRC0A, (1 << 2));

    bus.write_data(OCR0RB + 1, TOP_VAL >> 8);
    bus.write_data(OCR0RB,     TOP_VAL & 0xFF);

    bus.write_data(OCR0SA + 1, 0);
    bus.write_data(OCR0SA,     0);
    bus.write_data(OCR0RA + 1, OUTA_END >> 8);
    bus.write_data(OCR0RA,     OUTA_END & 0xFF);
    bus.write_data(OCR0SB + 1, OUTB_START >> 8);
    bus.write_data(OCR0SB,     OUTB_START & 0xFF);
    bus.write_data(OCR0RB + 1, TOP_VAL >> 8);
    bus.write_data(OCR0RB,     TOP_VAL & 0xFF);
    bus.write_data(PCTL0, 1);

    vcd::SignalTracer tracer;
    tracer.add_signal("OutA");
    tracer.add_signal("OutB");
    uint64_t t = 0;

    // Apply duty matching the firmware's set_duty().
    // OCR0RA, OCR0SB, OCR0RB stay fixed — only OCR0SA varies.
    auto set_duty = [&](uint16_t percent) -> uint16_t {
        uint16_t pulse_width = (uint32_t)percent * (HALF_TOP - 2 * DEAD_TIME) / 100;
        uint16_t start_a = (pulse_width < (uint16_t)(HALF_TOP - DEAD_TIME))
                           ? (HALF_TOP - DEAD_TIME - pulse_width) : 0;

        bus.write_data(OCR0SA + 1, start_a >> 8);
        bus.write_data(OCR0SA,     start_a & 0xFF);
        return pulse_width;
    };

    struct DutyCase { const char* label; uint16_t pct; int pw; int exp_start_a; };
    DutyCase cases[] = {
        {"50%",  50, 192, 200},
        {"25%",  25,  96, 296},
        {"100%", 100, 384,   8},
    };

    for (auto& dc : cases) {
        uint16_t pw = set_duty(dc.pct);
        CHECK_MESSAGE(pw == dc.pw, dc.label << " pulse width = " << pw);

        auto check = [&](int c, int a, int b) -> bool {
            if (c == 0) return true;
            bool exp_a = (c >= dc.exp_start_a && c < OUTA_END);
            bool exp_b = (c >= OUTB_START  && c < OUTB_END);
            bool ok_a = (a == (exp_a ? 1 : 0));
            bool ok_b = (b == (exp_b ? 1 : 0));
            if (!ok_a || !ok_b) {
                FAIL(dc.label << " c=" << c
                     << " OutA got=" << a << " exp=" << (exp_a?1:0)
                     << " OutB got=" << b << " exp=" << (exp_b?1:0));
                return false;
            }
            return true;
        };

        // Drain one period (VCD included), then verify next period
        walk_cycle(bus, &tracer, {}, &t);
        CycleShape s = walk_cycle(bus, &tracer, check, &t);

        CHECK_MESSAGE(s.start_a == dc.exp_start_a,
                      dc.label << " OutA rise at " << s.start_a
                      << " (expected " << dc.exp_start_a << ")");
        CHECK_MESSAGE(s.end_a == OUTA_END,
                      dc.label << " OutA fall at " << s.end_a
                      << " (expected " << OUTA_END << ")");
        CHECK_MESSAGE(s.start_b == OUTB_START,
                      dc.label << " OutB rise at " << s.start_b
                      << " (expected " << OUTB_START << ")");

        int on_a = s.end_a - s.start_a;
        int on_b = s.end_b - s.start_b;
        CHECK_MESSAGE(on_a == dc.pw, dc.label << " OutA width " << on_a << " (expected " << dc.pw << ")");
        CHECK_MESSAGE(on_b == OUTB_END - OUTB_START,
                      dc.label << " OutB width " << on_b << " (expected " << (OUTB_END - OUTB_START) << " fixed)");
    }

    tracer.dump("duty_modulation.vcd");
}

TEST_CASE("PSC PFRC0B register does not produce dead-time")
{
    // PFRC0B on AT90PWM316 controls fault channel B (PRFM0B, PFLTE0B, PELEV0B,
    // PISEL0B, PCAE0B).  It does NOT contain DT0L/DT0H dead-time duration nibbles.
    // This test verifies that writing PFRC0B does NOT affect output timing.
    auto machine = Machine::create_for_device("AT90PWM316");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();

    // Non-overlapping pulses: OutA=[0, 392), OutB=[400, 800)
    bus.write_data(OCR0RB + 1, 800 >> 8); bus.write_data(OCR0RB, 800 & 0xFF);
    bus.write_data(OCR0SA + 1, 0); bus.write_data(OCR0SA, 0);
    bus.write_data(OCR0RA + 1, 392 >> 8); bus.write_data(OCR0RA, 392 & 0xFF);
    bus.write_data(OCR0SB + 1, 400 >> 8); bus.write_data(OCR0SB, 400 & 0xFF);
    bus.write_data(PCNF0, 0);

    bus.write_data(PSOC0, (1 << 0) | (1 << 2));

    // Write PFRC0B with non-zero values that would have been misinterpreted
    // as dead-time.  Bits 3:0 = PRFM0B=0x5, bits 7:4 = PCAE0B|PISEL0B|PELEV0B|PFLTE0B.
    bus.write_data(PFRC0A, 0);
    bus.write_data(0xDD, 0xF5);

    bus.write_data(PCTL0, 1);

    int fall_a = -1, rise_b = -1;
    int prev_a = 0, prev_b = 0;

    for (int c = 0; c < 800; ++c) {
        int a = bus.pin_mux()->get_state_by_address(PORTD_PIN, 0).drive_level ? 1 : 0;
        int b = bus.pin_mux()->get_state_by_address(PORTD_PIN, 1).drive_level ? 1 : 0;

        if (fall_a < 0 && prev_a == 1 && a == 0) fall_a = c;
        if (rise_b < 0 && prev_b == 0 && b == 1) rise_b = c;

        prev_a = a; prev_b = b;
        bus.tick_peripherals(1);
    }

    // Verify natural gap = 400 - 392 = 8 cycles (no dead-time added)
    CHECK_MESSAGE(fall_a == 392, "OutA fall at " << fall_a << " (expected 392)");
    CHECK_MESSAGE(rise_b == 400, "OutB rise at " << rise_b << " (expected 400, no dead-time)");
    CHECK_MESSAGE(rise_b - fall_a == 8,
                  "Gap = " << (rise_b - fall_a) << " (expected 8, no dead-time added by PFRC0B)");
}

