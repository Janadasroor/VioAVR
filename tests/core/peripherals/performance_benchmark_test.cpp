#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/uart.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/logger.hpp"
#include <string>
#include <vector>
#include <chrono>
#include <filesystem>
#include <cstdio>

using namespace vioavr::core;

static std::string find_hex(const std::string& name) {
    std::string paths[] = {
        "build/tests/" + name + ".hex",
        "../" + name + ".hex",
        "../../build/tests/" + name + ".hex",
        "../../tests/" + name + ".hex",
    };
    for (auto& p : paths) {
        if (std::filesystem::exists(p)) return p;
    }
    return "";
}

TEST_CASE("Performance: pure CPU benchmark (100M cycles)") {
    Logger::set_callback([](LogLevel, std::string_view) {});

    std::string hex = find_hex("benchmark");
    REQUIRE(!hex.empty());

    auto machine_ptr = Machine::create_for_device("atmega328p");
    REQUIRE(machine_ptr != nullptr);
    auto& machine = *machine_ptr;

    auto image = HexImageLoader::load_file(hex, machine.bus().device());
    machine.bus().load_image(image);
    machine.reset();

    const u64 cycle_limit = 100000000;
    auto start = std::chrono::high_resolution_clock::now();
    machine.cpu().run(cycle_limit);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double seconds = duration_us.count() / 1000000.0;
    double mhz = (cycle_limit / seconds) / 1000000.0;

    std::printf("[BENCH] CPU benchmark: 100M cycles in %.2f s = %.1f MHz\n", seconds, mhz);

    CHECK(mhz >= 6.0);
}

TEST_CASE("Performance: integration stress with peripherals (20M cycles)") {
    Logger::set_callback([](LogLevel, std::string_view) {});

    std::string hex = find_hex("integration_stress");
    REQUIRE(!hex.empty());

    auto machine_ptr = Machine::create_for_device("atmega328p");
    REQUIRE(machine_ptr != nullptr);
    auto& machine = *machine_ptr;

    auto image = HexImageLoader::load_file(hex, machine.bus().device());
    machine.bus().load_image(image);
    machine.reset();

    auto uarts = machine.peripherals_of_type<Uart>();
    auto adcs = machine.peripherals_of_type<Adc>();
    REQUIRE(!uarts.empty());
    REQUIRE(!adcs.empty());

    auto* uart = uarts[0];
    auto* adc = adcs[0];

    std::string output;
    const u64 max_cycles = 20000000;

    adc->set_channel_voltage(0, 1.25);

    u32 pwm_high_counts = 0;
    u32 pwm_low_counts = 0;
    bool adc_updated = false;

    auto start = std::chrono::high_resolution_clock::now();

    u64 last_cycles = machine.cpu().cycles();
    while (machine.cpu().cycles() < max_cycles &&
           output.find("INTEGRATION-DONE-V2") == std::string::npos &&
           machine.cpu().state() == CpuState::running) {
        const u64 chunk_target = last_cycles + 1000;
        while (machine.cpu().cycles() < chunk_target && machine.cpu().state() == CpuState::running) {
            machine.step();
            u64 now = machine.cpu().cycles();
            u32 d = static_cast<u32>(now - last_cycles);
            if (machine.pin_mux().get_state(1, 1).drive_level) {
                pwm_high_counts += d;
            } else {
                pwm_low_counts += d;
            }
            last_cycles = now;
        }

        u16 data;
        while (uart->consume_transmitted_byte(data)) {
            output += static_cast<char>(data);
        }

        if (last_cycles >= 8000000 && !adc_updated) {
            adc->set_channel_voltage(0, 3.75);
            adc_updated = true;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double seconds = duration_us.count() / 1000000.0;

    u64 actual_cycles = machine.cpu().cycles();
    double mhz = (actual_cycles / seconds) / 1000000.0;
    std::printf("[BENCH] Integration stress: %llu cycles in %.2f s = %.1f MHz\n",
                (unsigned long long)actual_cycles, seconds, mhz);

    std::printf("[BENCH] Integration Output:\n%s\n", output.c_str());
    std::printf("[BENCH] PWM PB1 High: %u, Low: %u\n", pwm_high_counts, pwm_low_counts);

    CHECK(output.find("INTEGRATION-START") != std::string::npos);
    CHECK(output.find("E:1447644984") != std::string::npos);
    CHECK(output.find("T:100 A:256") != std::string::npos);
    CHECK(output.find("T:500 A:768") != std::string::npos);
    CHECK(output.find("INTEGRATION-DONE") != std::string::npos);

    double duty = (double)pwm_high_counts / (pwm_high_counts + pwm_low_counts);
    CHECK(duty >= 0.45);
    CHECK(duty <= 0.55);

    CHECK(mhz >= 4.0);
}
