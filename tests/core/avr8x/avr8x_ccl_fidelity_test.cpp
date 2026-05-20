#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include "vioavr/core/port_mux.hpp"
#include "vioavr/core/ccl.hpp"
#include "vioavr/core/tca.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X CCL and PORTMUX Signal Routing Fidelity") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    
    const u16 TCA0_BASE = 0x0A00;
    const u16 CCL_BASE = 0x01D0;        // LUT2 Base (used in original plan)
    const u16 CCL_CTRLA = 0x01C0;
    const u16 PORTMUX_TCAROUTEA = 0x05E4;
    const u16 PORTMUX_TCBROUTEA = 0x05E5;

    SUBCASE("TCA0 to CCL Routing through PortMux") {
        // Essential: Keep CPU running so Machine::step() ticks peripherals
        u16 loop[] = { 0xCFFF }; // RJMP -1
        machine.bus().load_flash(loop);
        machine.reset();

        // 1. Configure PortMux to route TCA0 to PORTB (ALT1)
        // TCA0 is bits 2:0. Value 1 = PORTB.
        bus.write_data(PORTMUX_TCAROUTEA, 0x01);

        // 2. Configure TCA0 for simple periodic toggling
        // CTRLA: ENABLE=1, CLKSEL=DIV1
        bus.write_data(TCA0_BASE + 0x00, 0x01);
        // CTRLB: WGMODE=NORMAL (0x00), CMP0EN=1 (0x10) => 0x10
        bus.write_data(TCA0_BASE + 0x01, 0x10);
        // PER = 100
        bus.write_data(TCA0_BASE + 0x26, 100);
        bus.write_data(TCA0_BASE + 0x27, 0);
        // CMP0 = 50
        bus.write_data(TCA0_BASE + 0x28, 50);
        bus.write_data(TCA0_BASE + 0x29, 0);

        // 3. Configure CCL LUT0
        // Global CCL Enable
        bus.write_data(CCL_CTRLA, 0x01);
        
        // LUT0 (at 0x01C8) CTRLA: ENABLE=1, OUTEN=1
        bus.write_data(0x01C8, 0x41);
        // LUT0 CTRLB: INSEL0=TCA0 (0x0A)
        bus.write_data(0x01C9, 0x0A);
        // TRUTH0: Input0 (TCA0) directly to output. (Out = In0) => Truth 0xAA (if bit0 is in0)
        // Truth table: in2 in1 in0 | out
        //              0   0   0   | 0
        //              0   0   1   | 1
        //              0   1   0   | 0
        //              0   1   1   | 1
        //              1   0   0   | 0
        //              1   0   1   | 1
        //              1   1   0   | 0
        //              1   1   1   | 1
        // Binary: 10101010 = 0xAA
        bus.write_data(0x01CB, 0xAA);

        // 4. Verification
        machine.step();
        
        // PB0 (TCA0 WO0 ALT1) should be claimed by Timer and High
        CHECK(machine.pin_mux().get_state(1, 0).owner == PinOwner::timer);
        CHECK(machine.pin_mux().get_state(1, 0).drive_level == true);
        
        // PA3 (CCL LUT0 OUT) should be claimed by CCL and High
        CHECK(machine.pin_mux().get_state(0, 3).owner == PinOwner::ccl);
        CHECK(machine.pin_mux().get_state(0, 3).drive_level == true);

        // Advance CNT past CMP0=50
        for (int i = 0; i < 200; ++i) machine.step();
        
        // Should be False now
        CHECK(machine.pin_mux().get_state(1, 0).drive_level == false);
        CHECK(machine.pin_mux().get_state(0, 3).drive_level == false);
    }

    SUBCASE("PortMux Dynamic Switching") {
        // Essential: Keep CPU running so Machine::step() ticks peripherals
        u16 loop[] = { 0xCFFF }; // RJMP -1
        machine.bus().load_flash(loop);
        machine.reset();

        bus.write_data(TCA0_BASE + 0x00, 0x01);
        bus.write_data(TCA0_BASE + 0x01, 0x10);
        bus.write_data(TCA0_BASE + 0x29, 0);  // High
        bus.write_data(TCA0_BASE + 0x28, 50); // Low

        // Step 1: Default Routing (PORTA)
        bus.write_data(PORTMUX_TCAROUTEA, 0x00);
        machine.step();
        CHECK(machine.pin_mux().get_state(0, 0).owner == PinOwner::timer);
        CHECK(machine.pin_mux().get_state(0, 0).drive_level == true);

        // Step 2: Switch to PORTB
        bus.write_data(PORTMUX_TCAROUTEA, 0x01);
        
        machine.step();
        auto state = machine.pin_mux().get_state(1, 0);
        printf("[TEST DEBUG] PB0 State: owner=%d, level=%d\n", (int)state.owner, (int)state.drive_level);

        // PA0 should be released
        CHECK(machine.pin_mux().get_state(0, 0).owner == PinOwner::gpio);
        // PB0 should be claimed
        CHECK(machine.pin_mux().get_state(1, 0).owner == PinOwner::timer);
        CHECK(machine.pin_mux().get_state(1, 0).drive_level == true);
    }
}
