#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/rtc.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;

static const auto& D = devices::atmega4809.timers_rtc[0];

struct RtcFixture {
    MemoryBus bus{devices::atmega4809};
    Rtc rtc{D};

    RtcFixture() {
        rtc.set_memory_bus(&bus);
        rtc.reset();
    }
};

TEST_CASE("Rtc — reset defaults") {
    RtcFixture f;

    CHECK(f.rtc.read(D.ctrla_address)        == 0x00U);
    CHECK(f.rtc.read(D.status_address)       == 0x00U);
    CHECK(f.rtc.read(D.intctrl_address)      == 0x00U);
    CHECK(f.rtc.read(D.intflags_address)     == 0x00U);
    CHECK(f.rtc.read(D.temp_address)         == 0x00U);
    CHECK(f.rtc.read(D.clksel_address)       == 0x00U);
    CHECK(f.rtc.read(D.calib_address)        == 0x00U);
    CHECK(f.rtc.read(D.dbgctrl_address)      == 0x00U);
    CHECK(f.rtc.read(D.cnt_address)          == 0x00U);
    CHECK(f.rtc.read(D.cnt_address + 1)       == 0x00U);
    CHECK(f.rtc.read(D.per_address)          == 0xFFU); // PER default 0xFFFF
    CHECK(f.rtc.read(D.per_address + 1)       == 0xFFU);
    CHECK(f.rtc.read(D.cmp_address)          == 0x00U);
    CHECK(f.rtc.read(D.cmp_address + 1)       == 0x00U);
    CHECK(f.rtc.read(D.pitctrla_address)     == 0x00U);
    CHECK(f.rtc.read(D.pitstatus_address)    == 0x00U);
    CHECK(f.rtc.read(D.pitintctrl_address)   == 0x00U);
    CHECK(f.rtc.read(D.pitintflags_address)  == 0x00U);
}

TEST_CASE("Rtc — CMP compare match sets CMP flag") {
    RtcFixture f;

    // RTC enabled, DIV1, PER=10, CMP=7
    f.rtc.write(D.per_address + 1, 0x00U); f.rtc.write(D.per_address, 10);
    f.rtc.write(D.cmp_address + 1, 0x00U); f.rtc.write(D.cmp_address, 7);
    f.rtc.write(D.ctrla_address, 0x01U); // ENABLE=1, PRESCALER=DIV1

    // Run 8 ticks: CMP match at old_cnt=7==cmp_=7
    f.rtc.tick(8);
    u16 cnt = f.rtc.read(D.cnt_address) | (f.rtc.read(D.cnt_address + 1) << 8);
    CHECK(cnt == 8);

    // CMP flag should be set (match at cmp_=7)
    CHECK((f.rtc.read(D.intflags_address) & 0x02) != 0U);
    // OVF flag NOT set yet
    CHECK((f.rtc.read(D.intflags_address) & 0x01) == 0U);
}

TEST_CASE("Rtc — CMP == PER edge case (CNT resets but CMP also matches)") {
    RtcFixture f;

    // RTC enabled, DIV1, PER=5, CMP=5
    f.rtc.write(D.per_address + 1, 0x00U); f.rtc.write(D.per_address, 5);
    f.rtc.write(D.cmp_address + 1, 0x00U); f.rtc.write(D.cmp_address, 5);
    f.rtc.write(D.ctrla_address, 0x01U);

    // Run 5 ticks: cnt goes 0→1→2→3→4→5
    f.rtc.tick(5);
    // On tick 5: cnt was 4, tick_core checks cnt==cmp(5?)→no, cnt==per(5?)→no, cnt=5
    // On tick 6: cnt was 5, tick_core checks cnt==cmp(5)→yes, set CMP, cnt==per(5)→yes, reset cnt, set OVF
    // Wait, tick_core is called each time internal_ticks_ reaches div=1
    // So after 5 ticks: cnt went 0→1→2→3→4→5 (5 transitions)
    // check_cmp: cnt_++ first? No, tick_core checks BEFORE incrementing old_cnt
    // Actually: old_cnt = cnt_ = 0 at first call
    // old_cnt (0) == cmp_ (5)? No. old_cnt (0) == per_ (5)? No. cnt_ = 1
    // After 5 calls: cnt_ = 5
    // After 6 calls: old_cnt = 5 == cmp_ (5) → CMP set. old_cnt == per_ (5) → cnt_ = 0, OVF set

    // Wait, but I ran 5 ticks which produces 5 calls to tick_core (since there are 5 internal ticks reaching div threshold)
    // Actually no: each tick(1) increments internal_ticks_ by 1, checks >=1, resets, calls handle_rtc_tick()→tick_core()
    // So tick(5) → 5 calls to tick_core
    // After 5 calls: cnt_ = 5 (0→1→2→3→4→5)
    // But wait, cnt_ starts at 0. After tick_core call 1: cnt_=1. After call 5: cnt_=5.
    // So after tick(5), cnt_=5. The CMP check in call 5: old_cnt=4 != cmp_=5. OVF check: old_cnt=4 != per_=5.
    // Hmm, so CMP wasn't triggered yet. Need tick(6) to trigger both.

    f.rtc.tick(1);
    u16 cnt = f.rtc.read(D.cnt_address) | (f.rtc.read(D.cnt_address + 1) << 8);
    CHECK(cnt == 0); // Wrapped from 5 to 0

    // Both OVF and CMP should be set
    CHECK((f.rtc.read(D.intflags_address) & 0x01) != 0U); // OVF
    CHECK((f.rtc.read(D.intflags_address) & 0x02) != 0U); // CMP
}

