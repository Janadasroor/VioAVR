#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/cpu_int.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

static constexpr u16 CTRLA    = 0x0110;
static constexpr u16 STATUS   = 0x0111;
static constexpr u16 LVL0PRI  = 0x0112;
static constexpr u16 LVL1VEC  = 0x0113;

TEST_CASE("CPUINT — Reset defaults") {
    DeviceDescriptor dev = atmega4809;
    Machine machine(dev);
    auto& bus = machine.bus();
    machine.reset();

    CHECK(bus.read_data(CTRLA) == 0x00);
    CHECK(bus.read_data(STATUS) == 0x00);
    CHECK(bus.read_data(LVL0PRI) == 0x00);
    CHECK(bus.read_data(LVL1VEC) == 0xFF);
}

TEST_CASE("CPUINT — Register round-trips") {
    DeviceDescriptor dev = atmega4809;
    Machine machine(dev);
    auto& bus = machine.bus();
    machine.reset();

    bus.write_data(CTRLA, 0x61);
    CHECK(bus.read_data(CTRLA) == 0x61);

    bus.write_data(LVL0PRI, 0x50);
    CHECK(bus.read_data(LVL0PRI) == 0x50);

    bus.write_data(LVL1VEC, 0x10);
    CHECK(bus.read_data(LVL1VEC) == 0x10);
}

TEST_CASE("CPUINT — CTRLA mask (only bits 0,5,6 writable)") {
    DeviceDescriptor dev = atmega4809;
    Machine machine(dev);
    auto& bus = machine.bus();
    machine.reset();

    bus.write_data(CTRLA, 0xFF);
    CHECK(bus.read_data(CTRLA) == 0x61);
}

TEST_CASE("CPUINT — STATUS read-only") {
    DeviceDescriptor dev = atmega4809;
    Machine machine(dev);
    auto& bus = machine.bus();
    machine.reset();

    bus.write_data(STATUS, 0xFF);
    CHECK(bus.read_data(STATUS) == 0x00);
}

TEST_CASE("CPUINT — LVL1VEC = 0xFF disables level 1") {
    DeviceDescriptor dev = atmega4809;
    Machine machine(dev);
    auto& bus = machine.bus();
    machine.reset();

    CpuInt* cpuint = machine.peripherals_of_type<CpuInt>()[0];

    bus.write_data(LVL1VEC, 0xFF);
    CHECK(cpuint->is_lvl1_vector(10) == false);
    CHECK(cpuint->is_lvl1_vector(0xFF) == false);
}

TEST_CASE("CPUINT — is_lvl1_vector with valid value") {
    DeviceDescriptor dev = atmega4809;
    Machine machine(dev);
    auto& bus = machine.bus();
    machine.reset();

    CpuInt* cpuint = machine.peripherals_of_type<CpuInt>()[0];

    bus.write_data(LVL1VEC, 0x15);
    CHECK(cpuint->is_lvl1_vector(0x15) == true);
    CHECK(cpuint->is_lvl1_vector(0x14) == false);
    CHECK(cpuint->is_lvl1_vector(0x16) == true);  // all vectors >= LVL1VEC are level 1
}

TEST_CASE("CPUINT — highest_priority_vector default") {
    DeviceDescriptor dev = atmega4809;
    Machine machine(dev);
    auto& bus = machine.bus();
    machine.reset();

    CpuInt* cpuint = machine.peripherals_of_type<CpuInt>()[0];

    CHECK(cpuint->highest_priority_vector() == 0);
}

TEST_CASE("CPUINT — highest_priority_vector with LVL0PRI") {
    DeviceDescriptor dev = atmega4809;
    Machine machine(dev);
    auto& bus = machine.bus();
    machine.reset();

    CpuInt* cpuint = machine.peripherals_of_type<CpuInt>()[0];

    bus.write_data(LVL0PRI, 0x20);
    CHECK(cpuint->highest_priority_vector() == 0x20);
}

TEST_CASE("CPUINT — Round-robin cycles through vectors") {
    DeviceDescriptor dev = atmega4809;
    Machine machine(dev);
    auto& bus = machine.bus();
    machine.reset();

    CpuInt* cpuint = machine.peripherals_of_type<CpuInt>()[0];

    bus.write_data(CTRLA, 0x01);
    CHECK(cpuint->round_robin_enabled() == true);

    cpuint->on_interrupt_acknowledged(5);
    CHECK(cpuint->highest_priority_vector() == 6);

    cpuint->on_interrupt_acknowledged(10);
    CHECK(cpuint->highest_priority_vector() == 11);

    cpuint->on_interrupt_acknowledged(0xFF);
    CHECK(cpuint->highest_priority_vector() == 1); // wraps to 1 (vector 0 is reset)
}

TEST_CASE("CPUINT — RETI clears STATUS bits") {
    DeviceDescriptor dev = atmega4809;
    Machine machine(dev);
    auto& bus = machine.bus();
    machine.reset();

    CpuInt* cpuint = machine.peripherals_of_type<CpuInt>()[0];

    cpuint->set_executing_lvl0(true);
    CHECK(bus.read_data(STATUS) == 0x01);

    cpuint->on_reti();
    CHECK(bus.read_data(STATUS) == 0x00);

    cpuint->set_executing_lvl0(true);
    cpuint->set_executing_lvl1(true);
    CHECK(bus.read_data(STATUS) == 0x03);

    cpuint->on_reti();
    CHECK(bus.read_data(STATUS) == 0x01);

    cpuint->on_reti();
    CHECK(bus.read_data(STATUS) == 0x00);
}

TEST_CASE("CPUINT — compact_vector_table and ivsel flags") {
    DeviceDescriptor dev = atmega4809;
    Machine machine(dev);
    auto& bus = machine.bus();
    machine.reset();

    CpuInt* cpuint = machine.peripherals_of_type<CpuInt>()[0];

    CHECK(cpuint->compact_vector_table() == false);
    CHECK(cpuint->ivsel() == false);

    bus.write_data(CTRLA, 0x20);
    CHECK(cpuint->compact_vector_table() == true);

    bus.write_data(CTRLA, 0x40);
    CHECK(cpuint->ivsel() == true);
}

TEST_CASE("CPUINT — Unmapped addresses return 0") {
    DeviceDescriptor dev = atmega4809;
    Machine machine(dev);
    auto& bus = machine.bus();
    machine.reset();

    CHECK(bus.read_data(0x0114) == 0);
    CHECK(bus.read_data(0x0115) == 0);
    CHECK(bus.read_data(0x010F) == 0);
}

TEST_CASE("CPUINT — get_cpuint returns valid pointer") {
    DeviceDescriptor dev = atmega4809;
    Machine machine(dev);
    machine.reset();

    CpuInt* cpuint = machine.peripherals_of_type<CpuInt>()[0];
    REQUIRE(cpuint != nullptr);
    CHECK(cpuint->name() == "CPUINT");
}
