#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/ac8x.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;

static constexpr u16 AC_CTRLA = 0x680;
static constexpr u16 AC_MUXCTRLA = 0x682;
static constexpr u16 AC_DACCTRLA = 0x684;
static constexpr u16 AC_INTCTRL = 0x686;
static constexpr u16 AC_STATUS = 0x687;

TEST_CASE("Ac8x — reset defaults via machine") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    auto acs = machine.peripherals_of_type<Ac8x>();
    REQUIRE(acs.size() == 1);
    auto* ac = acs.front();

    CHECK(bus.read_data(AC_CTRLA) == 0x00U);
    CHECK(bus.read_data(AC_MUXCTRLA) == 0x00U);
    CHECK(bus.read_data(AC_DACCTRLA) == 0xFFU);
    CHECK(bus.read_data(AC_INTCTRL) == 0x00U);
    CHECK(bus.read_data(AC_STATUS) == 0x00U);
    CHECK_FALSE(ac->get_state());
}

TEST_CASE("Ac8x — register round-trips via machine") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();

    bus.write_data(AC_CTRLA, 0x8FU);
    CHECK(bus.read_data(AC_CTRLA) == 0x8FU);

    bus.write_data(AC_MUXCTRLA, 0x97U);
    CHECK(bus.read_data(AC_MUXCTRLA) == 0x97U);

    bus.write_data(AC_DACCTRLA, 0x80U);
    CHECK(bus.read_data(AC_DACCTRLA) == 0x80U);

    bus.write_data(AC_INTCTRL, 0x01U);
    CHECK(bus.read_data(AC_INTCTRL) == 0x01U);
}

TEST_CASE("Ac8x — STATUS STATE bit reflects comparison") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    auto& bank = machine.analog_signal_bank();

    bank.set_voltage(0, 2.0);
    bank.set_voltage(4, 1.0);
    bus.write_data(AC_CTRLA, 0x01U);
    bus.tick_peripherals(1);
    CHECK((bus.read_data(AC_STATUS) & 0x10U) != 0U);

    bank.set_voltage(0, 0.5);
    bus.tick_peripherals(1);
    CHECK((bus.read_data(AC_STATUS) & 0x10U) == 0U);
}

TEST_CASE("Ac8x — STATUS CMP write-1-to-clear") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    auto& bank = machine.analog_signal_bank();

    bank.set_voltage(0, 0.0);
    bank.set_voltage(4, 0.5);
    bus.write_data(AC_CTRLA, 0x01U);
    bus.tick_peripherals(1);

    bank.set_voltage(0, 1.0);
    bus.tick_peripherals(1);
    CHECK((bus.read_data(AC_STATUS) & 0x01U) != 0U);

    bus.write_data(AC_STATUS, 0x01U);
    CHECK((bus.read_data(AC_STATUS) & 0x01U) == 0U);
    CHECK((bus.read_data(AC_STATUS) & 0x10U) != 0U);

    bus.write_data(AC_STATUS, 0x01U);
    CHECK((bus.read_data(AC_STATUS) & 0x01U) == 0U);
}

TEST_CASE("Ac8x — rising edge interrupt INTMODE=1") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    auto& bank = machine.analog_signal_bank();
    auto acs = machine.peripherals_of_type<Ac8x>();
    auto* ac = acs.front();

    bank.set_voltage(0, 0.0);
    bank.set_voltage(4, 1.0);
    bus.write_data(AC_CTRLA, 0x11U);
    bus.write_data(AC_INTCTRL, 0x01U);
    bus.tick_peripherals(1);
    CHECK_FALSE(ac->get_state());
    CHECK((bus.read_data(AC_STATUS) & 0x01U) == 0U);

    bank.set_voltage(0, 2.0);
    bus.tick_peripherals(1);
    CHECK(ac->get_state());
    CHECK((bus.read_data(AC_STATUS) & 0x01U) != 0U);

    bus.write_data(AC_STATUS, 0x01U);
    bank.set_voltage(0, 0.0);
    bus.tick_peripherals(1);
    CHECK_FALSE(ac->get_state());
    CHECK((bus.read_data(AC_STATUS) & 0x01U) == 0U);
}

