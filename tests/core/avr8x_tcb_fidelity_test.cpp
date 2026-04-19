#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include "vioavr/core/tcb.hpp"
#include "vioavr/core/memory_bus.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

TEST_CASE("AVR8X TCB - Input Capture Fidelity") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    // Load an infinite loop to keep the CPU running
    std::vector<u16> program = {0xCFFF}; // RJMP -1
    bus.load_flash(program);
    machine.cpu().reset();

    // 4809 TCB0 addresses: CTRLA=0xA80, CTRLB=0xA81, EVCTRL=0xA84, CCMP=0xA8C, CNT=0xA8A
    const u16 TCB0_CTRLA  = 0xA80;
    const u16 TCB0_CTRLB  = 0xA81;
    const u16 TCB0_EVCTRL = 0xA84;
    const u16 TCB0_INTFLAGS = 0xA86;
    const u16 TCB0_CNT    = 0xA8A;
    const u16 TCB0_CCMP   = 0xA8C;

    // EVSYS addresses: CHANNEL0=0x0190, USER_TCB0=0x1B4
    const u16 EVSYS_CHANNEL0 = 0x0190;
    const u16 EVSYS_USER_TCB0 = 0x01B4;

    // 1. Setup TCB0
    // CTRLB: Mode 3 (Frequency Measurement)
    bus.write_data(TCB0_CTRLB, 0x03U);
    // EVCTRL: CAPTEI=1
    bus.write_data(TCB0_EVCTRL, 0x01U);
    // CTRLA: ENABLE=1
    bus.write_data(TCB0_CTRLA, 0x01U);

    // 2. Setup EVSYS
    // CHANNEL0 = 0x01 (Generator for PORTA.PIN0)
    bus.write_data(EVSYS_CHANNEL0, 0x01U);
    // USER_TCB0 = 0x01 (Listen to Channel 0. In VioAVR index+1 = 1)
    bus.write_data(EVSYS_USER_TCB0, 0x01U);

    // 3. Advance some cycles
    machine.run(100);
    
    // Verify TCNT has advanced
    u16 cnt1 = bus.read_data(TCB0_CNT) | (static_cast<u16>(bus.read_data(TCB0_CNT + 1)) << 8);
    CHECK(cnt1 >= 100);

    // 4. Trigger Event manually via EVSYS Strobe or direct trigger
    // Actually, in the test we can just call trigger_event on the EVSYS if we have access.
    // Or we can trigger a real pin change if PORTA is connected.
    // Let's use EVSYS STROBE (0x0180) to fire Channel 0.
    bus.write_data(0x180, 0x01U); // Strobe CH0

    // 5. Verify Capture
    u16 ccmp = bus.read_data(TCB0_CCMP) | (static_cast<u16>(bus.read_data(TCB0_CCMP + 1)) << 8);
    CHECK(ccmp == cnt1);
    
    // Verify Reset (Frequency Measurement mode resets CNT to 0)
    u16 cnt2 = bus.read_data(TCB0_CNT) | (static_cast<u16>(bus.read_data(TCB0_CNT + 1)) << 8);
    CHECK(cnt2 == 0);
    
    // Verify Interrupt Flag
    CHECK((bus.read_data(TCB0_INTFLAGS) & 0x01U) != 0);
}
TEST_CASE("AVR8X TCB - Cascaded Mode Fidelity") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    // Load NOPs to maintain 1 cycle per machine.run(1)
    std::vector<u16> program(1000, 0x0000); 
    bus.load_flash(program);
    machine.cpu().reset();

    // 1. Configure TCA0 to overflow every 10 cycles
    bus.write_data(0xA26, 9);
    bus.write_data(0xA27, 0); // High byte
    bus.write_data(0xA00, 0x01); // ENABLE

    // 2. Configure TCB0 for Cascaded Mode (CLKSEL=2)
    bus.write_data(0xA8C, 0xFF); // CCMPL
    bus.write_data(0xA8D, 0xFF); // CCMPH
    bus.write_data(0xA80, (0x02 << 1) | 0x01);

    // 3. Tick 5 cycles
    machine.run(5);
    CHECK(bus.read_data(0xA20) == 5);
    CHECK(bus.read_data(0xA8A) == 0);

    // 4. Tick 5 more cycles (Total 10)
    machine.run(5);
    // TCA just reached 0 (overflowed on 10th cycle)
    CHECK(bus.read_data(0xA20) == 0);
    CHECK(bus.read_data(0xA8A) == 1);
}
TEST_CASE("AVR8X TCB - Single Shot Mode 6") {
    Tcb tcb(devices::atmega4809.timers_tcb[0]);
    tcb.reset();

    // Mode 6: Single-shot
    tcb.write(devices::atmega4809.timers_tcb[0].ctrlb_address, 0x06);
    // CCMP = 10
    tcb.write(devices::atmega4809.timers_tcb[0].ccmp_address, 10);
    tcb.write(devices::atmega4809.timers_tcb[0].ccmp_address + 1, 0);
    // Enable
    tcb.write(devices::atmega4809.timers_tcb[0].ctrla_address, 0x01);

    // Tick 10 cycles
    tcb.tick(10);
    CHECK((tcb.read(devices::atmega4809.timers_tcb[0].ctrla_address) & 0x01) != 0); // Still enabled at match
    
    // One more tick should trigger logic and disable
    tcb.tick(1);
    CHECK((tcb.read(devices::atmega4809.timers_tcb[0].ctrla_address) & 0x01) == 0);
    CHECK((tcb.read(devices::atmega4809.timers_tcb[0].intflags_address) & 0x01) != 0);
}

TEST_CASE("AVR8X TCB - 8-bit PWM (PWM8) Fidelity") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    // Load NOPs
    std::vector<u16> program(1000, 0x0000);
    bus.load_flash(program);
    machine.cpu().reset();

    // Configure TCB0 for PWM8 (Mode 1)
    bus.write_data(0xA81, 0x01); // PWM8
    bus.write_data(0xA8C, 10);   // CCMPL = 10
    bus.write_data(0xA80, 0x01); // ENABLE

    // Initial CNT=0
    CHECK(bus.read_data(0xA8A) == 0);

    // Tick 10 cycles
    machine.run(10);
    CHECK(bus.read_data(0xA8A) == 10);

    // Next tick should reset to 0 and set CAPT flag
    machine.run(1);
    CHECK(bus.read_data(0xA8A) == 0);
    CHECK((bus.read_data(0xA86) & 0x01) != 0);
}
