#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/evsys.hpp"
#include "vioavr/core/tca.hpp"
#include "vioavr/core/tcb.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

TEST_CASE("EVSYS: TCA0 OVF -> TCB0 Capture") {
    Machine machine(atmega4809);
    auto& bus = machine.bus();
    
    // 1. Configure EVSYS
    // CHANNEL0 (0x190) -> Generator 128 (TCA0 OVF)
    bus.write_data(0x0190, 128);
    
    // USER TCB0 (0x1B4) -> Channel 0 (value 1)
    bus.write_data(0x01B4, 1);
    
    // 2. Configure TCA0
    // PER = 10 (Write High first, then Low)
    bus.write_data(0x0A27, 0);
    bus.write_data(0x0A26, 10);
    // ENABLE = 1, CLK_PER
    bus.write_data(0x0A00, 0x01);
    
    // 3. Configure TCB0
    // MODE = 2 (Input Capture)
    bus.write_data(0x0A81, 0x02);
    // CAPTEI = 1 (Event Input Enable)
    bus.write_data(0x0A84, 0x01);
    // ENABLE = 1, CLK_PER
    bus.write_data(0x0A80, 0x01);
    
    // 4. Run simulation
    // TCA0 will overflow after 11 cycles. We tick peripherals directly
    // since no program is loaded (CPU would halt in machine.step()).
    for (int i=0; i < 100; ++i) {
        bus.tick_peripherals(1);
    }
    
    // 5. Check TCB0
    // CCMP should have captured some value (it increments every cycle)
    // Read Low then High
    u8 cap_l = bus.read_data(0x0A8C);
    u8 cap_h = bus.read_data(0x0A8D);
    u16 captured = cap_l | (static_cast<u16>(cap_h) << 8);
    u8 tcb_intflags = bus.read_data(0x0A86);
    
    // TCA0 overflows at TCNT=10. 
    // Step 0: TCNT=0
    // Step 1: TCNT=1
    // ...
    // Step 10: TCNT=10 (MATCH! OVF set next step)
    // Step 11: TCNT=0, OVF=1 -> Trigger Event -> TCB0 Captures
    
    CHECK((tcb_intflags & 0x01) != 0); // CAPT flag should be set
    CHECK(captured > 0);
    printf("[DEBUG] EVSYS Test: TCB0 Captured %d, Flags=0x%02X\n", (int)captured, (int)tcb_intflags);
}
