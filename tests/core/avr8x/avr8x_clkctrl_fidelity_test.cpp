#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/clkctrl.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;

// ─── Helpers ────────────────────────────────────────────────────────────────

static ClkCtrl make_clkctrl() {
    return ClkCtrl{devices::atmega4809.clkctrl, 20'000'000U};
}

// ─── Tests ──────────────────────────────────────────────────────────────────

TEST_CASE("ClkCtrl — Reset defaults")
{
    auto clk = make_clkctrl();
    clk.reset();

    // After reset: MCLKCTRLB = 0x11 → PEN=1, PDIV=0x08 (bits[4:1]=0b1000 → idx 8 → /6)
    // 20 MHz / 6 = 3,333,333 Hz  (matches ATmega4809 factory default)
    CHECK(clk.cpu_frequency_hz() == 3'333'333U);
    CHECK(clk.clock_source() == ClkCtrl::ClockSource::osc20m);
}

TEST_CASE("ClkCtrl — MCLKCTRLB prescaler combinations")
{
    auto clk = make_clkctrl();
    clk.reset();

    const auto& desc = devices::atmega4809.clkctrl;

    SUBCASE("Prescaler disabled → full 20 MHz") {
        // MCLKCTRLB = 0x00: PEN=0 → no prescaler
        clk.write(desc.ctrlb_address, 0x00U);
        CHECK(clk.cpu_frequency_hz() == 20'000'000U);
    }

    SUBCASE("PDIV /2 (idx 0, bits [4:1]=0b0000)") {
        // PEN=1, PDIV=0x00 → bits[4:1]=0000 → idx 0 → /2
        clk.write(desc.ctrlb_address, 0x01U); // PEN=1, PDIV bits=0
        CHECK(clk.cpu_frequency_hz() == 10'000'000U);
    }

    SUBCASE("PDIV /4 (idx 1, bits [4:1]=0b0010)") {
        clk.write(desc.ctrlb_address, 0x03U); // PEN=1 + PDIV=0x01<<1
        CHECK(clk.cpu_frequency_hz() == 5'000'000U);
    }

    SUBCASE("PDIV /8 (idx 2)") {
        clk.write(desc.ctrlb_address, 0x05U);
        CHECK(clk.cpu_frequency_hz() == 2'500'000U);
    }

    SUBCASE("PDIV /64 (idx 5, bits[4:1]=0b1010)") {
        clk.write(desc.ctrlb_address, 0x0BU);
        CHECK(clk.cpu_frequency_hz() == 312'500U);
    }

    SUBCASE("PDIV /6 (idx 8, bits[4:1]=0b10000)") {
        // This is the reset default (MCLKCTRLB=0x11 = PEN|PDIV=0x08)
        clk.write(desc.ctrlb_address, 0x11U);
        CHECK(clk.cpu_frequency_hz() == 3'333'333U);
    }
}

TEST_CASE("ClkCtrl — MCLKCTRLA clock source switch")
{
    auto clk = make_clkctrl();
    clk.reset();

    const auto& desc = devices::atmega4809.clkctrl;

    SUBCASE("Switch to OSCULP32K — no prescaler") {
        clk.write(desc.ctrlb_address, 0x00U);  // Disable prescaler first
        clk.write(desc.ctrla_address, 0x01U);  // CLKSEL = OSCULP32K
        CHECK(clk.clock_source() == ClkCtrl::ClockSource::osculp32k);
        CHECK(clk.cpu_frequency_hz() == 32'768U);
    }

    SUBCASE("Switch to XOSC32K — no prescaler") {
        clk.write(desc.ctrlb_address, 0x00U);
        clk.write(desc.ctrla_address, 0x02U);
        CHECK(clk.clock_source() == ClkCtrl::ClockSource::xosc32k);
        CHECK(clk.cpu_frequency_hz() == 32'768U);
    }

    SUBCASE("External clock at 8 MHz — no prescaler") {
        clk.set_ext_clock_hz(8'000'000U);
        clk.write(desc.ctrlb_address, 0x00U);
        clk.write(desc.ctrla_address, 0x03U);
        CHECK(clk.clock_source() == ClkCtrl::ClockSource::extclk);
        CHECK(clk.cpu_frequency_hz() == 8'000'000U);
    }
}

TEST_CASE("ClkCtrl — MCLKLOCK prevents writes to MCLKCTRLA/B")
{
    auto clk = make_clkctrl();
    clk.reset();

    const auto& desc = devices::atmega4809.clkctrl;

    // Confirm prescaler currently enabled (default /6 = 3.333 MHz)
    CHECK(clk.cpu_frequency_hz() == 3'333'333U);

    // Lock the clock
    clk.write(desc.mclklock_address, 0x01U);

    // Try to switch to no-prescaler — should be silently ignored
    clk.write(desc.ctrlb_address, 0x00U);
    CHECK(clk.cpu_frequency_hz() == 3'333'333U);

    // Lock register read-back
    CHECK(clk.read(desc.mclklock_address) == 0x01U);
}

TEST_CASE("ClkCtrl — MCLKSTATUS reflects selected source")
{
    auto clk = make_clkctrl();
    clk.reset();

    const auto& desc = devices::atmega4809.clkctrl;

    // After reset: OSC20M selected → OSC20MS bit should be set
    u8 status = clk.read(desc.mclkstatus_address);
    CHECK((status & 0x10U) != 0U); // OSC20MS

    // Switch to ULP32K
    clk.write(desc.ctrla_address, 0x01U);
    status = clk.read(desc.mclkstatus_address);
    CHECK((status & 0x20U) != 0U); // OSC32KS
    CHECK((status & 0x10U) == 0U); // OSC20MS cleared
}

TEST_CASE("ClkCtrl — Frequency changed callback")
{
    auto clk = make_clkctrl();
    clk.reset();

    const auto& desc = devices::atmega4809.clkctrl;

    u32 reported_hz = 0;
    clk.set_frequency_changed_callback([&](u32 hz) { reported_hz = hz; });

    // Disable prescaler → callback should fire with 20 MHz
    clk.write(desc.ctrlb_address, 0x00U);
    CHECK(reported_hz == 20'000'000U);

    // Switch source to ULP while prescaler still off
    clk.write(desc.ctrla_address, 0x01U);
    CHECK(reported_hz == 32'768U);
}

TEST_CASE("ClkCtrl — OSC20M calibration registers round-trip")
{
    auto clk = make_clkctrl();
    clk.reset();

    const auto& desc = devices::atmega4809.clkctrl;

    clk.write(desc.osc20mctrla_address,  0x02U); // RUNSTDBY
    clk.write(desc.osc20mcalib_address,  0x3FU); // CAL20M[6:0]

    CHECK(clk.read(desc.osc20mctrla_address) == 0x02U);
    CHECK(clk.read(desc.osc20mcalib_address) == 0x3FU);
}
