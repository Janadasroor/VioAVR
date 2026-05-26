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

TEST_CASE("ATmega328P System Monitor Integration Test") {
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);
    machine->reset();

    std::string hex_path = "tests/sysmon.hex";
    if (!std::filesystem::exists(hex_path)) {
        hex_path = "../sysmon.hex";
    }
    if (!std::filesystem::exists(hex_path)) {
        hex_path = "build/tests/sysmon.hex";
    }
    if (!std::filesystem::exists(hex_path)) {
        FAIL("sysmon.hex not found. Run 'make sysmon_hex' in build directory first.");
    }

    auto image = HexImageLoader::load_file(hex_path, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    auto adcs = machine->peripherals_of_type<Adc>();
    auto uarts = machine->peripherals_of_type<Uart>();
    REQUIRE(!adcs.empty());
    REQUIRE(!uarts.empty());

    auto* adc = adcs[0];
    auto* uart = uarts[0];

    const u64 CYCLES_PER_BYTE = 16640;

    auto flush_uart = [&]() -> std::string {
        std::string out;
        u16 b;
        while (uart->consume_transmitted_byte(b)) {
            out += static_cast<char>(b);
        }
        return out;
    };

    // Send command byte-by-byte and collect full response
    auto send_and_recv = [&](const std::string& cmd) -> std::string {
        for (char c : cmd) {
            uart->inject_received_byte(static_cast<u8>(c));
            machine->run(2 * CYCLES_PER_BYTE);
        }
        uart->inject_received_byte(static_cast<u8>('\r'));
        machine->run(2 * CYCLES_PER_BYTE);

        std::string out;
        u64 max_cycles = 12000000;
        u64 chunk = 100000;
        u64 elapsed = 0;
        while (elapsed < max_cycles) {
            machine->run(chunk);
            elapsed += chunk;
            u16 b;
            while (uart->consume_transmitted_byte(b)) {
                out += static_cast<char>(b);
            }
            if (out.find("OK> ") != std::string::npos) break;
            if (machine->cpu().state() != CpuState::running) break;
        }
        return out;
    };

    // Boot
    machine->run(2000000);
    std::string boot = flush_uart();
    REQUIRE(!boot.empty());
    CHECK(boot.find("System Monitor") != std::string::npos);
    flush_uart();

    // help
    std::string out = send_and_recv("help");
    CHECK(out.find("help") != std::string::npos);
    CHECK(out.find("adc") != std::string::npos);
    CHECK(out.find("pwm") != std::string::npos);
    CHECK(out.find("blink") != std::string::npos);
    CHECK(out.find("eeprom") != std::string::npos);
    CHECK(out.find("gpio") != std::string::npos);
    CHECK(out.find("info") != std::string::npos);
    CHECK(out.find("echo") != std::string::npos);

    // adc
    adc->set_channel_voltage(0, 2.5);
    out = send_and_recv("adc 0");
    CHECK(out.find("ADC0") != std::string::npos);
    CHECK(out.find("2500 mV") != std::string::npos);

    // echo
    out = send_and_recv("echo hello world");
    CHECK(out.find("hello world") != std::string::npos);

    // unknown command
    out = send_and_recv("bogus");
    CHECK(out.find("ERR: unknown") != std::string::npos);

    // pwm
    out = send_and_recv("pwm 127");
    CHECK(out.find("PWM: 127") != std::string::npos);

    // blink
    out = send_and_recv("blink 500");
    CHECK(out.find("BLINK: 500") != std::string::npos);

    // eeprom write
    out = send_and_recv("eeprom w 10 42");
    CHECK(out.find("EEPROM[10] := 2A") != std::string::npos);

    // eeprom read
    out = send_and_recv("eeprom r 10");
    CHECK(out.find("EEPROM[10] = 2A") != std::string::npos);

    // gpio write via firmware command
    out = send_and_recv("gpio w 0 0 1");
    CHECK(out.find("GPIO OK") != std::string::npos);

    // gpio read back via firmware
    out = send_and_recv("gpio r 0 0");
    CHECK(out.find("PA0: 1") != std::string::npos);  // firmware labels port 0 as 'A' (cosmetic)

    // info
    out = send_and_recv("info");
    CHECK(out.find("ATmega328P") != std::string::npos);
    CHECK(out.find("16 MHz") != std::string::npos);

    // blink 0
    out = send_and_recv("blink 0");
    CHECK(out.find("BLINK: 0") != std::string::npos);
}
