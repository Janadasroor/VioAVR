#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X EVSYS Chaining: TCA0 -> CCL -> ADC") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    
    // Addresses for ATmega4809
    const u16 TCA0_BASE = 0xA00;
    const u16 EVSYS_CHANNEL0 = 0x190;
    const u16 EVSYS_CHANNEL1 = 0x191;
    const u16 EVSYS_USER_CCL_LUT0A = 0x1A0;
    const u16 EVSYS_USER_ADC0 = 0x1A8;
    const u16 CCL_CTRLA = 0x1C0;
    const u16 CCL_LUT0_CTRLA = 0x1C8;
    const u16 CCL_LUT0_CTRLB = 0x1C9;
    const u16 CCL_LUT0_TRUTH = 0x1CB;
    const u16 ADC0_BASE = 0x600;

    // 5. Run and Verify
    // We need some code in flash to keep the CPU "running" so ticks are processed
    u16 loop[] = { 0xCFFF }; // RJMP -1
    machine.bus().load_flash(loop);
    machine.reset();

    // 1. Setup EVSYS Routing
    // Channel 0: Source = TCA0_OVF (128)
    bus.write_data(EVSYS_CHANNEL0, 128);
    // User CCL_LUT0A: Connect to Channel 0 (value 1)
    bus.write_data(EVSYS_USER_CCL_LUT0A, 1);
    
    // Channel 1: Source = CCL_LUT0 (16)
    bus.write_data(EVSYS_CHANNEL1, 16);
    // User ADC0: Connect to Channel 1 (value 2)
    bus.write_data(EVSYS_USER_ADC0, 2);

    // 2. Setup CCL
    // Global Enable
    bus.write_data(CCL_CTRLA, 0x01);
    // LUT0: Enable, INSEL0 = EVENTA (0x03)
    bus.write_data(CCL_LUT0_CTRLA, 0x01);
    bus.write_data(CCL_LUT0_CTRLB, 0x03); 
    // Truth table: Out = IN0. 
    // In bits: (IN2, IN1, IN0). Patterns: 001, 011, 101, 111 should be 1.
    // 0xAA = 10101010. Patterns: 7, 5, 3, 1 are set. 
    // Pattern 1 (001) is IN0 only. Pattern 3 (011) is IN1 & IN0.
    // So 0xAA works for "Out = IN0".
    bus.write_data(CCL_LUT0_TRUTH, 0xAA);

    // 3. Setup ADC
    // ENABLE=1
    bus.write_data(ADC0_BASE + 0x00, 0x01);
    // EVCTRL: STARTEI=1 (Start conversion on event)
    bus.write_data(ADC0_BASE + 0x09, 0x01);
    // MUXPOS: AIN0 (0x00)
    bus.write_data(ADC0_BASE + 0x06, 0x00);

    // 4. Setup TCA0
    // PER = 10 (Fast overflow for test)
    bus.write_data(TCA0_BASE + 0x26, 10);
    bus.write_data(TCA0_BASE + 0x27, 0);
    // CTRLA: ENABLE=1, CLKSEL=DIV1
    bus.write_data(TCA0_BASE + 0x00, 0x01);

    // 5. Run and Verify
    // We need some code in flash to keep the CPU "running" so ticks are processed
    SUBCASE("Chain Works with Pass-through Truth 0xAA") {
        bus.write_data(CCL_LUT0_TRUTH, 0xAA);
        
        // Reset peripherals but keep EVSYS/CCL config? 
        // No, reset() clears them. We just wrote 0xAA to TRUTH, which is fine since we already configured others.
        
        // Initially ADC should not be converting
        CHECK((bus.read_data(ADC0_BASE + 0x0D) & 0x01) == 0);

        for (int i = 0; i < 12; ++i) machine.step();

        CHECK((bus.read_data(ADC0_BASE + 0x0D) & 0x01) == 0x01);
    }

    SUBCASE("Chain Blocked by Truth 0x00") {
        bus.write_data(CCL_LUT0_TRUTH, 0x00);
        
        CHECK((bus.read_data(ADC0_BASE + 0x0D) & 0x01) == 0);

        for (int i = 0; i < 12; ++i) machine.step();

        // ADC should NOT be converting because CCL output stayed low
        CHECK((bus.read_data(ADC0_BASE + 0x0D) & 0x01) == 0);
    }
}
