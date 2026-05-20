#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include "vioavr/core/evsys.hpp"
#include "vioavr/core/tca.hpp"
#include "vioavr/core/adc8x.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X EVSYS TCA to ADC Loopback") {
    // This high-fidelity test validates the asynchronous inter-peripheral signaling
    // infrastructure of the AVR8X (Mega-0) architecture.
    // Signal Chain: TCA0 Overflow -> EVSYS Channel 0 -> ADC0 Start Conversion
    
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    
    Tca* tca = nullptr;
    Adc8x* adc = nullptr;
    EventSystem* evsys = nullptr;
    
    for (auto* p : bus.peripherals()) {
        if (auto* t = dynamic_cast<Tca*>(p)) tca = t;
        if (auto* a = dynamic_cast<Adc8x*>(p)) adc = a;
        if (auto* e = dynamic_cast<EventSystem*>(p)) evsys = e;
    }
    
    REQUIRE(tca != nullptr);
    REQUIRE(adc != nullptr);
    REQUIRE(evsys != nullptr);

    const auto& tca_desc = devices::atmega4809.timers_tca[0];
    const auto& adc_desc = devices::atmega4809.adcs8x[0];
    const auto& evsys_desc = devices::atmega4809.evsys;

    // 1. Configure EVSYS Routing
    // Map EVSYS Channel 0 to Generator ID 128 (TCA0_OVF)
    bus.write_data(evsys_desc.channels_address + 0, 128); 
    // Connect ADC0 User to EVSYS Channel 0 (Select 1 = Channel 0)
    bus.write_data(adc_desc.user_event_address, 1); 

    // 2. Configure ADC
    // Enable ADC and configure it to start conversion on event
    bus.write_data(adc_desc.ctrla_address, 0x01); // ENABLE=1
    bus.write_data(adc_desc.evctrl_address, 0x01); // STARTEI=1

    // 3. Configure TCA
    // Set a short period and enable the timer
    bus.write_data(tca_desc.period_address, 5);
    bus.write_data(tca_desc.period_address + 1, 0);
    bus.write_data(tca_desc.ctrla_address, 0x01); // ENABLE=1, CLKSEL=DIV1

    // 4. Tick machine and wait for signal propagation
    bool conversion_completed = false;
    for (int i = 0; i < 100; ++i) {
        bus.tick_peripherals(1);
        
        u8 ads_flags = bus.read_data(adc_desc.intflags_address);
        if (ads_flags & 0x01) { // RESRDY bit
            conversion_completed = true;
            break;
        }
    }

    // Verify that the ADC conversion was indeed triggered by the TCA overflow
    CHECK(conversion_completed);
}
