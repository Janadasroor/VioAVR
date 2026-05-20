#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/rstctrl.hpp"
#include "vioavr/core/slpctrl.hpp"
#include "vioavr/core/vref.hpp"
#include "vioavr/core/bodctrl.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;

// ═══════════════════════════════════════════════════════════════════════════
// RstCtrl
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("RstCtrl — reset defaults set PORF")
{
    RstCtrl rst{devices::atmega4809.rstctrl};
    rst.reset();

    const u16 addr = devices::atmega4809.rstctrl.rstfr_address;
    u8 flags = rst.read(addr);
    CHECK(flags == static_cast<u8>(RstCtrl::ResetCause::power_on));
}

TEST_CASE("RstCtrl — clear-by-writing-1 semantics")
{
    RstCtrl rst{devices::atmega4809.rstctrl};
    rst.reset();

    const u16 addr = devices::atmega4809.rstctrl.rstfr_address;

    // Inject watchdog + external reset
    rst.set_reset_cause(static_cast<u8>(RstCtrl::ResetCause::watchdog)
                      | static_cast<u8>(RstCtrl::ResetCause::external));

    CHECK(rst.read(addr) == (static_cast<u8>(RstCtrl::ResetCause::watchdog)
                           | static_cast<u8>(RstCtrl::ResetCause::external)));

    // Clear only the watchdog flag
    rst.write(addr, static_cast<u8>(RstCtrl::ResetCause::watchdog));
    CHECK(rst.read(addr) == static_cast<u8>(RstCtrl::ResetCause::external));

    // Clear the remaining flag
    rst.write(addr, static_cast<u8>(RstCtrl::ResetCause::external));
    CHECK(rst.read(addr) == 0x00U);
}

TEST_CASE("RstCtrl — set_reset_cause / reset_flags round-trip")
{
    RstCtrl rst{devices::atmega4809.rstctrl};
    rst.reset();

    rst.set_reset_cause(static_cast<u8>(RstCtrl::ResetCause::software));
    CHECK(rst.reset_flags() == static_cast<u8>(RstCtrl::ResetCause::software));

    rst.set_reset_cause(static_cast<u8>(RstCtrl::ResetCause::updi));
    CHECK(rst.reset_flags() == static_cast<u8>(RstCtrl::ResetCause::updi));
}

TEST_CASE("RstCtrl — mapped_ranges covers RSTFR")
{
    RstCtrl rst{devices::atmega4809.rstctrl};
    auto ranges = rst.mapped_ranges();
    REQUIRE(ranges.size() == 1U);
    CHECK(ranges[0].begin == devices::atmega4809.rstctrl.rstfr_address);
    CHECK(ranges[0].end   == devices::atmega4809.rstctrl.rstfr_address);
}

// ═══════════════════════════════════════════════════════════════════════════
// SlpCtrl
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("SlpCtrl — reset defaults (sleep disabled, IDLE mode)")
{
    SlpCtrl slp{devices::atmega4809.slpctrl};
    slp.reset();

    CHECK_FALSE(slp.sleep_enabled());
    CHECK(slp.sleep_mode() == 0x00U);
}

TEST_CASE("SlpCtrl — CTRLA register write / read")
{
    SlpCtrl slp{devices::atmega4809.slpctrl};
    slp.reset();

    const u16 addr = devices::atmega4809.slpctrl.ctrla_address;

    // SEN=1, SMODE=STANDBY (0b010 << 1 = 0x04) → CTRLA = 0x05
    slp.write(addr, 0x05U);
    CHECK(slp.read(addr)     == 0x05U);
    CHECK(slp.sleep_enabled());
    CHECK(slp.sleep_mode()   == 0x02U); // STANDBY

    // SEN=0 → disable
    slp.write(addr, 0x04U);
    CHECK_FALSE(slp.sleep_enabled());
}