TEST_CASE("Rtc — INTCTRL OVFIE enables OVF interrupt") {
    RtcFixture f;

    f.rtc.write(D.per_address + 1, 0x00U); f.rtc.write(D.per_address, 3);
    f.rtc.write(D.intctrl_address, 0x01U); // OVFIE=1
    f.rtc.write(D.ctrla_address, 0x01U);

    InterruptRequest req{};
    CHECK_FALSE(f.rtc.pending_interrupt_request(req));

    f.rtc.tick(4); // 0→1→2→3→0
    CHECK((f.rtc.read(D.intflags_address) & 0x01) != 0U);

    CHECK(f.rtc.pending_interrupt_request(req));
    CHECK(req.vector_index == D.ovf_vector_index);
    CHECK(f.rtc.consume_interrupt_request(req));
    CHECK_FALSE(f.rtc.pending_interrupt_request(req));
}

TEST_CASE("Rtc — INTCTRL CMPIE enables CMP interrupt") {
    RtcFixture f;

    f.rtc.write(D.per_address + 1, 0x00U); f.rtc.write(D.per_address, 10);
    f.rtc.write(D.cmp_address + 1, 0x00U); f.rtc.write(D.cmp_address, 3);
    f.rtc.write(D.intctrl_address, 0x02U); // CMPIE=1
    f.rtc.write(D.ctrla_address, 0x01U);

    InterruptRequest req{};
    f.rtc.tick(4); // 0→1→2→3→4 triggers CMP at old_cnt=3==cmp_=3
    CHECK((f.rtc.read(D.intflags_address) & 0x02) != 0U);
    CHECK(f.rtc.pending_interrupt_request(req));
    CHECK(req.vector_index == D.ovf_vector_index);
}

TEST_CASE("Rtc — INTFLAGS clear-by-writing-1") {
    RtcFixture f;

    f.rtc.write(D.per_address + 1, 0x00U); f.rtc.write(D.per_address, 3);
    f.rtc.write(D.ctrla_address, 0x01U);
    f.rtc.tick(4); // Overflow → OVF set

    CHECK((f.rtc.read(D.intflags_address) & 0x01) != 0U);

    f.rtc.write(D.intflags_address, 0x01U); // Write 1 to OVF bit
    CHECK((f.rtc.read(D.intflags_address) & 0x01) == 0U);

    // Write 1 to non-set bits does nothing
    f.rtc.write(D.intflags_address, 0xFEU); // Only clears bits already set
    CHECK(f.rtc.read(D.intflags_address) == 0x00U);
}

TEST_CASE("Rtc — PITINTCTRL enables PIT interrupt") {
    RtcFixture f;

    f.rtc.write(D.pitctrla_address, 0x09U); // PERIOD=1 (4 cycles), ENABLE=1
    f.rtc.write(D.pitintctrl_address, 0x01U); // PITIE=1

    InterruptRequest req{};
    CHECK_FALSE(f.rtc.pending_interrupt_request(req));

    f.rtc.tick(4); // 4 cycles → PIT fires
    CHECK((f.rtc.read(D.pitintflags_address) & 0x01) != 0U);

    CHECK(f.rtc.pending_interrupt_request(req));
    CHECK(req.vector_index == D.pit_vector_index);
    CHECK(f.rtc.consume_interrupt_request(req));
    CHECK_FALSE(f.rtc.pending_interrupt_request(req));
}

TEST_CASE("Rtc — PITINTFLAGS clear-by-writing-1") {
    RtcFixture f;

    f.rtc.write(D.pitctrla_address, 0x09U); // PERIOD=1, ENABLE=1
    f.rtc.tick(4);
    CHECK((f.rtc.read(D.pitintflags_address) & 0x01) != 0U);

    f.rtc.write(D.pitintflags_address, 0x01U);
    CHECK((f.rtc.read(D.pitintflags_address) & 0x01) == 0U);
}

TEST_CASE("Rtc — PITSTATUS clear after sync") {
    RtcFixture f;

    f.rtc.write(D.pitctrla_address, 0x01U); // ENABLE=1, PERIOD=0
    CHECK((f.rtc.read(D.pitstatus_address) & 0x01) != 0U); // CTRLBUSY

    f.rtc.tick(64); // Sync finishes
    CHECK((f.rtc.read(D.pitstatus_address) & 0x01) == 0U);
}

