#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/clkctrl.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;

static const auto& D = devices::atmega4809.clkctrl;

static ClkCtrl make_clkctrl() {
    return ClkCtrl{D, 20'000'000U};
}

TEST_CASE("ClkCtrl — OSC32KCTRLA register round-trip") {
    auto clk = make_clkctrl();
    clk.reset();

    CHECK(clk.read(D.osc32kctrla_address) == 0x00U);

    clk.write(D.osc32kctrla_address, 0x01U); // RUNSTDBY
    CHECK(clk.read(D.osc32kctrla_address) == 0x01U);

    clk.write(D.osc32kctrla_address, 0x00U);
    CHECK(clk.read(D.osc32kctrla_address) == 0x00U);
}

TEST_CASE("ClkCtrl — XOSC32KCTRLA register round-trip") {
    auto clk = make_clkctrl();
    clk.reset();

    CHECK(clk.read(D.xosc32kctrla_address) == 0x00U);

    // XOSC32KCTRLA bits: SEL(0), CSUT[2:1], RUNSTDBY(7)
    clk.write(D.xosc32kctrla_address, 0x82U); // RUNSTDBY=1, CSUT=01
    CHECK(clk.read(D.xosc32kctrla_address) == 0x82U);

    clk.write(D.xosc32kctrla_address, 0x00U);
    CHECK(clk.read(D.xosc32kctrla_address) == 0x00U);
}

TEST_CASE("ClkCtrl — MCLKCTRLA CLKOUT bit") {
    auto clk = make_clkctrl();
    clk.reset();

    // CLKOUT is bit 7 of MCLKCTRLA
    CHECK((clk.read(D.ctrla_address) & 0x80U) == 0U); // Default CLKOUT=0

    clk.write(D.ctrla_address, 0x80U); // Set CLKOUT, keep CLKSEL=0
    CHECK((clk.read(D.ctrla_address) & 0x80U) != 0U);
    CHECK((clk.read(D.ctrla_address) & 0x03U) == 0x00U); // CLKSEL unaffected

    // Write CLKSEL=1 with CLKOUT=1
    clk.write(D.ctrla_address, 0x81U);
    CHECK((clk.read(D.ctrla_address) & 0x80U) != 0U);
    CHECK((clk.read(D.ctrla_address) & 0x03U) == 0x01U);
}

