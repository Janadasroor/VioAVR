#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/uart.hpp"
#include "vioavr/core/hex_image.hpp"
#include <filesystem>
#include <string>
#include <cstdio>

namespace { using namespace vioavr::core; }

TEST_CASE("ATmega328P Voltmeter Debug") {
    auto machine = Machine::create_for_device("atmega328p");
    REQUIRE(machine != nullptr);
    machine->reset();

    std::string hex_path = "../tests/voltmeter.hex";
    auto image = HexImageLoader::load_file(hex_path, machine->bus().device());
    machine->bus().load_image(image);
    machine->reset();

    for (int i = 0; i < 500; i++) {
        machine->step();
        if (i < 20 || (machine->bus().cpu().program_counter() < 0x80)) {
            auto pc = machine->bus().cpu().program_counter();
            auto sp = machine->bus().cpu().stack_pointer();
            printf("step %4d: PC=0x%04X SP=0x%04X\n", i, pc, sp);
        }
    }
    printf("Final PC: 0x%04X SP: 0x%04X cycles: %lu\n",
           machine->bus().cpu().program_counter(),
           machine->bus().cpu().stack_pointer(),
           machine->bus().cpu().cycles());
}