TEST_CASE("Rtc — PIT multiple periods (PERIOD=2 → 8 cycles)") {
    RtcFixture f;

    f.rtc.write(D.pitctrla_address, 0x11U); // PERIOD=2, ENABLE=1
    f.rtc.tick(7);
    CHECK((f.rtc.read(D.pitintflags_address) & 0x01) == 0U); // Not yet

    f.rtc.tick(1);
    CHECK((f.rtc.read(D.pitintflags_address) & 0x01) != 0U); // At 8
}

TEST_CASE("Rtc — PIT PERIOD=7 → 256 cycles") {
    RtcFixture f;

    f.rtc.write(D.pitctrla_address, ((0x07U << 3) | 0x01)); // PERIOD=7, ENABLE=1
    f.rtc.tick(255);
    CHECK((f.rtc.read(D.pitintflags_address) & 0x01) == 0U); // Not yet

    f.rtc.tick(1);
    CHECK((f.rtc.read(D.pitintflags_address) & 0x01) != 0U); // At 256
}

TEST_CASE("Rtc — prescaler values: DIV2, DIV4, DIV8") {
    RtcFixture f;

    auto check_prescaler = [&](u8 prescaler_bits, u16 div, int cycles_needed) {
        f.rtc.reset();
        f.rtc.write(D.per_address + 1, 0x00U); f.rtc.write(D.per_address, 10);
        f.rtc.write(D.ctrla_address, 0x01U | (prescaler_bits << 3));
        f.rtc.tick(cycles_needed);
        u16 cnt = f.rtc.read(D.cnt_address) | (f.rtc.read(D.cnt_address + 1) << 8);
        // After cycles_needed cycles, cnt should be cycles_needed / div
        // But the internal prescaler resets at cycle 0, so first count at cycle div-1
        // After cycles_needed cycles, cnt = cycles_needed / div
        CHECK(cnt == static_cast<u16>(cycles_needed / div));
    };

    check_prescaler(0, 1, 5);      // DIV1: 5 ticks → cnt=5
    check_prescaler(1, 2, 6);      // DIV2: 6 ticks → cnt=3
    check_prescaler(2, 4, 12);     // DIV4: 12 ticks → cnt=3
    check_prescaler(3, 8, 16);     // DIV8: 16 ticks → cnt=2
}

TEST_CASE("Rtc — CLKSEL register round-trip") {
    RtcFixture f;

    CHECK(f.rtc.read(D.clksel_address) == 0x00U);
    f.rtc.write(D.clksel_address, 0x03U); // XOSC32K
    CHECK(f.rtc.read(D.clksel_address) == 0x03U);
}

TEST_CASE("Rtc — CALIB register round-trip") {
    RtcFixture f;

    CHECK(f.rtc.read(D.calib_address) == 0x00U);
    f.rtc.write(D.calib_address, 0x85U); // SIGN=1 (negative), ERROR=5
    CHECK(f.rtc.read(D.calib_address) == 0x85U);
}

TEST_CASE("Rtc — DBGCTRL register round-trip") {
    RtcFixture f;

    CHECK(f.rtc.read(D.dbgctrl_address) == 0x00U);
    f.rtc.write(D.dbgctrl_address, 0x01U); // DBGRUN=1
    CHECK(f.rtc.read(D.dbgctrl_address) == 0x01U);
}

TEST_CASE("Rtc — disabled RTC does not count") {
    RtcFixture f;

    f.rtc.write(D.per_address + 1, 0x00U); f.rtc.write(D.per_address, 10);
    // ctrla_ = 0x00 (RTC disabled)
    f.rtc.tick(100);
    u16 cnt = f.rtc.read(D.cnt_address) | (f.rtc.read(D.cnt_address + 1) << 8);
    CHECK(cnt == 0);
    CHECK((f.rtc.read(D.intflags_address) & 0x01) == 0U); // No OVF
}

TEST_CASE("Rtc — CMP register 16-bit atomic access") {
    RtcFixture f;

    // Write CMPH then CMPL
    f.rtc.write(D.cmp_address + 1, 0xABU);
    f.rtc.write(D.cmp_address, 0xCDU);
    u16 cmp = f.rtc.read(D.cmp_address) | (f.rtc.read(D.cmp_address + 1) << 8);
    CHECK(cmp == 0xABCDU);
}

TEST_CASE("Rtc — unmapped addresses return 0") {
    RtcFixture f;

    CHECK(f.rtc.read(0x0000) == 0x00U);
    CHECK(f.rtc.read(D.ctrla_address - 1) == 0x00U);
    CHECK(f.rtc.read(D.pitintflags_address + 1) == 0x00U);
    CHECK(f.rtc.read(0xFFFFU) == 0x00U);
}
