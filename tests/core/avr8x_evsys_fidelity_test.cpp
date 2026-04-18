#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/rtc.hpp"
#include "vioavr/core/tca.hpp"
#include "vioavr/core/evsys.hpp"
#include "doctest.h"

using namespace vioavr::core;

TEST_CASE("AVR8X EVSYS + TCA + RTC Fidelity Test") {
    const auto* device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();

    // 1. Configure RTC to overflow every 10 cycles
    // RTC Base: 0x0140. PER: +0x0A (10, 11). CTRLA: +0x00.
    bus.write_data(0x140 + 0x0A, 9); // PERL = 9 (10 counts)
    bus.write_data(0x140 + 0x0B, 0); 
    bus.write_data(0x140 + 0x00, 0x01); // ENABLE=1

    // 2. Configure EVSYS
    // EVSYS Base: 0x0180.
    // CHANNEL0 (0x190) selects RTC_OVF (Generator 6)
    bus.write_data(0x180 + 0x10, 6); 

    // USERTCA0 (0x1B3) selects CHANNEL0 (Value 1)
    bus.write_data(0x180 + 0x33, 1);

    // 3. Configure TCA0
    // TCA0 Base: 0x0A00. CTRLA: +0x00.
    bus.write_data(0x0A00 + 0x00, 0x01); // ENABLE=1
    u8 ctrla = bus.read_data(0x0A00 + 0x00);
    CHECK(ctrla == 0x01); // Ensure it was written

    // Initial TCNT should be 0
    u16 tcnt = bus.read_data(0x0A00 + 0x20) | (bus.read_data(0x0A00 + 0x21) << 8);
    CHECK(tcnt == 0);

    // Ensure RTC is enabled
    u8 rtc_ctrla = bus.read_data(0x140 + 0x00);
    CHECK(rtc_ctrla == 0x01);

    // Run 5 cycles. RTC should be at 5. TCA should be at 5 (ticks on every cycle by default)
    // Wait! Event system triggers on RTC_OVF.
    // So TCA should only tick when RTC overflows (every 10 cycles).
    // EXCEPT: TCA also ticks on its own I/O clock.
    // In our simplified test, we want TCA to respond to the event.
    
    // Let's disable TCA I/O clock tick for this test if possible,
    // or just check that it ticks EXTRA on events.
    // Our Tca::on_event() calls perform_tick().
    
    bus.tick_peripherals(5);
    tcnt = bus.read_data(0x0A00 + 0x20) | (bus.read_data(0x0A00 + 0x21) << 8);
    CHECK(tcnt == 5); // From I/O clock

    bus.tick_peripherals(5); // RTC should overflow now (at 10)
    tcnt = bus.read_data(0x0A00 + 0x20) | (bus.read_data(0x0A00 + 0x21) << 8);
    // TCA TCNT: 10 (from clock) + 1 (from event) = 11?
    // Wait, RTC ticks, then EVSYS triggers, then TCA on_event ticks.
    CHECK(tcnt == 11);

    bus.tick_peripherals(10); // Another RTC overflow
    tcnt = bus.read_data(0x0A00 + 0x20) | (bus.read_data(0x0A00 + 0x21) << 8);
    CHECK(tcnt == 22); // 20 from clock + 2 from events
}
