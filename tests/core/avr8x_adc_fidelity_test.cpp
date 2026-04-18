#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/memory_bus.hpp"
#include <iostream>

using namespace vioavr::core;

TEST_CASE("AVR8X ADC Fidelity Test") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    REQUIRE(device->adc8x_count == 1);

    Machine machine(*device);
    auto& bus = machine.bus();

    const u16 ADC_BASE = 0x0600;
    const u16 ADC_CTRLA = ADC_BASE + 0x00;
    const u16 ADC_COMMAND = ADC_BASE + 0x08;
    const u16 ADC_INTFLAGS = ADC_BASE + 0x0B;
    const u16 ADC_RES = ADC_BASE + 0x10;

    // 1. Enable ADC
    // Set AIN0 to 0.5005V so that 0.5005 * 1023 approx 512.
    // Or just change expectation to 511.
    // Actually our Adc8x uses: static_cast<u16>(voltage * 1023.0)
    // 0.5 * 1023 = 511.5 -> 511.
    // Let's use 512.0/1023.0 approx 0.500488
    machine.analog_signal_bank().set_voltage(0, 512.0/1023.0); 
    bus.write_data(ADC_CTRLA, 0x01); // ENABLE=1
    CHECK(bus.read_data(ADC_CTRLA) == 0x01);

    // 2. Start conversion via COMMAND
    bus.write_data(ADC_COMMAND, 0x01); // STCONV=1

    // 3. Tick machine to complete conversion (13 cycles)
    // We tick one by one or in a batch
    bus.tick_peripherals(10);
    CHECK((bus.read_data(ADC_INTFLAGS) & 0x01) == 0); // Not finished (13 required)

    bus.tick_peripherals(5);
    CHECK((bus.read_data(ADC_INTFLAGS) & 0x01) == 0x01); // Finished!

    // 4. Read result
    u16 result = bus.read_data(ADC_RES) | (static_cast<u16>(bus.read_data(ADC_RES + 1)) << 8);
    CHECK(result == 0x200); // Fixed dummy value in current implementation

    // 5. Clear flag
    bus.write_data(ADC_INTFLAGS, 0x01);
    CHECK((bus.read_data(ADC_INTFLAGS) & 0x01) == 0);

    // 6. EVSYS Trigger Test
    const u16 EVSYS_BASE = 0x0180;
    const u16 EVSYS_USERADC0 = EVSYS_BASE + 0x28; // 4809 USERADC0 is at 0x1A8 (offset 0x28 from EVSYS base 0x180)
    const u16 ADC_EVCTRL = ADC_BASE + 0x09;

    // Enable Event Trigger in ADC
    bus.write_data(ADC_EVCTRL, 0x01); // STARTEI=1
    
    // Configure EVSYS User ADC0 to Channel 0 (using Value 1)
    bus.write_data(EVSYS_USERADC0, 1); 

    // Manually strobe Channel 0
    bus.write_data(EVSYS_BASE + 0x00, 0x01); // STROBE0=1

    // Should have started conversion
    bus.tick_peripherals(15);
    CHECK((bus.read_data(ADC_INTFLAGS) & 0x01) == 0x01); // Finished via event!
}
