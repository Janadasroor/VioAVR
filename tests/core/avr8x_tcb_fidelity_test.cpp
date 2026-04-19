#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

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
