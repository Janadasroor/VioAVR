#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include "vioavr/core/ccl.hpp"
#include "vioavr/core/tcb.hpp"
#include "vioavr/core/tca.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X CCL Sequential Units - JKFF and RS-Latch") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    
    const u16 CCL_BASE = 0x01C0;
    const u16 CCL_LUT0_BASE = 0x01C8;
    const u16 CCL_LUT1_BASE = 0x01CC;
    const u16 CCL_LUT2_BASE = 0x01D0;
    const u16 CCL_LUT3_BASE = 0x01D4;
    const u16 CCL_SEQCTRL0 = 0x01C1;
    const u16 CCL_SEQCTRL1 = 0x01C2;

    u16 loop[] = { 0xCFFF };
    bus.load_flash(loop);
    machine.reset();

    // Enable CCL
    bus.write_data(CCL_BASE, 0x01);

    // --- JK Flip-Flop Test (SEQ0) ---
    // In0=J, In1=K, In2=CLK (from LUT0)
    // Actually SEQ0 inputs are: 
    // In0 = LUT1 output
    // In1 = LUT0 output
    // In2 = LUT0 IN2 pin
    
    // Configure LUT0 (K and CLK source)
    bus.write_data(CCL_LUT0_BASE, 0x01); // ENABLE=1
    bus.write_data(CCL_LUT0_BASE + 1, 0x55); // INSEL1=IO (K), INSEL0=IO (J-ignored by SEQ logic)
    bus.write_data(CCL_LUT0_BASE + 2, 0x05); // INSEL2=IO (CLK)
    bus.write_data(CCL_LUT0_BASE + 3, 0xCC); // Out=In1 (K)

    // Configure LUT1 (J source)
    bus.write_data(CCL_LUT1_BASE, 0x01);
    bus.write_data(CCL_LUT1_BASE + 1, 0x05); // INSEL0=IO (J)
    bus.write_data(CCL_LUT1_BASE + 3, 0xAA); // Out=In0

    bus.write_data(CCL_SEQCTRL0, 0x02); // JKFF mode

    auto ccls = machine.peripherals_of_type<Ccl>();
    auto* ccl = ccls[0];

    // J=1, K=0, CLK=0 -> State should stay 0
    ccl->set_pin_input(1, 0, true);  // LUT1 IN0 = J = 1
    ccl->set_pin_input(0, 1, false); // LUT0 IN1 = K = 0
    ccl->set_pin_input(0, 2, false); // LUT0 IN2 = CLK = 0
    CHECK(ccl->get_seq_output(0) == false);

    // J=1, K=0, CLK Rising Edge -> State=1
    ccl->set_pin_input(0, 2, true);  // CLK=1
    CHECK(ccl->get_seq_output(0) == true);

    // J=0, K=0, CLK Falling Edge -> State=1 (Hold)
    ccl->set_pin_input(0, 2, false); 
    CHECK(ccl->get_seq_output(0) == true);

    // J=0, K=1, CLK Rising Edge -> State=0 (Reset)
    ccl->set_pin_input(0, 1, true); // K=1
    ccl->set_pin_input(1, 0, false); // J=0
    ccl->set_pin_input(0, 2, true); // CLK=1
    CHECK(ccl->get_seq_output(0) == false);

    // J=1, K=1, CLK Rising Edge -> Toggle
    ccl->set_pin_input(1, 0, true); // J=1
    ccl->set_pin_input(0, 2, false); // CLK=0
    ccl->set_pin_input(0, 2, true); // CLK=1 (Toggle 0 -> 1)
    CHECK(ccl->get_seq_output(0) == true);
    
    ccl->set_pin_input(0, 2, false);
    ccl->set_pin_input(0, 2, true); // CLK=1 (Toggle 1 -> 0)
    CHECK(ccl->get_seq_output(0) == false);

    // --- RS Latch Test (SEQ1) ---
    // In0 = LUT3 output (S)
    // In1 = LUT2 output (R)
    bus.write_data(CCL_LUT2_BASE, 0x01);
    bus.write_data(CCL_LUT2_BASE + 1, 0x05); // INSEL0=IO (R)
    bus.write_data(CCL_LUT2_BASE + 3, 0xAA); // Out=In0

    bus.write_data(CCL_LUT3_BASE, 0x01);
    bus.write_data(CCL_LUT3_BASE + 1, 0x05); // INSEL0=IO (S)
    bus.write_data(CCL_LUT3_BASE + 3, 0xAA); // Out=In0

    bus.write_data(CCL_SEQCTRL1, 0x04); // RS Latch mode

    // S=1, R=0 -> Q=1
    ccl->set_pin_input(3, 0, true); // S=1
    ccl->set_pin_input(2, 0, false); // R=0
    CHECK(ccl->get_seq_output(1) == true);

    // S=0, R=0 -> Q=1 (Hold)
    ccl->set_pin_input(3, 0, false);
    CHECK(ccl->get_seq_output(1) == true);

    // S=0, R=1 -> Q=0 (Reset)
    ccl->set_pin_input(2, 0, true); // R=1
    CHECK(ccl->get_seq_output(1) == false);

    // S=1, R=1 -> Q=0 (Reset Priority)
    ccl->set_pin_input(3, 0, true); // S=1
    CHECK(ccl->get_seq_output(1) == false);
}
