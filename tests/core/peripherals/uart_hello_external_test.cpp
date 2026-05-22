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

TEST_CASE("External UART firmware - Hello from ATmega328p") {
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);
    machine->reset();

    std::string hex_path = "tests/uart_hello.hex";
    if (!std::filesystem::exists(hex_path)) {
        hex_path = "../tests/uart_hello.hex";
    }
    if (!std::filesystem::exists(hex_path)) {
        hex_path = "../../tests/uart_hello.hex";
    }
    if (!std::filesystem::exists(hex_path)) {
        hex_path = "../../../tests/uart_hello.hex";
    }
    if (!std::filesystem::exists(hex_path)) {
        hex_path = "build/tests/uart_hello.hex";
    }
    if (!std::filesystem::exists(hex_path)) {
        FAIL("uart_hello.hex not found");
    }

    auto image = HexImageLoader::load_file(hex_path, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    auto uarts = machine->peripherals_of_type<Uart>();
    REQUIRE(!uarts.empty());
    auto* uart = uarts[0];

    // Run enough cycles for the firmware to boot and start sending
    machine->run(5000000);

    std::string out;
    u8 b;
    while (uart->consume_transmitted_byte(b)) {
        out += static_cast<char>(b);
    }

    CHECK(!out.empty());
    CHECK(out.find("Hello") != std::string::npos);
}
