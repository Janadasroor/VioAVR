#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/lcd_controller.hpp"

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
        
        bus.write_data(0xE6, 0x77);
        CHECK(bus.read_data(0xE6) == 0x77);

        bus.write_data(0xE7, 0x0F);
        CHECK(bus.read_data(0xE7) == 0x0F);
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
    desc.lcds[0].vector_index = 22; // LCD Start of Frame

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
