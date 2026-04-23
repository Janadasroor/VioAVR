#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/uart.hpp"
#include "vioavr/core/uart8x.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/device_catalog.hpp"
#include <string>
#include <vector>

using namespace vioavr::core;

TEST_CASE("Legacy UART - Stress Test (Interrupt-driven Echo)") {
    auto machine_ptr = Machine::create_for_device("atmega328p");
    REQUIRE(machine_ptr != nullptr);
    auto& machine = *machine_ptr;

    // Load the compiled hex file
    auto image = HexImageLoader::load_file("/home/jnd/cpp_projects/VioAVR/tests/uart_stress_test.hex", machine.bus().device());
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

TEST_CASE("Modern USART (AVR8X) - Stress Test (Interrupt-driven Echo)") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    
    // Construct a minimal AVR8X firmware to say "OK" and echo +1
    // We'll use hex bytes for robustness
    std::vector<u16> flash(32768, 0x0000); // 32K words (64KB)
    
    // Vector table (4809 vectors are 2 words each)
    // 0x00: Reset -> 0x100
    flash[0x00] = 0x940C; flash[0x01] = 0x0100; // JMP 0x100
    // USART0_RXC is vector 17. Word address is 17*2 = 34 (0x22)
    flash[0x22] = 0x940C; flash[0x23] = 0x0200; // JMP 0x200
    
    // --- MAIN ---
    int pc = 0x100;
    // Set Baud Rate (6667 @ 16MHz)
    flash[pc++] = 0xE083; // LDI R24, 0xAB (6667 & 0xFF)
    flash[pc++] = 0xE19A; // LDI R25, 0x1A (6667 >> 8)
    flash[pc++] = 0x9380; flash[pc++] = 0x0808; // STS 0x0808, R24 (BAUDL)
    flash[pc++] = 0x9390; flash[pc++] = 0x0809; // STS 0x0809, R25 (BAUDH)
    
    // Enable TX + RX (0xC0)
    flash[pc++] = 0xEC80; // LDI R24, 0xC0
    flash[pc++] = 0x9380; flash[pc++] = 0x0806; // STS 0x0806, R24 (CTRLB)
    
    // Enable RXCIE
    flash[pc++] = 0xE880; // LDI R24, 0x80 (RXCIE)
    flash[pc++] = 0x9380; flash[pc++] = 0x0805; // STS 0x0805, R24 (CTRLA)
    
    flash[pc++] = 0x9478; // SEI
    
    // Send "OK\n"
    const char* ok = "OK\n";
    // Initialize SP
    flash[pc++] = 0xE08F; // LDI R24, 0xFF
    flash[pc++] = 0xBF8D; // OUT 0x3D, R24 (SPL)
    flash[pc++] = 0xE28F; // LDI R24, 0x2F
    flash[pc++] = 0xBF8E; // OUT 0x3E, R24 (SPH)

    for (int i=0; ok[i]; ++i) {
        // wait DREIF
        flash[pc++] = 0x9180; flash[pc++] = 0x0804; // LDS R24, 0x0804 (STATUS)
        flash[pc++] = 0xFF85; // SBRS R24, 5
        flash[pc++] = 0xCFFC; // RJMP -4
        
        flash[pc++] = 0xE080 | ((ok[i] & 0xF0) << 4) | (ok[i] & 0x0F); // LDI R24, ok[i]
        flash[pc++] = 0x9380; flash[pc++] = 0x0802; // STS 0x0802, R24 (TXDATA)
    }
    
    flash[pc++] = 0xCFFF; // RJMP . (Main Loop)
    
    // --- ISR ---
    pc = 0x200;
    flash[pc++] = 0x9180; flash[pc++] = 0x0800; // LDS R24, 0x0800 (RXDATA)
    flash[pc++] = 0x9583; // INC R24
    // wait DREIF
    flash[pc++] = 0x9190; flash[pc++] = 0x0804; // LDS R25, 0x0804 (STATUS)
    flash[pc++] = 0xFF95; // SBRS R25, 5
    flash[pc++] = 0xCFFC; // RJMP -4
    flash[pc++] = 0x9380; flash[pc++] = 0x0802; // STS 0x0802, R24 (TXDATA)
    flash[pc++] = 0x9518; // RETI
    
    bus.load_flash(flash);
    
    // Park RX pin at High
    machine.pin_mux().register_port(0x408, 0); // PORTA
    machine.pin_mux().update_pin_by_address(0x408, 1, PinOwner::gpio, false, true, true);
    
    machine.reset();
    
    // Explicitly clear queue in case earlier cycles populated it
    auto uart_list = machine.peripherals_of_type<Uart8x>();
    REQUIRE(!uart_list.empty());
    auto* uart = uart_list[0];
    u16 dummy;
    while(uart->consume_transmitted_byte(dummy));
    
    std::string received;
    for (int i=0; i < 200000; ++i) {
        machine.step();
        u16 rdata;
        while (uart->consume_transmitted_byte(rdata)) {
            received += static_cast<char>(rdata);
        }
        if (received.find("OK\n") != std::string::npos) break;
    }
    CHECK(received == "OK\n");
    
    // Inject "ABC"
    std::string echo_out;
    for (char c : std::string("ABC")) {
        uart->inject_received_byte(static_cast<u8>(c));
        bool got_this_echo = false;
        for (int i = 0; i < 100000; ++i) {
            machine.step();
            u16 rdata;
            while (uart->consume_transmitted_byte(rdata)) {
                echo_out += static_cast<char>(rdata);
                got_this_echo = true;
            }
            if (got_this_echo) break;
        }
    }
    CHECK(echo_out == "BCD");
    
    printf("[DEBUG] Final Modern Echo Out: '%s'\n", echo_out.c_str());
    CHECK(echo_out == "BCD");
}
