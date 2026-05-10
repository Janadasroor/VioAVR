#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/uart.hpp"
#include "vioavr/core/hex_image.hpp"
#include <filesystem>
#include <string>

namespace {
using namespace vioavr::core;
}

TEST_CASE("ATmega328P Voltmeter Integration Test") {
    // 1. Setup Machine
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);
    machine->reset();

    // 2. Load Firmware
    // The build system should produce voltmeter.hex
    std::string hex_path = "tests/voltmeter.hex"; 
    if (!std::filesystem::exists(hex_path)) {
        // Fallback to absolute if needed, but build/tests/voltmeter.hex is expected
        hex_path = "build/tests/voltmeter.hex";
    }
    
    if (!std::filesystem::exists(hex_path)) {
        FAIL("voltmeter.hex not found. Run make in build directory first.");
    }

    auto image = HexImageLoader::load_file(hex_path, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    // 3. Get Peripherals
    auto adcs = machine->peripherals_of_type<Adc>();
    auto uarts = machine->peripherals_of_type<Uart>();
    REQUIRE(!adcs.empty());
    REQUIRE(!uarts.empty());
    
    auto* adc = adcs[0];
    auto* uart = uarts[0];

    // 4. Test Case 1: 2.5V Input (Mid-scale)
    // 2.5V / 5.0V = 0.5
    adc->set_channel_voltage(0, 0.5);
    
    // Clear any previous output
    u8 dummy;
    while (uart->consume_transmitted_byte(dummy));

    // Run enough cycles to complete conversion and UART transmission
    // 1ms per loop in firmware + UART delay.
    machine->run(1000000);

    // Collect UART output
    std::string output;
    u8 data;
    while (uart->consume_transmitted_byte(data)) {
        output += static_cast<char>(data);
    }
    
    // Verify output contains the expected voltage string
    // ADC 512 -> millivolts ~2500 -> "V: 2.500"
    CHECK(output.find("V: 2.500") != std::string::npos);

    // 5. Test Case 2: 0V Input
    adc->set_channel_voltage(0, 0.0);
    machine->run(1000000);
    
    output.clear();
    while (uart->consume_transmitted_byte(data)) {
        output += static_cast<char>(data);
    }
    CHECK(output.find("V: 0.000") != std::string::npos);
}
