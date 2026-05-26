#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/adc8x.hpp"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include <vector>

using namespace vioavr::core;

static constexpr u16 CTRLA    = 0x600;
static constexpr u16 CTRLB    = 0x601;
static constexpr u16 CTRLC    = 0x602;
static constexpr u16 CTRLD    = 0x603;
static constexpr u16 CTRLE    = 0x604;
static constexpr u16 SAMPCTRL = 0x605;
static constexpr u16 MUXPOS   = 0x606;
static constexpr u16 MUXNEG   = 0x607;
static constexpr u16 COMMAND  = 0x608;
static constexpr u16 EVCTRL   = 0x609;
static constexpr u16 INTCTRL  = 0x60A;
static constexpr u16 INTFLAGS = 0x60B;
static constexpr u16 DBGCTRL  = 0x60C;
static constexpr u16 TEMP     = 0x60D;
static constexpr u16 RES      = 0x610;
static constexpr u16 WINLT    = 0x612;
static constexpr u16 WINHT    = 0x614;

TEST_CASE("ADC8X — Reset defaults") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    CHECK(bus.read_data(CTRLA) == 0x00);
    CHECK(bus.read_data(CTRLB) == 0x00);
    CHECK(bus.read_data(CTRLC) == 0x00);
    CHECK(bus.read_data(CTRLD) == 0x00);
    CHECK(bus.read_data(CTRLE) == 0x00);
    CHECK(bus.read_data(SAMPCTRL) == 0x00);
    CHECK(bus.read_data(MUXPOS) == 0x00);
    CHECK(bus.read_data(COMMAND) == 0x00);
    CHECK(bus.read_data(EVCTRL) == 0x00);
    CHECK(bus.read_data(INTCTRL) == 0x00);
    CHECK(bus.read_data(INTFLAGS) == 0x00);
    CHECK(bus.read_data(DBGCTRL) == 0x00);
    CHECK(bus.read_data(RES) == 0x00);
    CHECK(bus.read_data(RES + 1) == 0x00);
    CHECK(bus.read_data(WINLT) == 0x00);
    CHECK(bus.read_data(WINLT + 1) == 0x00);
    CHECK(bus.read_data(WINHT) == 0x00);
    CHECK(bus.read_data(WINHT + 1) == 0x00);
}

TEST_CASE("ADC8X — Register round-trips") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    bus.write_data(CTRLA, 0x85);
    CHECK(bus.read_data(CTRLA) == 0x85);

    bus.write_data(CTRLB, 0x47);
    CHECK(bus.read_data(CTRLB) == 0x47);

    bus.write_data(CTRLC, 0x37);
    CHECK(bus.read_data(CTRLC) == 0x37);

    bus.write_data(CTRLD, 0x0A);
    CHECK(bus.read_data(CTRLD) == 0x0A);

    bus.write_data(CTRLE, 0x06);
    CHECK(bus.read_data(CTRLE) == 0x06);

    bus.write_data(SAMPCTRL, 0x1F);
    CHECK(bus.read_data(SAMPCTRL) == 0x1F);

    bus.write_data(MUXPOS, 0x0F);
    CHECK(bus.read_data(MUXPOS) == 0x0F);

    bus.write_data(DBGCTRL, 0xAB);
    CHECK(bus.read_data(DBGCTRL) == 0xAB);

    bus.write_data(EVCTRL, 0x55);
    CHECK(bus.read_data(EVCTRL) == 0x55);

    bus.write_data(INTCTRL, 0x03);
    CHECK(bus.read_data(INTCTRL) == 0x03);
}

TEST_CASE("ADC8X — Disabled does not convert") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    AnalogSignalBank bank;
    auto adcs = machine.peripherals_of_type<Adc8x>();
    adcs[0]->set_analog_signal_bank(&bank);
    bank.set_voltage(0, 5.0);

    std::vector<u16> prog(1000, 0x0000);
    bus.load_flash(prog);
    machine.cpu().reset();

    bus.write_data(COMMAND, 0x01); // STCONV without ENABLE
    machine.run(100);

    CHECK((bus.read_data(INTFLAGS) & 0x01) == 0);
    CHECK(bus.read_data(RES) == 0);
}

TEST_CASE("ADC8X — Basic conversion") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    AnalogSignalBank bank;
    auto adcs = machine.peripherals_of_type<Adc8x>();
    adcs[0]->set_analog_signal_bank(&bank);
    bank.set_voltage(0, 5.0);

    std::vector<u16> prog(1000, 0x0000);
    bus.load_flash(prog);
    machine.cpu().reset();

    bus.write_data(CTRLC, 0x01); // VREFSEL=VDD, PRESC=DIV4
    bus.write_data(MUXPOS, 0x00);
    bus.write_data(CTRLA, 0x01); // ENABLE

    bus.write_data(COMMAND, 0x01); // STCONV
    machine.run(200);

    CHECK((bus.read_data(INTFLAGS) & 0x01) != 0);
    u8 lo = bus.read_data(RES);
    u8 hi = bus.read_data(RES + 1);
    u16 result = (static_cast<u16>(hi) << 8) | lo;
    CHECK(result > 0);
}