TEST_CASE("ClkCtrl — PDIV /10, /12, /24, /48") {
    auto clk = make_clkctrl();
    clk.reset();

    // Default PEN=1, disable prescaler first, then set specific PDIV
    // PDIV encoding: idx stored in bits[4:1], idx 9 = 0b10010 → PDIV=0x12
    // bits[4:1]: (idx << 1) & 0x1E

    SUBCASE("/10 (idx 9)") {
        clk.write(D.ctrlb_address, 0x13U); // PEN=1 + PDIV=0x10010>>1 = 0x12
        CHECK(clk.cpu_frequency_hz() == 2'000'000U); // 20 MHz / 10
    }

    SUBCASE("/12 (idx 10)") {
        clk.write(D.ctrlb_address, 0x15U); // PEN=1 + PDIV=0x10100>>1 = 0x14
        CHECK(clk.cpu_frequency_hz() == 1'666'666U); // 20 MHz / 12
    }

    SUBCASE("/24 (idx 11)") {
        clk.write(D.ctrlb_address, 0x17U); // PEN=1 + PDIV=0x10110>>1 = 0x16
        CHECK(clk.cpu_frequency_hz() == 833'333U); // 20 MHz / 24
    }

    SUBCASE("/48 (idx 12)") {
        clk.write(D.ctrlb_address, 0x19U); // PEN=1 + PDIV=0x11000>>1 = 0x18
        CHECK(clk.cpu_frequency_hz() == 416'666U); // 20 MHz / 48
    }
}

TEST_CASE("ClkCtrl — MCLKSTATUS SOSC bit for XOSC32K source") {
    auto clk = make_clkctrl();
    clk.reset();

    // SOSC (bit 0) — external crystal oscillator status
    clk.write(D.ctrla_address, 0x02U); // CLKSEL = XOSC32K
    clk.write(D.ctrlb_address, 0x00U); // No prescaler

    u8 status = clk.read(D.mclkstatus_address);
    CHECK((status & 0x40U) != 0U); // EXTS (bit 6) set for XOSC32K
    CHECK((status & 0x10U) == 0U); // OSC20MS cleared
    CHECK((status & 0x20U) == 0U); // OSC32KS not set

    // Switch to EXTCLK
    clk.set_ext_clock_hz(8'000'000U);
    clk.write(D.ctrla_address, 0x03U);
    status = clk.read(D.mclkstatus_address);
    CHECK((status & 0x80U) != 0U); // EXTS (bit 7) set for EXTCLK
}

TEST_CASE("ClkCtrl — source_frequency_hz for all clock sources") {
    auto clk = make_clkctrl();
    clk.reset();

    clk.write(D.ctrlb_address, 0x00U); // No prescaler → cpu == source

    // OSC20M (default)
    CHECK(clk.source_frequency_hz() == 20'000'000U);
    CHECK(clk.cpu_frequency_hz() == 20'000'000U);

    // OSCULP32K
    clk.write(D.ctrla_address, 0x01U);
    CHECK(clk.source_frequency_hz() == 32'768U);

    // XOSC32K
    clk.write(D.ctrla_address, 0x02U);
    CHECK(clk.source_frequency_hz() == 32'768U);

    // EXTCLK — default 0 Hz (no ext clock set)
    clk.write(D.ctrla_address, 0x03U);
    CHECK(clk.source_frequency_hz() == 0U);

    // Set ext clock
    clk.set_ext_clock_hz(4'000'000U);
    CHECK(clk.source_frequency_hz() == 4'000'000U);
}

TEST_CASE("ClkCtrl — set_base_frequency changes computed freq") {
    auto clk = make_clkctrl();
    clk.reset();

    // Default: 20 MHz / 6 = 3.333 MHz
    CHECK(clk.cpu_frequency_hz() == 3'333'333U);

    // Change base to 16 MHz → 16 MHz / 6 = 2.666 MHz
    clk.set_base_frequency(16'000'000U);
    CHECK(clk.cpu_frequency_hz() == 2'666'666U);

    // Back to 20 MHz, disable prescaler
    clk.set_base_frequency(20'000'000U);
    clk.write(D.ctrlb_address, 0x00U);
    CHECK(clk.cpu_frequency_hz() == 20'000'000U);
}

TEST_CASE("ClkCtrl — OSC20M CALIBB register (second cal byte)") {
    auto clk = make_clkctrl();
    clk.reset();

    u16 calibb_addr = static_cast<u16>(D.osc20mcalib_address + 1U);

    CHECK(clk.read(calibb_addr) == 0x00U);

    clk.write(calibb_addr, 0xFFU);
    CHECK(clk.read(calibb_addr) == 0xFFU);

    clk.write(calibb_addr, 0xAAU);
    CHECK(clk.read(calibb_addr) == 0xAAU);
}

TEST_CASE("ClkCtrl — MCLKSTATUS read-only") {
    auto clk = make_clkctrl();
    clk.reset();

    // Try to write to MCLKSTATUS — should be silently ignored (no write handler)
    u8 initial = clk.read(D.mclkstatus_address);
    CHECK((initial & 0x10U) != 0U); // OSC20MS set

    // Write via address — there is no explicit write handler for status
    // so it should remain unchanged
    clk.write(D.mclkstatus_address, 0x00U);
    CHECK(clk.read(D.mclkstatus_address) == initial);

    clk.write(D.mclkstatus_address, 0xFFU);
    CHECK(clk.read(D.mclkstatus_address) == initial);
}

TEST_CASE("ClkCtrl — Unmapped address returns 0") {
    auto clk = make_clkctrl();
    clk.reset();

    CHECK(clk.read(0x64U) == 0x00U); // Between mapped blocks
    CHECK(clk.read(0x6FU) == 0x00U);
    CHECK(clk.read(0x79U) == 0x00U); // Between OSC32K and XOSC32K
    CHECK(clk.read(0xFFFFU) == 0x00U); // Way out of range
}
