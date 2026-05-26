#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/uart.hpp"
#include "vioavr/core/hex_image.hpp"
#include <filesystem>
#include <string>

namespace {
using namespace vioavr::core;
}

TEST_CASE("External UART firmware - Hello from ATmega328p (16MHz)") {
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);
    machine->reset();

    std::string hex_path = "tests/uart_hello_16mhz.hex";
    if (!std::filesystem::exists(hex_path)) {
        hex_path = "../tests/uart_hello_16mhz.hex";
    }
    if (!std::filesystem::exists(hex_path)) {
        hex_path = "../../tests/uart_hello_16mhz.hex";
    }
    if (!std::filesystem::exists(hex_path)) {
        hex_path = "../../../tests/uart_hello_16mhz.hex";
    }
    if (!std::filesystem::exists(hex_path)) {
        hex_path = "build/tests/uart_hello_16mhz.hex";
    }
    if (!std::filesystem::exists(hex_path)) {
        FAIL("uart_hello_16mhz.hex not found");
    }

    auto image = HexImageLoader::load_file(hex_path, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    auto uarts = machine->peripherals_of_type<Uart>();
    REQUIRE(!uarts.empty());
    auto* uart = uarts[0];

    // Boot + UART init + first TX. The delay loop is 1,607,680 cycles per 100ms.
    // At ~21 chars * ~10-12us per char + 100ms delay ≈ 100ms per iteration.
    // Run 1.1M to get past boot/init, expect first "Hello" output.
    machine->run(1100000);

    std::string out;
    u16 b;
    while (uart->consume_transmitted_byte(b)) {
        out += static_cast<char>(b);
    }

    CHECK(!out.empty());
    CHECK(out.find("Hello") != std::string::npos);

    // Verify the delay: run enough to complete delay + next string.
    // First iteration: 23 chars at ~16640 cycles each ≈ 383K cycles for TX,
    // then delay = 1,607,680 cycles. Total first iteration ≈ 1,991,000.
    // Run 2.5M to definitely be into the second string output.
    machine->run(2500000);
    std::string out2;
    while (uart->consume_transmitted_byte(b)) {
        out2 += static_cast<char>(b);
    }
    CHECK(!out2.empty());
    CHECK(out2.find("Hello") != std::string::npos); // second iteration
}