TEST_CASE("ADC8X — INTFLAGS write-1-to-clear") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    AnalogSignalBank bank;
    auto adcs = machine.peripherals_of_type<Adc8x>();
    adcs[0]->set_analog_signal_bank(&bank);
    bank.set_voltage(0, 3.3);

    std::vector<u16> prog(1000, 0x0000);
    bus.load_flash(prog);
    machine.cpu().reset();

    bus.write_data(CTRLC, 0x11); // VREFSEL=VDD, PRESC=DIV4
    bus.write_data(CTRLA, 0x01);
    bus.write_data(COMMAND, 0x01);
    machine.run(200);

    CHECK((bus.read_data(INTFLAGS) & 0x01) != 0);
    bus.write_data(INTFLAGS, 0x01);
    CHECK(bus.read_data(INTFLAGS) == 0);
}

TEST_CASE("ADC8X — 8-bit mode") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    AnalogSignalBank bank;
    auto adcs = machine.peripherals_of_type<Adc8x>();
    adcs[0]->set_analog_signal_bank(&bank);
    bank.set_voltage(0, 3.3);

    std::vector<u16> prog(1000, 0x0000);
    bus.load_flash(prog);
    machine.cpu().reset();

    bus.write_data(CTRLC, 0x11); // VDD, DIV4
    bus.write_data(CTRLA, 0x05); // ENABLE + RESSEL=1 (8-bit)
    bus.write_data(COMMAND, 0x01);
    machine.run(200);

    CHECK((bus.read_data(INTFLAGS) & 0x01) != 0);
    u16 res = bus.read_data(RES) | (static_cast<u16>(bus.read_data(RES + 1)) << 8);
    CHECK(res <= 0xFF);
}

TEST_CASE("ADC8X — Accumulation mode") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    AnalogSignalBank bank;
    auto adcs = machine.peripherals_of_type<Adc8x>();
    adcs[0]->set_analog_signal_bank(&bank);
    bank.set_voltage(0, 2.5);

    std::vector<u16> prog(1000, 0x0000);
    bus.load_flash(prog);
    machine.cpu().reset();

    bus.write_data(CTRLC, 0x11); // VDD, DIV4
    bus.write_data(CTRLB, 0x01); // SAMPNUM=1 (2 samples)
    bus.write_data(CTRLA, 0x01); // ENABLE
    bus.write_data(COMMAND, 0x01);
    machine.run(500);

    CHECK((bus.read_data(INTFLAGS) & 0x01) != 0);
}

TEST_CASE("ADC8X — Window comparator BELOW") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    AnalogSignalBank bank;
    auto adcs = machine.peripherals_of_type<Adc8x>();
    adcs[0]->set_analog_signal_bank(&bank);
    bank.set_voltage(0, 0.5);

    std::vector<u16> prog(1000, 0x0000);
    bus.load_flash(prog);
    machine.cpu().reset();

    bus.write_data(CTRLC, 0x11); // VDD, DIV4
    bus.write_data(WINLT, 0xC8); // LOW threshold (200, result ~155 < 200)
    bus.write_data(WINHT, 0xFF);
    bus.write_data(CTRLE, 0x01); // WINCM=BELOW
    bus.write_data(CTRLA, 0x01);
    bus.write_data(COMMAND, 0x01);
    machine.run(200);

    CHECK((bus.read_data(INTFLAGS) & 0x03) == 0x03);
}

TEST_CASE("ADC8X — Window comparator ABOVE") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    AnalogSignalBank bank;
    auto adcs = machine.peripherals_of_type<Adc8x>();
    adcs[0]->set_analog_signal_bank(&bank);
    bank.set_voltage(0, 4.5);

    std::vector<u16> prog(1000, 0x0000);
    bus.load_flash(prog);
    machine.cpu().reset();

    bus.write_data(CTRLC, 0x11);
    bus.write_data(WINLT, 0x00);
    bus.write_data(WINHT, 0x80); // HIGH threshold (2.58V)
    bus.write_data(CTRLE, 0x02); // WINCM=ABOVE
    bus.write_data(CTRLA, 0x01);
    bus.write_data(COMMAND, 0x01);
    machine.run(200);

    CHECK((bus.read_data(INTFLAGS) & 0x03) == 0x03);
}

TEST_CASE("ADC8X — Window comparator INSIDE") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    AnalogSignalBank bank;
    auto adcs = machine.peripherals_of_type<Adc8x>();
    adcs[0]->set_analog_signal_bank(&bank);
    bank.set_voltage(0, 2.5);

    std::vector<u16> prog(1000, 0x0000);
    bus.load_flash(prog);
    machine.cpu().reset();

    bus.write_data(CTRLC, 0x11);
    bus.write_data(WINLT, 0x00); // 0x02BC = 700
    bus.write_data(WINLT + 1, 0x02);
    bus.write_data(WINHT, 0x20); // 0x0320 = 800
    bus.write_data(WINHT + 1, 0x03);
    bus.write_data(CTRLE, 0x03); // WINCM=INSIDE
    bus.write_data(CTRLA, 0x01);
    bus.write_data(COMMAND, 0x01);
    machine.run(200);

    CHECK((bus.read_data(INTFLAGS) & 0x03) == 0x03);
}

