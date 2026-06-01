#include "doctest.h"
#include "vioavr/core/viospice.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/logger.hpp"
#include <iostream>

using namespace vioavr::core;

TEST_CASE("AT90PWM316 ADC-to-PWM System Integration") {
    printf("Starting ADC-to-PWM Integration Test\n"); fflush(stdout);
    // Logger::set_level(LogLevel::debug);

    // Create a VioSpice instance for AT90PWM316
    const auto* desc = DeviceCatalog::find("AT90PWM316");
    REQUIRE(desc != nullptr);
    
    VioSpice machine(*desc);
    
    // Load the compiled firmware
    // Note: Path is relative to project root or build dir depending on execution
    bool loaded = machine.load_hex("tests/adc_pwm.hex");
    if (!loaded) loaded = machine.load_hex("../adc_pwm.hex");
    if (!loaded) loaded = machine.load_hex("tests/firmware/adc_pwm.hex");
    if (!loaded) loaded = machine.load_hex("../tests/firmware/adc_pwm.hex");
    REQUIRE(loaded);
    machine.reset();
    
    // 1. Set ADC0 (PB0) to 2.5V (50% of 5V VREF)
    // 2.5V / 5.0V * 1024 = 512. 512 >> 2 is 128.
    // Expected OCR0A = 128
    machine.set_external_voltage(0, 2.5); 
    
    // 2. Run for 20ms (320,000 cycles @ 16MHz)
    // Timer0 at 16MHz with /64 prescaler has a cycle of (16e6 / 64) = 250kHz
    // 256 overflow ticks = 976Hz (~1ms period)
    machine.step_duration(0.020); 
    
    // 3. Verify OCR0A register (at address 0x47 for Timer0)
    u8 ocr0a = machine.bus().read_data(0x47);
    std::cout << "ADC-to-PWM Test: OCR0A = " << (int)ocr0a << std::endl;
    std::cout << "ADC-to-PWM Test: PC = 0x" << std::hex << (int)machine.cpu().program_counter() << std::dec << std::endl;
    std::cout << "ADC-to-PWM Test: SP = 0x" << std::hex << (int)machine.cpu().stack_pointer() << std::dec << std::endl;
    
    // Check if ADC is enabled and started
    u8 adcsra = machine.bus().read_data(0x7A);
    u8 admux = machine.bus().read_data(0x7C);
    std::cout << "ADC-to-PWM Test: ADCSRA = 0x" << std::hex << (int)adcsra << std::dec << std::endl;
    std::cout << "ADC-to-PWM Test: ADMUX = 0x" << std::hex << (int)admux << std::dec << std::endl;

    // Expected ~128 (50% of 256)
    CHECK(ocr0a > 120);
    CHECK(ocr0a < 136);

    // 4. Check pin changes on PORTB bit 7 (OC0A)
    auto changes = machine.consume_pin_changes();
    int pb7_toggles = 0;
    for (const auto& c : changes) {
        std::cout << "DEBUG CHANGE: port=" << c.port_name << ", bit=" << (int)c.bit_index << ", level=" << c.level << " @ " << c.cycle_stamp << std::endl;
        if (c.bit_index == 7 && c.port_name == "PORTB") {
            pb7_toggles++;
        }
    }
    
    std::cout << "ADC-to-PWM Test: Detected " << pb7_toggles << " toggles on OC0A (PB7)" << std::endl;
    
    // 20ms at ~1ms period should give ~40 toggles (2 per cycle)
    CHECK(pb7_toggles >= 30);
}
