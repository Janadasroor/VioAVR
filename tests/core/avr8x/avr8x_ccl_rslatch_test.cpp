#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

TEST_CASE("AVR8X CCL - RS Latch Sequential Mode") {
    Machine machine(atmega4809);
    auto& bus = machine.bus();
    machine.reset();

    const u16 CCL_BASE = 0x01C0;
    const u16 CCL_SEQCTRL0 = 0x01C0;
    const u16 CCL_LUT0_CTRLA = 0x01C8;
    const u16 CCL_LUT0_CTRLB = 0x01C9;
    const u16 CCL_LUT0_TRUTH = 0x01CB;

    u16 loop[] = { 0xCFFF };
    machine.bus().load_flash(loop);
    machine.reset();

    // Configure CCL LUT0 with RS Latch sequential mode
    // SEQCTRL0 bits 1:0 = SEQSEL: 0=DFF, 1=JK, 2=DLATCH, 3=RS
    bus.write_data(CCL_SEQCTRL0, 0x03); // RS-Latch mode

    // LUT0 CTRLA: ENABLE=1, OUTEN=1
    bus.write_data(CCL_LUT0_CTRLA, 0x41);

    // LUT0 CTRLB: INSEL0=FEEDBACK(1), INSEL1=IO pin(4)
    // For RS latch: IN0 = /S (Set, active low), IN1 = /R (Reset, active low)
    // We use: IN0 from LUT1 feedback (LINK), IN1 from EVENTA
    // Actually in the implementation, RS latch uses IN0 as /S and IN1 as /R
    // INSEL0=0 (MASK) -> use TRUTH table bit 0
    // INSEL1=0 (MASK) -> use TRUTH table bit 1
    bus.write_data(CCL_LUT0_CTRLB, 0x00);
    bus.write_data(CCL_LUT0_CTRLB + 1, 0x00);

    // For RS Latch truth table:
    // /S=0, /R=0 -> Q=1 (forbidden but stable)
    // /S=0, /R=1 -> Q=1 (SET)
    // /S=1, /R=0 -> Q=0 (RESET)
    // /S=1, /R=1 -> Q=Q (hold)
    // With FEEDBACK/LINK to IN0 and IO to IN1,
    // we set TRUTH for LUT0 output = f(IN0, IN1, IN2)
    // For RS latch, only IN0 and IN1 matter with the sequencer
    bus.write_data(CCL_LUT0_TRUTH, 0xCA);

    machine.step();

    // In RS latch mode with MASK inputs, the sequencer handles it.
    // We can't easily stimulate the inputs from here without digital pins.
    // This test verifies configuration doesn't crash and LUT is enabled.
    CHECK((bus.read_data(CCL_LUT0_CTRLA) & 0x01) != 0);
}

TEST_CASE("AVR8X CCL - RS Latch with EVENTA Input") {
    Machine machine(atmega4809);
    auto& bus = machine.bus();
    machine.reset();

    const u16 CCL_BASE = 0x01C0;
    const u16 EVSYS_CH0 = 0x01F0;
    const u16 CCL_SEQCTRL0 = 0x01C0;
    const u16 CCL_LUT0_CTRLA = 0x01C8;
    const u16 CCL_LUT0_CTRLB = 0x01C9;
    const u16 CCL_LUT0_TRUTH = 0x01CB;

    u16 loop[] = { 0xCFFF };
    machine.bus().load_flash(loop);
    machine.reset();

    bus.write_data(CCL_SEQCTRL0, 0x03);

    // IN0 = EVENTA (0x04), IN1 = FEEDBACK (0x01)
    bus.write_data(CCL_LUT0_CTRLB, 0x04);
    bus.write_data(CCL_LUT0_CTRLB + 1, 0x01);

    bus.write_data(CCL_LUT0_CTRLA, 0x41);
    bus.write_data(CCL_LUT0_TRUTH, 0xCA);

    // Enable global CCL
    bus.write_data(CCL_BASE, 0x01);

    machine.step();
    CHECK((bus.read_data(CCL_LUT0_CTRLA) & 0x01) != 0);
}

TEST_CASE("AVR8X CCL - D Latch Sequential Mode") {
    Machine machine(atmega4809);
    auto& bus = machine.bus();
    machine.reset();

    const u16 CCL_BASE = 0x01C0;
    const u16 CCL_SEQCTRL0 = 0x01C0;
    const u16 CCL_LUT0_CTRLA = 0x01C8;
    const u16 CCL_LUT0_CTRLB = 0x01C9;

    u16 loop[] = { 0xCFFF };
    machine.bus().load_flash(loop);
    machine.reset();

    // D-Latch mode (SEQSEL=2)
    bus.write_data(CCL_SEQCTRL0, 0x02);
    bus.write_data(CCL_LUT0_CTRLA, 0x41);
    bus.write_data(CCL_LUT0_CTRLB, 0x00);
    bus.write_data(CCL_LUT0_CTRLB + 1, 0x00);
    bus.write_data(CCL_BASE, 0x01);

    machine.step();
    CHECK((bus.read_data(CCL_LUT0_CTRLA) & 0x01) != 0);
}
