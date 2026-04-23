#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/uart.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/hex_image.hpp"
#include <string>
#include <vector>

using namespace vioavr::core;

TEST_CASE("System Integration Stress Test") {
    auto machine_ptr = Machine::create_for_device("atmega328p");
    REQUIRE(machine_ptr != nullptr);
    auto& machine = *machine_ptr;

    // Load integration_stress.hex
    auto image = HexImageLoader::load_file("/home/jnd/cpp_projects/VioAVR/integration_stress.hex", machine.bus().device());
    machine.bus().load_image(image);
    machine.reset(); // Crucial: sets PC to entry point and state to running

    auto uarts = machine.peripherals_of_type<Uart>();
    auto adcs = machine.peripherals_of_type<Adc>();
    REQUIRE(!uarts.empty());
    REQUIRE(!adcs.empty());

    auto* uart = uarts[0];
    auto* adc = adcs[0];

    std::string output;
    u32 total_cycles = 0;
    const u32 max_cycles = 20000000; // 20 million cycles (approx 1.25s real time at 16MHz)

    // Set some ADC values to be picked up by the conversions
    adc->set_channel_voltage(0, 0.25); // ~256
    
    u32 pwm_high_counts = 0;
    u32 pwm_low_counts = 0;

    u64 last_cycles = machine.cpu().cycles();
    bool adc_updated = false;
    while (machine.cpu().cycles() < max_cycles && 
           output.find("INTEGRATION-DONE-V2") == std::string::npos &&
           machine.cpu().state() == CpuState::running) {
        const u64 chunk_target = last_cycles + 1000;
        while (machine.cpu().cycles() < chunk_target && machine.cpu().state() == CpuState::running) {
            machine.step();
            u64 now = machine.cpu().cycles();
            u32 d = static_cast<u32>(now - last_cycles);
            
            if (machine.pin_mux().get_state(0, 1).drive_level) {
                pwm_high_counts += d;
            } else {
                pwm_low_counts += d;
            }
            last_cycles = now;
        }

        // Collect UART output
        u8 data;
        while (uart->consume_transmitted_byte(data)) {
            output += static_cast<char>(data);
        }

        // Change setpoint at 500ms (8,000,000 cycles)
        if (last_cycles >= 8000000 && !adc_updated) {
            adc->set_channel_voltage(0, 0.75); // ~768
            adc_updated = true;
        }
    }

    printf("[DEBUG] Integration Output:\n%s\n", output.c_str());
    printf("[DEBUG] PWM PB1 High: %u, Low: %u\n", pwm_high_counts, pwm_low_counts);

    // Assertions
    CHECK(output.find("INTEGRATION-START") != std::string::npos);
    CHECK(output.find("E:1447644984") != std::string::npos); // 0x56494F38 decimal
    CHECK(output.find("T:100 A:256") != std::string::npos); // Initial ADC value
    CHECK(output.find("T:500 A:768") != std::string::npos); // Updated ADC value (at 500 instead of 1000)
    CHECK(output.find("INTEGRATION-DONE") != std::string::npos);

    // PWM Check: Fast PWM 8-bit with OCR1A=127 should be ~50%
    double duty = (double)pwm_high_counts / (pwm_high_counts + pwm_low_counts);
    CHECK(duty >= 0.45);
    CHECK(duty <= 0.55);
}
