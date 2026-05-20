#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/evsys.hpp"
#include "vioavr/core/ccl.hpp"
#include "vioavr/core/ac8x.hpp"
#include "vioavr/core/tcb.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include "vioavr/core/analog_signal_bank.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X CCL + EVSYS Fidelity: AC0 -> EVSYS -> CCL -> EVSYS -> TCB0") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    auto* evsys = static_cast<EventSystem*>(bus.get_peripheral_by_name("EVSYS"));
    
    // 1. Configure AC0
    // AC0 Generator ID = 32 (0x20)
    // Positive Input = 0 (AIN0), Negative Input = 3 (DACREF)
    // DACREF = 0x80 (approx 0.5 * VDD)
    bus.write_data(0x0680, 0x01); // AC0 CTRLA: ENABLE=1
    bus.write_data(0x0682, 0x03); // AC0 MUXCTRLA: POS=AIN0, NEG=DACREF
    bus.write_data(0x0684, 0x80); // AC0 DACREF: DACREF=0x80
    
    // 2. Configure EVSYS Channel 0: AC0_OUT -> CCL_LUT0_EV0 (which is EVENTA for LUT0)
    // Generator AC0_OUT = 32
    bus.write_data(0x0190, 32); // CHANNEL0 = 32
    // User USERCCLLUT0A (Address 0x1A0 in 4809) -> Channel 0 (Value 1)
    bus.write_data(0x01A0, 0x01); // Route Channel 0 to LUT0 EV0 (EVENTA)
    
    // 3. Configure CCL LUT0
    bus.write_data(0x01C0, 0x01); // Global CCL ENABLE=1
    // ENABLE=1, OUTEN=1
    bus.write_data(0x01C8, 0x09); 
    // INSEL0 = EVENTA (0x03), TRUTH = 0x02 (IN0 only)
    bus.write_data(0x01C9, 0x03); // INSEL0=EVENTA, INSEL1=MASK
    bus.write_data(0x01CB, 0x02); // TRUTH: output 1 if IN0 is 1
    
    // 4. Configure EVSYS Channel 1: CCL_LUT0_OUT -> TCB0_CAPT
    // Generator CCL_LUT0_OUT = 16 (0x10)
    bus.write_data(0x0191, 16); // CHANNEL1 = 16
    // User USERTCEB0 (TCB0 Capture) is 0x1B4. Index 20 in 4809
    bus.write_data(0x01B4, 0x02); // Route Channel 1 to TCB0
    
    // 5. Configure TCB0
    // ENABLE=1, CLKSEL=CLKPER/1 (0x0)
    bus.write_data(0xA80, 0x01);
    // MODE=Input Capture (2), CAPTEI=1
    bus.write_data(0xA81, 0x02);
    bus.write_data(0xA84, 0x01); // EVCTRL: CAPTEI=1
    
    // 6. Stimulate AC0
    auto& signal_bank = machine.analog_signal_bank();
    signal_bank.set_voltage(0, 0.1); // Lower than DACREF
    bus.tick_peripherals(10);
    
    CHECK(bus.read_data(0xA86) == 0); // CAPT flag should be 0
    
    signal_bank.set_voltage(0, 0.9); // Higher than DACREF -> AC0_OUT goes high
    bus.tick_peripherals(10);
    
    u8 tcb0_flags = bus.read_data(0xA86);
    u16 tcb0_ccmp = bus.read_data(0xA8C) | (bus.read_data(0xA8D) << 8);
    
    printf("CCL Stress Test: TCB0 Flags=0x%02X, CCMP=%d\n", tcb0_flags, tcb0_ccmp);
    
    // If CCL works, flags should be 0x01
    CHECK((tcb0_flags & 0x01) != 0);
}

TEST_CASE("AVR8X CCL: Sequential Unit (DFF) Test") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    
    // Global CCL ENABLE=1
    bus.write_data(0x01C0, 0x01);
    
    // 1. Configure LUT1 (Data)
    // Always Output 1 initially
    bus.write_data(0x01CC, 0x01); // ENABLE=1
    bus.write_data(0x01CF, 0xFF); // TRUTH=0xFF (Always 1)
    
    // 2. Configure LUT0 (Clock)
    // Input 0 is from Pin PA0 (IN0)
    bus.write_data(0x01C8, 0x01); // ENABLE=1
    bus.write_data(0x01C9, 0x05); // INSEL0 = IO
    bus.write_data(0x01CB, 0x02); // TRUTH = IN0
    
    // 3. Configure SEQCTRL0
    // MODE = DFF (0x01)
    bus.write_data(0x01C1, 0x01);
    
    auto* ccl = static_cast<Ccl*>(bus.get_peripheral_by_name("CCL"));
    
    // Initially PA0=0 -> LUT0=0. DFF state=0
    ccl->set_pin_input(0, 0, false);
    bus.tick_peripherals(10);
    CHECK(ccl->get_lut_output(0) == false);
    
    // Set PA0=1 -> LUT0=1 (Rising Edge). Latches LUT1=1.
    ccl->set_pin_input(0, 0, true);
    bus.tick_peripherals(10);
    // LUT0 output should now be the DFF state (1)
    CHECK(ccl->get_lut_output(0) == true);
    
    // Change LUT1 to 0
    bus.write_data(0x01CF, 0x00); // TRUTH=0 (Always 0)
    bus.tick_peripherals(10);
    // Should still be 1 (latched)
    CHECK(ccl->get_lut_output(0) == true);
    
    // Falling Edge (PA0=0)
    ccl->set_pin_input(0, 0, false);
    bus.tick_peripherals(10);
    CHECK(ccl->get_lut_output(0) == true);
    
    // Rising Edge (PA0=1) -> Latches LUT1=0
    ccl->set_pin_input(0, 0, true);
    bus.tick_peripherals(10);
    CHECK(ccl->get_lut_output(0) == false);
}

TEST_CASE("AVR8X Timer Cascade: TCA0 OVF -> TCB0 Clock") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    
    // 1. Configure TCA0
    // ENABLE=1, CLKSEL=DIV1
    bus.write_data(0xA00, 0x01);
    // PERIOD = 9 (Overflow every 10 cycles)
    bus.write_data(0xA26, 0x09);
    bus.write_data(0xA27, 0x00);
    
    // 2. Configure TCB0
    // ENABLE=1, CLKSEL=EVENT (Value 2 in bits 2:1 => 0x04)
    bus.write_data(0xA80, 0x05);
    // MODE=Periodic Interrupt (0)
    bus.write_data(0xA81, 0x00);
    // CCMP = 0xFFFF (Don't reset early)
    bus.write_data(0xA8C, 0xFF);
    bus.write_data(0xA8D, 0xFF);
    
    // 3. Tick 50 cycles
    // TCA0 should overflow 5 times.
    // TCB0 should reach 5.
    bus.tick_peripherals(50);
    
    u16 tcb0_cnt = bus.read_data(0xA8A) | (bus.read_data(0xA8B) << 8);
    printf("Timer Cascade Test: TCB0 CNT=%d\n", tcb0_cnt);
    
    CHECK(tcb0_cnt == 5);
}
