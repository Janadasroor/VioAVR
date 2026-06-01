#include "doctest.h"
#include "vioavr/core/adc8x.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include <vector>

using namespace vioavr::core;

TEST_CASE("AVR8X ADC - Accumulation Mode (SAMPNUM)") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    std::vector<u16> program(1000, 0x0000);
    bus.load_flash(program);
    machine.cpu().reset();

    AnalogSignalBank signal_bank;
    auto adcs = machine.peripherals_of_type<Adc8x>();
    REQUIRE(!adcs.empty());
    adcs[0]->set_analog_signal_bank(&signal_bank);
    signal_bank.set_voltage(0, 3.3);

    // SAMPNUM=4 (16 samples = 1<<4), DIV2, 10-bit, AIN0
    // CTRLB: SAMPNUM at bits 6:4 = 100
    bus.write_data(0x0601, 0x40);
    bus.write_data(0x0602, 0x00); // CTRLC: VREF=VDD, PRESC=DIV2
    bus.write_data(0x0606, 0x00); // MUXPOS: AIN0
    bus.write_data(0x0600, 0x01); // CTRLA: ENABLE=1

    // Start conversion
    bus.write_data(0x0608, 0x01); // COMMAND: STCONV

    // Wait for completion: (2+2+13) * 16 * 2 = 544 cycles
    machine.run(600);
    CHECK((bus.read_data(0x060B) & 0x01) != 0); // RESRDY

    u16 result = bus.read_data(0x0610) | (bus.read_data(0x0611) << 8);
    // With 3.3V / 3.3V VREF, result near 1023
    // SAMPNUM=4: accumulate 16 samples, right-shift by 4
    CHECK_MESSAGE(result > 990, "Accumulated result should be near 1023");
    CHECK_MESSAGE(result <= 1023, "Result capped at 1023");
}

TEST_CASE("AVR8X ADC - Window Comparator Mode ABOVE") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    std::vector<u16> program(1000, 0x0000);
    bus.load_flash(program);
    machine.cpu().reset();

    AnalogSignalBank signal_bank;
    auto adcs = machine.peripherals_of_type<Adc8x>();
    REQUIRE(!adcs.empty());
    adcs[0]->set_analog_signal_bank(&signal_bank);

    // Setup: DIV2, AIN0
    bus.write_data(0x0602, 0x00);
    bus.write_data(0x0606, 0x00);
    bus.write_data(0x0600, 0x01);

    // WINLT = 100, WINHT = 900
    bus.write_data(0x0612, 100 & 0xFF);
    bus.write_data(0x0613, (100 >> 8) & 0x03);
    bus.write_data(0x0614, 900 & 0xFF);
    bus.write_data(0x0615, (900 >> 8) & 0x03);

    // Set voltage above WINHT
    // VREF = VDD (3.3V), voltage = 3.0V -> ADC ≈ 930
    signal_bank.set_voltage(0, 3.0);

    // Enable window comparator: WINCM=ABOVE (0x02: result > WINHT)
    bus.write_data(0x0604, 0x02);

    bus.write_data(0x0608, 0x01);
    machine.run(60);

    u8 intflags = bus.read_data(0x060B);
    CHECK_MESSAGE((intflags & 0x02) != 0, "WCOMP should fire for ABOVE window");
}

TEST_CASE("AVR8X ADC - Window Comparator Mode INSIDE") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    std::vector<u16> program(1000, 0x0000);
    bus.load_flash(program);
    machine.cpu().reset();

    AnalogSignalBank signal_bank;
    auto adcs = machine.peripherals_of_type<Adc8x>();
    REQUIRE(!adcs.empty());
    adcs[0]->set_analog_signal_bank(&signal_bank);

    bus.write_data(0x0602, 0x00);
    bus.write_data(0x0606, 0x00);
    bus.write_data(0x0600, 0x01);

    bus.write_data(0x0612, 100 & 0xFF);
    bus.write_data(0x0613, (100 >> 8) & 0x03);
    bus.write_data(0x0614, 900 & 0xFF);
    bus.write_data(0x0615, (900 >> 8) & 0x03);

    // Voltage inside window: 0.5V -> ADC ≈ 155
    signal_bank.set_voltage(0, 0.5);

    // WINCM=INSIDE (0x03: WINLT ≤ result ≤ WINHT)
    bus.write_data(0x0604, 0x03);

    bus.write_data(0x0608, 0x01);
    machine.run(60);

    u8 intflags = bus.read_data(0x060B);
    CHECK_MESSAGE((intflags & 0x02) != 0, "WCOMP should fire for INSIDE window");
}

TEST_CASE("AVR8X ADC - Window Comparator Mode BELOW") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    std::vector<u16> program(1000, 0x0000);
    bus.load_flash(program);
    machine.cpu().reset();

    AnalogSignalBank signal_bank;
    auto adcs = machine.peripherals_of_type<Adc8x>();
    REQUIRE(!adcs.empty());
    adcs[0]->set_analog_signal_bank(&signal_bank);

    bus.write_data(0x0602, 0x00);
    bus.write_data(0x0606, 0x00);
    bus.write_data(0x0600, 0x01);

    bus.write_data(0x0612, 100 & 0xFF);
    bus.write_data(0x0613, (100 >> 8) & 0x03);
    bus.write_data(0x0614, 900 & 0xFF);
    bus.write_data(0x0615, (900 >> 8) & 0x03);

    // Voltage below window: 0.1V -> ADC ≈ 31
    signal_bank.set_voltage(0, 0.1);

    // WINCM=BELOW (0x01: result < WINLT)
    bus.write_data(0x0604, 0x01);

    bus.write_data(0x0608, 0x01);
    machine.run(60);

    u8 intflags = bus.read_data(0x060B);
    CHECK_MESSAGE((intflags & 0x02) != 0, "WCOMP should fire for BELOW window");
}
