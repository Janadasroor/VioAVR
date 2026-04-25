#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/lcd_controller.hpp"
#include "vioavr/core/pin_mux.hpp"

using namespace vioavr::core;

TEST_CASE("LCD Controller: Basic Register I/O") {
    DeviceDescriptor desc {};
    desc.lcd_count = 1;
    desc.lcds[0].lcdcra_address = 0xE4;
    desc.lcds[0].lcdcrb_address = 0xE5;
    desc.lcds[0].lcdfrr_address = 0xE6;
    desc.lcds[0].lcdccr_address = 0xE7;
    desc.lcds[0].lcddr_base_address = 0xEC;
    desc.lcds[0].lcddr_count = 20;

    Machine machine(desc);
    auto& bus = machine.bus();

    SUBCASE("Control Register Access") {
        bus.write_data(0xE4, 0x80); // Enable LCD
        CHECK(bus.read_data(0xE4) == 0x80);
        
        bus.write_data(0xE5, 0x3F);
        CHECK(bus.read_data(0xE5) == 0x3F);
    }

    SUBCASE("LCDDR Access") {
        bus.write_data(0xEC, 0xAA); // COM0
        CHECK(bus.read_data(0xEC) == 0xAA);
        
        bus.write_data(0xFF, 0x55); // LCDDR19
        CHECK(bus.read_data(0xFF) == 0x55);
    }
}

TEST_CASE("LCD Controller: Interrupt Handling") {
    DeviceDescriptor desc {};
    desc.lcd_count = 1;
    desc.lcds[0].lcdcra_address = 0xE4;
    desc.lcds[0].lcdcrb_address = 0xE5;
    desc.lcds[0].lcdfrr_address = 0xE6;
    desc.lcds[0].lcdccr_address = 0xE7;
    desc.lcds[0].vector_index = 22;

    Machine machine(desc);
    auto& bus = machine.bus();

    // Enable LCD and Interrupt
    bus.write_data(0xE4, 0x88); // LCDEN | LCDIE
    
    // Advance cycles
    bus.tick_peripherals(256000);

    CHECK((bus.read_data(0xE4) & 0x10) != 0); // LCDIF set
    
    SUBCASE("Clearing Interrupt Flag") {
        // Clear LCDIF by writing 1
        bus.write_data(0xE4, 0x10);
        CHECK((bus.read_data(0xE4) & 0x10) == 0);
    }
}

TEST_CASE("LCD Controller: Pin Arbitration") {
    DeviceDescriptor desc {};
    desc.lcd_count = 1;
    desc.lcds[0].lcdcra_address = 0xE4;
    desc.lcds[0].lcdcrb_address = 0xE5;
    desc.lcds[0].lcdfrr_address = 0xE6;
    desc.lcds[0].lcdccr_address = 0xE7;
    
    // Setup some segment pins
    desc.lcds[0].segment_pins[0] = {0x100, 0}; // SEG0
    desc.lcds[0].segment_pins[24] = {0x200, 7}; // SEG24
    
    Machine machine(desc);
    auto& bus = machine.bus();
    auto& pin_mux = machine.pin_mux();
    
    // Register the ports so PinMux knows them
    pin_mux.register_port(0x100, 0);
    pin_mux.register_port(0x200, 1);

    // Initially not claimed
    CHECK(pin_mux.get_state_by_address(0x100, 0).owner == PinOwner::gpio);

    // Enable LCD
    bus.write_data(0xE4, 0x80); // LCDEN
    
    // Default LCDPM is 0 (SEG0-12)
    CHECK(pin_mux.get_state_by_address(0x100, 0).owner == PinOwner::lcd);
    CHECK(pin_mux.get_state_by_address(0x200, 7).owner == PinOwner::gpio); // SEG24 not active

    // Change LCDPM to 7 (SEG0-24)
    bus.write_data(0xE5, 0x07);
    CHECK(pin_mux.get_state_by_address(0x200, 7).owner == PinOwner::lcd);

    // Disable LCD
    bus.write_data(0xE4, 0x00);
    CHECK(pin_mux.get_state_by_address(0x100, 0).owner == PinOwner::gpio);
    CHECK(pin_mux.get_state_by_address(0x200, 7).owner == PinOwner::gpio);
}

TEST_CASE("LCD Controller: Hardware-Accurate Timing") {
    DeviceDescriptor desc {};
    desc.lcd_count = 1;
    desc.lcds[0].lcdcra_address = 0xE4;
    desc.lcds[0].lcdcrb_address = 0xE5;
    desc.lcds[0].lcdfrr_address = 0xE6;
    desc.lcds[0].lcdccr_address = 0xE7;
    desc.lcds[0].vector_index = 22;

    Machine machine(desc);
    auto& bus = machine.bus();

    SUBCASE("Default timing (P=16, K=1, D=1)") {
        bus.write_data(0xE4, 0x80); // Enable
        // Default LCDFRR is 0 (P=16, K=1)
        // Default LCDCRB is 0 (D=1)
        // Cycles = 16 * 1 * 1 = 16
        
        bus.tick_peripherals(15);
        CHECK((bus.read_data(0xE4) & 0x10) == 0);
        
        bus.tick_peripherals(1);
        CHECK((bus.read_data(0xE4) & 0x10) != 0);
    }

    SUBCASE("Custom timing (P=64, K=4, D=3)") {
        // LCDFRR: PS=1 (64), CD=3 (K=4) -> 0x13
        // LCDCRB: MUX=2 (1/3) -> 0x20
        bus.write_data(0xE6, 0x13);
        bus.write_data(0xE5, 0x20);
        bus.write_data(0xE4, 0x80); // Enable
        
        // Cycles = 64 * 4 * 3 = 768
        bus.tick_peripherals(767);
        CHECK((bus.read_data(0xE4) & 0x10) == 0);
        
        bus.tick_peripherals(1);
        CHECK((bus.read_data(0xE4) & 0x10) != 0);
    }
}
