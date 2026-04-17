#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

TEST_CASE("AVR Math Fidelity Test") {
    MemoryBus bus {atmega328p};
    AvrCpu cpu {bus};

    vioavr::core::PinMux pm_portb { 10 }; GpioPort portb { "PORTB", 0x23U, 0x24U, 0x25U, pm_portb };
    vioavr::core::PinMux pm_portc { 10 }; GpioPort portc { "PORTC", 0x26U, 0x27U, 0x28U, pm_portc };
    bus.attach_peripheral(portb);
    bus.attach_peripheral(portc);

    const auto image = HexImageLoader::load_file("../test_math_advanced.hex", atmega328p);
    bus.load_image(image);
    cpu.reset();

    // Helper to run until a specific register value is hit
    auto run_until_portb = [&](u8 value, u64 limit = 10000) {
        u64 count = 0;
        while (portb.read(0x25U) != value && count < limit) {
            cpu.step();
            count++;
        }
        return count < limit;
    };

    // 1. Addition (16-bit) -> 1500 (0x05DC)
    REQUIRE(run_until_portb(0xDCU));
    // Step a few more to catch the PORTC write which usually follows immediately
    for(int i=0; i<10; ++i) { cpu.step(); if(portc.read(0x28U) == 0x05U) break; }
    CHECK(portc.read(0x28U) == 0x05U);

    // 2. Subtraction (16-bit) -> 500 (0x01F4)
    REQUIRE(run_until_portb(0xF4U));
    for(int i=0; i<10; ++i) { cpu.step(); if(portc.read(0x28U) == 0x01U) break; }
    CHECK(portc.read(0x28U) == 0x01U);

    // 3. Multiplication (8-bit) -> 200 (0xC8)
    REQUIRE(run_until_portb(0xC8U));

    // 4. Division (16-bit) -> 2
    REQUIRE(run_until_portb(0x02U));

    // 5. 32-bit Addition -> 150000 (0x000249F0)
    REQUIRE(run_until_portb(0xF0U));
    for(int i=0; i<10; ++i) { cpu.step(); if(portc.read(0x28U) == 0x49U) break; }
    CHECK(portc.read(0x28U) == 0x49U);
}