TEST_CASE("Ac8x — falling edge interrupt INTMODE=2") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    auto& bank = machine.analog_signal_bank();
    auto acs = machine.peripherals_of_type<Ac8x>();
    auto* ac = acs.front();

    bank.set_voltage(0, 2.0);
    bank.set_voltage(4, 1.0);
    bus.write_data(AC_CTRLA, 0x21U);
    bus.tick_peripherals(1);
    CHECK(ac->get_state());
    CHECK((bus.read_data(AC_STATUS) & 0x01U) == 0U);

    bank.set_voltage(0, 0.0);
    bus.tick_peripherals(1);
    CHECK_FALSE(ac->get_state());
    CHECK((bus.read_data(AC_STATUS) & 0x01U) != 0U);

    bus.write_data(AC_STATUS, 0x01U);
    bank.set_voltage(0, 2.0);
    bus.tick_peripherals(1);
    CHECK(ac->get_state());
    CHECK((bus.read_data(AC_STATUS) & 0x01U) == 0U);
}

TEST_CASE("Ac8x — toggle interrupt INTMODE=3") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    auto& bank = machine.analog_signal_bank();

    bank.set_voltage(0, 1.0);
    bank.set_voltage(4, 0.5);
    bus.write_data(AC_CTRLA, 0x31U);
    bus.tick_peripherals(1);
    CHECK((bus.read_data(AC_STATUS) & 0x01U) != 0U);

    bus.write_data(AC_STATUS, 0x01U);
    bank.set_voltage(0, 0.0);
    bus.tick_peripherals(1);
    CHECK((bus.read_data(AC_STATUS) & 0x01U) != 0U);

    bus.write_data(AC_STATUS, 0x01U);
    bank.set_voltage(0, 1.0);
    bus.tick_peripherals(1);
    CHECK((bus.read_data(AC_STATUS) & 0x01U) != 0U);
}

TEST_CASE("Ac8x — interrupt pending with and without CMPIE") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    auto& bank = machine.analog_signal_bank();
    auto acs = machine.peripherals_of_type<Ac8x>();
    auto* ac = acs.front();

    InterruptRequest req{};

    bank.set_voltage(0, 0.0);
    bank.set_voltage(4, 0.5);
    bus.write_data(AC_CTRLA, 0x01U);
    bus.write_data(AC_INTCTRL, 0x00U);
    bus.tick_peripherals(1);

    bank.set_voltage(0, 1.0);
    bus.tick_peripherals(1);
    CHECK((bus.read_data(AC_STATUS) & 0x01U) != 0U);
    CHECK_FALSE(ac->pending_interrupt_request(req));
    CHECK_FALSE(ac->consume_interrupt_request(req));

    bus.write_data(AC_STATUS, 0x01U);
    bus.write_data(AC_INTCTRL, 0x01U);

    bank.set_voltage(0, 0.0);
    bus.tick_peripherals(1);
    CHECK(ac->pending_interrupt_request(req));
    CHECK(req.vector_index == 21);
}

TEST_CASE("Ac8x — hysteresis 25mV (HYS=2)") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    auto& bank = machine.analog_signal_bank();
    auto acs = machine.peripherals_of_type<Ac8x>();
    auto* ac = acs.front();

    bank.set_voltage(0, 0.0);
    bank.set_voltage(4, 0.5);
    bus.write_data(AC_CTRLA, 0x05U);
    bus.tick_peripherals(1);
    CHECK_FALSE(ac->get_state());

    bank.set_voltage(0, 0.47);
    bus.tick_peripherals(1);
    CHECK_FALSE(ac->get_state());

    bank.set_voltage(0, 0.53);
    bus.tick_peripherals(1);
    CHECK(ac->get_state());

    bank.set_voltage(0, 0.49);
    bus.tick_peripherals(1);
    CHECK(ac->get_state());

    bank.set_voltage(0, 0.46);
    bus.tick_peripherals(1);
    CHECK_FALSE(ac->get_state());
}

TEST_CASE("Ac8x — DACREF as negative input MUXNEG=3") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    auto& bank = machine.analog_signal_bank();
    auto acs = machine.peripherals_of_type<Ac8x>();
    auto* ac = acs.front();

    bank.set_voltage(0, 1.0);
    bus.write_data(AC_MUXCTRLA, 0x03U);
    bus.write_data(AC_DACCTRLA, 0x80U);
    bus.write_data(AC_CTRLA, 0x01U);
    bus.tick_peripherals(1);
    CHECK_FALSE(ac->get_state());

    bank.set_voltage(0, 3.0);
    bus.tick_peripherals(1);
    CHECK(ac->get_state());

    bank.set_voltage(0, 2.0);
    bus.tick_peripherals(1);
    CHECK_FALSE(ac->get_state());
}