TEST_CASE("ADC8X — Window comparator OUTSIDE") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    AnalogSignalBank bank;
    auto adcs = machine.peripherals_of_type<Adc8x>();
    adcs[0]->set_analog_signal_bank(&bank);
    bank.set_voltage(0, 3.0);

    std::vector<u16> prog(1000, 0x0000);
    bus.load_flash(prog);
    machine.cpu().reset();

    bus.write_data(CTRLC, 0x11);
    bus.write_data(WINLT, 0x80); // 1.65V
    bus.write_data(WINHT, 0x90); // 1.86V
    bus.write_data(CTRLE, 0x04); // WINCM=OUTSIDE
    bus.write_data(CTRLA, 0x01);
    bus.write_data(COMMAND, 0x01);
    machine.run(200);

    CHECK((bus.read_data(INTFLAGS) & 0x03) == 0x03);
}

TEST_CASE("ADC8X — WINLT round-trip") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    bus.write_data(WINLT, 0x34);
    bus.write_data(WINLT + 1, 0x12);
    CHECK(bus.read_data(WINLT) == 0x34);
    CHECK(bus.read_data(WINLT + 1) == 0x12);
}

TEST_CASE("ADC8X — WINHT round-trip") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    bus.write_data(WINHT, 0xCD);
    bus.write_data(WINHT + 1, 0xAB);
    CHECK(bus.read_data(WINHT) == 0xCD);
    CHECK(bus.read_data(WINHT + 1) == 0xAB);
}

TEST_CASE("ADC8X — MUXPOS different channel") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    AnalogSignalBank bank;
    auto adcs = machine.peripherals_of_type<Adc8x>();
    adcs[0]->set_analog_signal_bank(&bank);
    bank.set_voltage(0, 0.0);
    bank.set_voltage(1, 3.3);

    std::vector<u16> prog(1000, 0x0000);
    bus.load_flash(prog);
    machine.cpu().reset();

    bus.write_data(CTRLC, 0x11);
    bus.write_data(MUXPOS, 0x01); // AIN1
    bus.write_data(CTRLA, 0x01);
    bus.write_data(COMMAND, 0x01);
    machine.run(200);

    CHECK((bus.read_data(INTFLAGS) & 0x01) != 0);
    u16 res = bus.read_data(RES) | (static_cast<u16>(bus.read_data(RES + 1)) << 8);
    CHECK(res > 900);
}

TEST_CASE("ADC8X — Interrupt enable RESRDY") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    AnalogSignalBank bank;
    auto adcs = machine.peripherals_of_type<Adc8x>();
    adcs[0]->set_analog_signal_bank(&bank);
    bank.set_voltage(0, 3.3);

    std::vector<u16> prog(1000, 0x0000);
    bus.load_flash(prog);
    machine.cpu().reset();

    bus.write_data(CTRLC, 0x11);
    bus.write_data(INTCTRL, 0x01); // RESRDYIE
    bus.write_data(CTRLA, 0x01);
    bus.write_data(COMMAND, 0x01);
    machine.run(200);

    InterruptRequest irq;
    CHECK(bus.pending_interrupt_request(irq) == true);
    CHECK(irq.vector_index == 22);
}

TEST_CASE("ADC8X — Interrupt disabled no pending") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    AnalogSignalBank bank;
    auto adcs = machine.peripherals_of_type<Adc8x>();
    adcs[0]->set_analog_signal_bank(&bank);
    bank.set_voltage(0, 3.3);

    std::vector<u16> prog(1000, 0x0000);
    bus.load_flash(prog);
    machine.cpu().reset();

    bus.write_data(CTRLC, 0x11);
    bus.write_data(INTCTRL, 0x00); // Interrupts disabled
    bus.write_data(CTRLA, 0x01);
    bus.write_data(COMMAND, 0x01);
    machine.run(200);

    CHECK((bus.read_data(INTFLAGS) & 0x01) != 0);
    InterruptRequest irq;
    CHECK(bus.pending_interrupt_request(irq) == false);
}

TEST_CASE("ADC8X — Prescaler affects timing") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    AnalogSignalBank bank;
    auto adcs = machine.peripherals_of_type<Adc8x>();
    adcs[0]->set_analog_signal_bank(&bank);
    bank.set_voltage(0, 3.3);

    std::vector<u16> prog(1000, 0x0000);
    bus.load_flash(prog);
    machine.cpu().reset();

    // DIV128 (prescaler=7 -> 256) = very slow
    bus.write_data(CTRLC, 0x17); // VDD, DIV128
    bus.write_data(CTRLA, 0x01);
    bus.write_data(COMMAND, 0x01);
    machine.run(100);

    // Should NOT be done yet
    CHECK((bus.read_data(INTFLAGS) & 0x01) == 0);
}

TEST_CASE("ADC8X — Unmapped addresses return 0") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    CHECK(bus.read_data(0x060E) == 0);
    CHECK(bus.read_data(0x060F) == 0);
    CHECK(bus.read_data(0x0617) == 0);
}
