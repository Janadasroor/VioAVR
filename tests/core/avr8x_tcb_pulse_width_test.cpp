#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include "vioavr/core/ccl.hpp"
#include "vioavr/core/tcb.hpp"
#include "vioavr/core/evsys.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X TCB Pulse Width Measurement via CCL") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    
    const u16 TCB0_BASE = 0x0A80;
    const u16 TCB0_EVCTRL = 0x0A84;
    const u16 TCB0_INTCTRL = 0x0A85; // Wait! Let's check descriptors again.
    const u16 TCB0_INTFLAGS = 0x0A86;
    const u16 TCB0_STATUS = 0x0A87;

    const u16 LUT0_BASE = 0x01C8;
    const u16 CCL_BASE = 0x01C0;

    // RJMP loop
    u16 loop[] = { 0xCFFF };
    bus.load_flash(loop);
    machine.reset();

    // 1. Configure CCL LUT0 as a pass-through
    bus.write_data(CCL_BASE, 0x01);
    bus.write_data(LUT0_BASE, 0x01);
    bus.write_data(LUT0_BASE + 1, 0x05); // INSEL0 = IO PIN
    bus.write_data(LUT0_BASE + 3, 0xAA); // TRUTH = In0
    
    // 2. Configure EVSYS
    bus.write_data(0x0190, 16); // Channel 0 -> Generator 16 (LUT0)
    bus.write_data(0x01B4, 0x01); // User TCB0 -> Channel 0

    // 3. Configure TCB0 in Pulse Width Measurement Mode (Mode 4)
    bus.write_data(TCB0_EVCTRL, 0x01); // CAPTEI=1, EDGE=Rising(0)
    bus.write_data(TCB0_BASE + 0x01, 0x04); // CTRL B: Mode = 4
    bus.write_data(TCB0_BASE, 0x01);      // CTRL A: ENABLE=1

    auto ccls = machine.peripherals_of_type<Ccl>();
    auto tcbs = machine.peripherals_of_type<Tcb>();
    auto* ccl = ccls[0];
    auto* tcb0 = tcbs[0];

    // Initial: Pin=0 -> LUT0=0
    ccl->set_pin_input(0, 0, false);
    machine.step();
    CHECK((tcb0->read(TCB0_STATUS) & 0x01) == 0); 

    // Pulse Starts: Pin=1 -> LUT0=1 -> TCB Starts
    ccl->set_pin_input(0, 0, true);
    machine.step();
    CHECK((tcb0->read(TCB0_STATUS) & 0x01) != 0); 

    // Advance 50 cycles
    for (int i = 0; i < 50; ++i) machine.step();

    // Pulse Ends: Pin=0 -> LUT0=0 -> TCB Captures
    ccl->set_pin_input(0, 0, false);
    machine.step();

    CHECK((tcb0->read(TCB0_STATUS) & 0x01) == 0); 
    
    // Read captured value (CCMP)
    u16 captured = tcb0->read(0x0A8C) | (static_cast<u16>(tcb0->read(0x0A8D)) << 8);
    
    CHECK(captured >= 100);
    CHECK(captured < 110);
    
    // Check interrupt flag
    CHECK((tcb0->read(TCB0_INTFLAGS) & 0x01) != 0);
}