TEST_CASE("Ac8x — enable disables toggle") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    auto& bank = machine.analog_signal_bank();
    auto acs = machine.peripherals_of_type<Ac8x>();
    auto* ac = acs.front();

    bank.set_voltage(0, 1.0);
    bank.set_voltage(4, 0.5);
    bus.write_data(AC_CTRLA, 0x01U);
    bus.tick_peripherals(1);
    CHECK(ac->get_state());

    bus.write_data(AC_CTRLA, 0x00U);
    bus.tick_peripherals(1);
    CHECK(ac->get_state());

    bank.set_voltage(0, 0.0);
    bus.tick_peripherals(1);
    CHECK(ac->get_state());

    bus.write_data(AC_CTRLA, 0x01U);
    bus.tick_peripherals(1);
    CHECK_FALSE(ac->get_state());
}

TEST_CASE("Ac8x — comparison with different input voltages") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    auto& bank = machine.analog_signal_bank();
    auto acs = machine.peripherals_of_type<Ac8x>();
    auto* ac = acs.front();

    bus.write_data(AC_CTRLA, 0x01U);

    bank.set_voltage(0, 5.0);
    bank.set_voltage(4, 0.0);
    bus.tick_peripherals(1);
    CHECK(ac->get_state());

    bank.set_voltage(0, 0.001);
    bank.set_voltage(4, 4.999);
    bus.tick_peripherals(1);
    CHECK_FALSE(ac->get_state());

    bank.set_voltage(0, 2.5);
    bank.set_voltage(4, 2.5);
    bus.tick_peripherals(1);
    CHECK_FALSE(ac->get_state());

    bank.set_voltage(0, 2.500001);
    bank.set_voltage(4, 2.5);
    bus.tick_peripherals(1);
    CHECK(ac->get_state());
}

TEST_CASE("Ac8x — unmapped addresses return 0") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();

    CHECK(bus.read_data(AC_CTRLA - 1) == 0x00U);
    CHECK(bus.read_data(AC_STATUS + 1) == 0x00U);
    CHECK(bus.read_data(0x0688) == 0x00U);
    CHECK(bus.read_data(0x0689) == 0x00U);
}

TEST_CASE("Ac8x — consume interrupt request") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    auto& bank = machine.analog_signal_bank();
    auto acs = machine.peripherals_of_type<Ac8x>();
    auto* ac = acs.front();

    bank.set_voltage(0, 0.0);
    bank.set_voltage(4, 0.5);
    bus.write_data(AC_CTRLA, 0x01U);
    bus.write_data(AC_INTCTRL, 0x01U);
    bus.tick_peripherals(1);

    bank.set_voltage(0, 1.0);
    bus.tick_peripherals(1);

    InterruptRequest req{};
    CHECK(ac->pending_interrupt_request(req));
    CHECK(req.vector_index == 21);

    CHECK(ac->consume_interrupt_request(req));
    CHECK_FALSE(ac->pending_interrupt_request(req));
    CHECK((bus.read_data(AC_STATUS) & 0x01U) == 0U);
}

TEST_CASE("Ac8x — multiple comparisons in sequence") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    auto& bank = machine.analog_signal_bank();
    auto acs = machine.peripherals_of_type<Ac8x>();
    auto* ac = acs.front();

    bus.write_data(AC_CTRLA, 0x01U);

    bank.set_voltage(0, 1.5);
    bank.set_voltage(4, 1.0);
    bus.tick_peripherals(1);
    CHECK(ac->get_state());
    CHECK((bus.read_data(AC_STATUS) & 0x01U) != 0U);

    bus.write_data(AC_STATUS, 0x01U);

    bank.set_voltage(0, 0.5);
    bus.tick_peripherals(1);
    CHECK_FALSE(ac->get_state());
    CHECK((bus.read_data(AC_STATUS) & 0x01U) != 0U);

    bus.write_data(AC_STATUS, 0x01U);

    bank.set_voltage(0, 1.5);
    bus.tick_peripherals(1);
    CHECK(ac->get_state());
    CHECK((bus.read_data(AC_STATUS) & 0x01U) != 0U);

    bus.write_data(AC_STATUS, 0x01U);

    bank.set_voltage(0, 0.5);
    bus.tick_peripherals(1);
    CHECK_FALSE(ac->get_state());
    CHECK((bus.read_data(AC_STATUS) & 0x01U) != 0U);
}