TEST_CASE("SlpCtrl — all documented sleep modes")
{
    SlpCtrl slp{devices::atmega4809.slpctrl};
    slp.reset();

    const u16 addr = devices::atmega4809.slpctrl.ctrla_address;

    // ATmega4809: SMODE: 0=IDLE, 1=STANDBY, 2=PDOWN, rest reserved
    for (u8 mode = 0; mode <= 3; ++mode) {
        u8 ctrla = static_cast<u8>(SlpCtrl::SEN_MASK | (mode << 1U));
        slp.write(addr, ctrla);
        CHECK(slp.sleep_mode() == mode);
        CHECK(slp.sleep_enabled());
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// VRef
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("VRef — reset defaults (both references = 0.55 V)")
{
    VRef vref{devices::atmega4809.vref};
    vref.reset();

    // CTRLA=0x00 → AC0REFSEL=0 (0.55V), ADC0REFSEL=0 (0.55V)
    CHECK(vref.adc0_reference_voltage() == doctest::Approx(0.55).epsilon(0.001));
    CHECK(vref.ac0_reference_voltage()  == doctest::Approx(0.55).epsilon(0.001));
}

TEST_CASE("VRef — ADC0 reference selection (CTRLA bits [6:4])")
{
    VRef vref{devices::atmega4809.vref};
    vref.reset();

    const u16 addr = devices::atmega4809.vref.ctrla_address;
    const double vdd = 5.0;

    struct Case { u8 sel; double expected_v; };
    const Case cases[] = {
        {0x00U, 0.55}, // 0.55V
        {0x01U, 1.1 }, // 1.1V   → bits[6:4]=001 → CTRLA = 0x10
        {0x02U, 1.5 }, // 1.5V
        {0x03U, 2.5 }, // 2.5V
        {0x04U, 4.3 }, // 4.3V
        {0x05U, vdd }, // VDD
    };

    for (const auto& c : cases) {
        vref.write(addr, static_cast<u8>(c.sel << 4U));
        CHECK(vref.adc0_reference_voltage(vdd) == doctest::Approx(c.expected_v).epsilon(0.01));
    }
}

TEST_CASE("VRef — AC0 reference selection (CTRLA bits [2:0])")
{
    VRef vref{devices::atmega4809.vref};
    vref.reset();

    const u16 addr = devices::atmega4809.vref.ctrla_address;
    const double vdd = 5.0;

    struct Case { u8 sel; double expected_v; };
    const Case cases[] = {
        {0x00U, 0.55},
        {0x01U, 1.1 },
        {0x02U, 1.5 },
        {0x03U, 2.5 },
        {0x04U, 4.3 },
        {0x05U, vdd },
    };

    for (const auto& c : cases) {
        vref.write(addr, c.sel); // AC0REFSEL in bits [2:0]
        CHECK(vref.ac0_reference_voltage(vdd) == doctest::Approx(c.expected_v).epsilon(0.01));
    }
}

TEST_CASE("VRef — CTRLB reference output enable flags")
{
    VRef vref{devices::atmega4809.vref};
    vref.reset();

    const u16 ctrlb = devices::atmega4809.vref.ctrlb_address;

    CHECK_FALSE(vref.adc0_ref_output_enabled());
    CHECK_FALSE(vref.ac0_ref_output_enabled());

    vref.write(ctrlb, 0x03U); // Both enable bits set
    CHECK(vref.adc0_ref_output_enabled());
    CHECK(vref.ac0_ref_output_enabled());

    vref.write(ctrlb, 0x01U); // Only AC0REFEN
    CHECK_FALSE(vref.adc0_ref_output_enabled());
    CHECK(vref.ac0_ref_output_enabled());
}

// ═══════════════════════════════════════════════════════════════════════════
// BodCtrl (VLM)
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("BodCtrl — reset defaults")
{
    BodCtrl bod{devices::atmega4809.bod};
    bod.reset();

    const auto& d = devices::atmega4809.bod;
    CHECK(bod.read(d.ctrla_address)    == 0x05U); // SLEEP=01,ACTIVE=01
    CHECK(bod.read(d.intctrl_address)  == 0x00U);
    CHECK(bod.read(d.intflags_address) == 0x00U);
    CHECK(bod.read(d.status_address)   == 0x00U);
}

TEST_CASE("BodCtrl — no interrupt when VLMIE disabled")
{
    BodCtrl bod{devices::atmega4809.bod};
    bod.reset();

    // VLMIE=0 (default), VDD drops below threshold
    bod.update_vdd(1.0, 5.0); // Way below VDD

    InterruptRequest req{};
    CHECK_FALSE(bod.pending_interrupt_request(req));
    // INTFLAGS.VLMIF should still be set (flag fires regardless of IE)
    CHECK((bod.read(devices::atmega4809.bod.intflags_address) & 0x01U) != 0U);
}

TEST_CASE("BodCtrl — VLM interrupt fires on falling edge (VLMCFG=0)")
{
    BodCtrl bod{devices::atmega4809.bod};
    bod.reset();

    const auto& d = devices::atmega4809.bod;

    // Set VLMLVL=0 (5%below VDD), enable interrupt, VLMCFG=falling (default 0)
    bod.write(d.vlmctrla_address, 0x00U); // VLMLVL=0 → threshold = VDD*(1-0.05) = 4.75V
    bod.write(d.intctrl_address,  0x01U); // VLMIE=1

    // VDD starts above threshold
    bod.update_vdd(5.0, 5.0); // above 4.75V

    InterruptRequest req{};
    CHECK_FALSE(bod.pending_interrupt_request(req));
    CHECK_FALSE(bod.consume_interrupt_request(req));

    // VDD drops below threshold → falling edge → interrupt
    bod.update_vdd(4.0, 5.0);
    CHECK(bod.pending_interrupt_request(req));
    CHECK(req.vector_index == devices::atmega4809.bod.vlm_vector_index);

    // Consume clears pending
    CHECK(bod.consume_interrupt_request(req));
    CHECK_FALSE(bod.pending_interrupt_request(req));
}

TEST_CASE("BodCtrl — INTFLAGS clear-by-writing-1")
{
    BodCtrl bod{devices::atmega4809.bod};
    bod.reset();

    const auto& d = devices::atmega4809.bod;
    bod.write(d.vlmctrla_address, 0x00U);
    bod.write(d.intctrl_address,  0x01U);

    // Trigger the flag (falling edge)
    bod.update_vdd(5.0, 5.0);
    bod.update_vdd(1.0, 5.0);

    CHECK((bod.read(d.intflags_address) & 0x01U) != 0U); // VLMIF set

    // Clear by writing 1 to bit 0
    bod.write(d.intflags_address, 0x01U);
    CHECK((bod.read(d.intflags_address) & 0x01U) == 0U); // VLMIF cleared
    InterruptRequest req{};
    CHECK_FALSE(bod.pending_interrupt_request(req)); // No longer pending
}

TEST_CASE("BodCtrl — VLM STATUS reflects voltage level")
{
    BodCtrl bod{devices::atmega4809.bod};
    bod.reset();

    const auto& d = devices::atmega4809.bod;
    bod.write(d.vlmctrla_address, 0x00U); // VLMLVL=0 → threshold 4.75V
    bod.write(d.intctrl_address,  0x01U);

    bod.update_vdd(5.0, 5.0); // Above threshold
    CHECK(bod.read(d.status_address) == 0x00U); // VLMS=0 (above)

    bod.update_vdd(4.0, 5.0); // Below threshold
    CHECK((bod.read(d.status_address) & 0x01U) != 0U); // VLMS=1 (below)
}

TEST_CASE("BodCtrl — VLM trigger on both edges (VLMCFG=2)")
{
    BodCtrl bod{devices::atmega4809.bod};
    bod.reset();

    const auto& d = devices::atmega4809.bod;
    bod.write(d.vlmctrla_address, 0x00U);
    bod.write(d.intctrl_address,  0x05U); // VLMIE=1, VLMCFG=0b10="both"

    bod.update_vdd(5.0, 5.0); // Start above

    InterruptRequest req{};
    CHECK_FALSE(bod.pending_interrupt_request(req));

    // Falling edge → trigger
    bod.update_vdd(4.0, 5.0);
    CHECK(bod.consume_interrupt_request(req));

    // Rising edge → trigger again
    bod.update_vdd(5.0, 5.0);
    CHECK(bod.consume_interrupt_request(req));
}
