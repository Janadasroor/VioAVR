#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/evsys.hpp"
#include "vioavr/core/tcb.hpp"
#include <vector>

using namespace vioavr::core;

TEST_CASE("AVR8X EVSYS - Event Routing Fidelity") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // Find EVSYS and first TCB
    auto evsys_list = machine.peripherals_of_type<EventSystem>();
    auto tcb_list = machine.peripherals_of_type<Tcb>();
    REQUIRE(!evsys_list.empty());
    REQUIRE(!tcb_list.empty());
    auto* evsys = evsys_list[0];
    auto* tcb0 = tcb_list[0];

    // ATmega4809 EVSYS Config:
    // CHANNEL0 (0x0180 + 0)
    // USERTCB0 (0x01A0 + 13 for TCB0? No, let's check USER mapping)
    // TCB0 user offset is usually 13 in ATmega4809.
    
    const u16 channel0_addr = 0x0190; // CHANNEL0 generator select
    const u16 user_tcb0_addr = 0x01B4; // USERTCB0 (TCB0 Event User)
    
    // 1. Configure Channel 0 to use RTC OVF (Generator ID 0x01)
    // Wait! Generator IDs for 4809:
    // RTC_OVF is 0x01.
    bus.write_data(channel0_addr, 0x01);
    
    // 2. Configure USER TCB0 to use Channel 0+1 = 1
    // (0: Off, 1: Channel 0, 2: Channel 1, ...)
    bus.write_data(user_tcb0_addr, 0x01);
    
    // 3. Configure TCB0 for Input Capture on Event
    // TCB0 CTRLA (0x0A80): ENABLE=1
    bus.write_data(0x0A80, 0x01);
    // TCB0 CTRLB (0x0A81): Mode 0x03 (Frequency Measurement)
    bus.write_data(0x0A81, 0x03); 
    // TCB0 EVCTRL (0x0A82): CAPTE=1
    // Wait! In AVR8X, EVCTRL usually has CAPTEI (Capture Event Input Enable)
    // For TCB0, INTFLAGS is at 0xA86.
    bus.write_data(0x0A84, 0x01); // EVCTRL.CAPTEI = 1

    // 4. Trigger Generator manually via EVSYS logic
    evsys->trigger_event(0x01); // RTC OVF
    
    // TCB0 should have triggered capture. INTFLAGS (0x0A86) bit 0 (CAPT)
    CHECK((bus.read_data(0x0A86) & 0x01) != 0);
    
    // 5. Clear flag and test Strobe
    bus.write_data(0x0A86, 0x01);
    CHECK((bus.read_data(0xA86) & 0x01) == 0);
    
    // EVSYS STROBE (0x0180). Trigger Channel 0 (bit 0)
    bus.write_data(0x0180, 0x01);
    
    // TCB0 should have triggered again
    CHECK((bus.read_data(0x0A86) & 0x01) != 0);
}
