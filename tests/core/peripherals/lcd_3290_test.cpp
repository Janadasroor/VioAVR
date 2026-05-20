#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega3290a.hpp"

using namespace vioavr::core;

TEST_CASE("LCD Controller: ATmega3290A Device Mapping") {
    auto desc = devices::atmega3290a;
    Machine machine(desc);
    auto& bus = machine.bus();
    auto& pin_mux = machine.pin_mux();
    
    // Setup: Enable all segments
    bus.write_data(0xE5, 0x0F); // LCDPM = 15 (SEG0-39)
    bus.write_data(0xE4, 0x80); // LCDEN
    
    SUBCASE("Verify Specific Segments Claim Pins") {
        // SEG0: PA4 (0x22)
        CHECK(pin_mux.get_state_by_address(0x22, 4).owner == PinOwner::lcd);
        
        // SEG11: PC5 (0x28)
        CHECK(pin_mux.get_state_by_address(0x28, 5).owner == PinOwner::lcd);
        
        // SEG27: PJ6 (0xDD)
        CHECK(pin_mux.get_state_by_address(0xDD, 6).owner == PinOwner::lcd);
        
        // SEG39: PH4 (0xDA)
        CHECK(pin_mux.get_state_by_address(0xDA, 4).owner == PinOwner::lcd);
        
        // COM0: PA0 (0x22)
        CHECK(pin_mux.get_state_by_address(0x22, 0).owner == PinOwner::lcd);
    }
    
    SUBCASE("Verify Masking (LCDPM)") {
        // SEG13: PC3 (0x28)
        // If LCDPM is 0, only SEG0-12 are LCD pins.
        bus.write_data(0xE5, 0x00); 
        CHECK(pin_mux.get_state_by_address(0x28, 3).owner == PinOwner::gpio);
        
        // If LCDPM is 1, SEG0-14 are LCD pins.
        bus.write_data(0xE5, 0x01);
        CHECK(pin_mux.get_state_by_address(0x28, 3).owner == PinOwner::lcd);
    }
}
