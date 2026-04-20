#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/uart.hpp"
#include "vioavr/core/hex_image.hpp"
#include <string>
#include <vector>

using namespace vioavr::core;

TEST_CASE("Legacy UART - Stress Test (Interrupt-driven Echo)") {
    auto machine_ptr = Machine::create_for_device("atmega328p");
    REQUIRE(machine_ptr != nullptr);
    auto& machine = *machine_ptr;

    // Load the compiled hex file
    auto image = HexImageLoader::load_file("tests/uart_stress_test.hex", machine.bus().device());
    machine.bus().load_image(image);
    machine.reset(); // Crucial for PC and state

    auto peripherals = machine.peripherals_of_type<Uart>();
    REQUIRE(!peripherals.empty());
    auto* uart = peripherals[0];

    std::string received_output;
    u64 last_cycles = machine.cpu().cycles();
    const u64 max_cycles = 10000000;

    // Run until we see "VioAVR-OK\n"
    while (machine.cpu().cycles() < max_cycles && 
           received_output.find("VioAVR-OK\n") == std::string::npos &&
           machine.cpu().state() == CpuState::running) {
        machine.run(1000);
        
        u8 data;
        while (uart->consume_transmitted_byte(data)) {
            received_output += static_cast<char>(data);
        }
    }
    printf("[DEBUG] Startup Output: %s\n", received_output.c_str());
    
    auto history = machine.trace_history();
    printf("[DEBUG] Last 20 instructions:\n");
    for (size_t i = (history.size() > 20 ? history.size() - 20 : 0); i < history.size(); ++i) {
        printf("  0x%04X: Sreg=0x%02X\n", history[i].program_counter, history[i].sreg);
    }
    
    CHECK( received_output.find("VioAVR-OK\n") != std::string::npos );

    // Now send "ABCD" and expect "BCDE"
    std::string echo_input = "ABCD";
    std::string echo_output;
    
    for (char c : echo_input) {
        uart->inject_received_byte(static_cast<u8>(c));
        
        // Wait long enough for processing (multiple rounds of polling)
        const u64 process_target = machine.cpu().cycles() + 1000000; // 1M cycles per char
        while (machine.cpu().cycles() < process_target) {
            machine.run(10000);
            
            u8 data;
            while (uart->consume_transmitted_byte(data)) {
                echo_output += static_cast<char>(data);
            }
        }
    }

    CHECK(echo_output == "BCDE");
}
