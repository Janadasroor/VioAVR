#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include "vioavr/core/ccl.hpp"
#include "vioavr/core/tcb.hpp"
#include "vioavr/core/tca.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X CCL Sequential Logic Fidelity") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    
    // Base addresses for ATmega4809
    const u16 TCA0_BASE = 0x0A00;
    const u16 TCB0_BASE = 0x0A80;
    const u16 TCB2_BASE = 0x0AA0; // Linked to LUT2
    const u16 CCL_BASE = 0x01C0;
    const u16 CCL_LUT0_BASE = 0x01C8;
    const u16 CCL_LUT1_BASE = 0x01CC;
    const u16 CCL_LUT2_BASE = 0x01D0;
    const u16 CCL_LUT3_BASE = 0x01D4;
    const u16 CCL_SEQCTRL0 = 0x01C1;
    const u16 CCL_SEQCTRL1 = 0x01C2;

    // RJMP -1 loop to keep CPU ticking
    u16 loop[] = { 0xCFFF };
    bus.load_flash(loop);
    machine.reset();

    // 1. Configure CCL Global Enable
    bus.write_data(CCL_BASE, 0x01);

    // --- D Flip-Flop Test (SEQ0) ---
    // LUT0: Clock (TCB0)
    bus.write_data(CCL_LUT0_BASE, 0x01);
    bus.write_data(CCL_LUT0_BASE + 1, 0xC0); // INSEL1=TCB
    bus.write_data(CCL_LUT0_BASE + 3, 0xCC); // Out=In1

    // LUT1: Data (TCA0)
    bus.write_data(CCL_LUT1_BASE, 0x01);
    bus.write_data(CCL_LUT1_BASE + 1, 0x0A); // INSEL0=TCA0
    bus.write_data(CCL_LUT1_BASE + 3, 0xAA); // Out=In0

    bus.write_data(CCL_SEQCTRL0, 0x01); // DFF mode

    // --- Latch Test (SEQ1) ---
    // LUT2: Gate (TCB2)
    bus.write_data(CCL_LUT2_BASE, 0x01);
    bus.write_data(CCL_LUT2_BASE + 1, 0xC0); // INSEL1=TCB
    bus.write_data(CCL_LUT2_BASE + 3, 0xCC); // Out=In1

    // LUT3: Data (TCA0)
    bus.write_data(CCL_LUT3_BASE, 0x01);
    bus.write_data(CCL_LUT3_BASE + 1, 0x0A); // INSEL0=TCA0
    bus.write_data(CCL_LUT3_BASE + 3, 0xAA); // Out=In0

    bus.write_data(CCL_SEQCTRL1, 0x03); // Latch mode

    // Configure Sources
    bus.write_data(TCA0_BASE, 0x01);
    bus.write_data(TCA0_BASE + 0x01, 0x10);
    bus.write_data(TCA0_BASE + 0x27, 0); bus.write_data(TCA0_BASE + 0x26, 200);
    bus.write_data(TCA0_BASE + 0x29, 0); bus.write_data(TCA0_BASE + 0x28, 100);

    bus.write_data(TCB0_BASE, 0x01);
    bus.write_data(TCB0_BASE + 0x01, 0x11);
    bus.write_data(TCB0_BASE + 0x0D, 5); bus.write_data(TCB0_BASE + 0x0C, 10);

    bus.write_data(TCB2_BASE, 0x01);
    bus.write_data(TCB2_BASE + 0x01, 0x11);
    bus.write_data(TCB2_BASE + 0x0D, 5); bus.write_data(TCB2_BASE + 0x0C, 10);

    auto ccls = machine.peripherals_of_type<Ccl>();
    auto* ccl = ccls[0];

    // Initial check
    machine.step();
    // At cycle 2, TCBs and TCA0 are enabled. 
    // Data=1, Gate=1. Latch should be 1.
    CHECK(ccl->get_seq_output(1) == true);
    
    // Advance and verify DFF
    for (int i = 0; i < 20; ++i) machine.step();
    CHECK(ccl->get_seq_output(0) == true);
}
