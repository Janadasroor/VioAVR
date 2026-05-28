#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/tce.hpp"
#include "vioavr/core/devices/avr16la28.hpp"
#include "vioavr/core/memory_bus.hpp"
#include <cstring>

using namespace vioavr::core;

TEST_CASE("AVR8X TCE Fidelity: Normal Mode Counting") {
    Tce tce("TCE0", devices::avr16la28.timers_tce[0]);
    tce.reset();

    tce.write(devices::avr16la28.timers_tce[0].ctrla_address, 0x01);

    tce.tick(1);
    CHECK(tce.read(devices::avr16la28.timers_tce[0].tcnt_address) == 1);

    tce.tick(4);
    CHECK(tce.read(devices::avr16la28.timers_tce[0].tcnt_address) == 5);
    CHECK(tce.read(devices::avr16la28.timers_tce[0].tcnt_address + 1) == 0);
}

TEST_CASE("AVR8X TCE Fidelity: Period Match and Wrap") {
    Tce tce("TCE0", devices::avr16la28.timers_tce[0]);
    tce.reset();

    tce.write(devices::avr16la28.timers_tce[0].ctrla_address, 0x01);
    tce.write(devices::avr16la28.timers_tce[0].period_address, 10);
    tce.write(devices::avr16la28.timers_tce[0].period_address + 1, 0);

    tce.tick(9);
    CHECK(tce.read(devices::avr16la28.timers_tce[0].tcnt_address) == 9);

    tce.tick(1);
    CHECK(tce.read(devices::avr16la28.timers_tce[0].tcnt_address) == 0);

    u8 intflags = tce.read(devices::avr16la28.timers_tce[0].intflags_address);
    CHECK((intflags & 0x01) != 0);
}

TEST_CASE("AVR8X TCE Fidelity: Double Buffering") {
    Tce tce("TCE0", devices::avr16la28.timers_tce[0]);
    tce.reset();

    tce.write(devices::avr16la28.timers_tce[0].ctrla_address, 0x01);
    tce.write(devices::avr16la28.timers_tce[0].period_address, 10);
    tce.write(devices::avr16la28.timers_tce[0].period_address + 1, 0);

    tce.tick(5);
    CHECK(tce.read(devices::avr16la28.timers_tce[0].tcnt_address) == 5);

    tce.write(devices::avr16la28.timers_tce[0].perbuf_address, 20);
    tce.write(devices::avr16la28.timers_tce[0].perbuf_address + 1, 0);

    CHECK(tce.read(devices::avr16la28.timers_tce[0].period_address) == 10);

    tce.tick(4);
    CHECK(tce.read(devices::avr16la28.timers_tce[0].tcnt_address) == 9);

    tce.tick(1);
    CHECK(tce.read(devices::avr16la28.timers_tce[0].tcnt_address) == 0);
    CHECK(tce.read(devices::avr16la28.timers_tce[0].period_address) == 20);
}

TEST_CASE("AVR8X TCE Fidelity: Compare Match Interrupt") {
    MemoryBus bus{devices::avr16la28};
    Tce tce("TCE0", devices::avr16la28.timers_tce[0]);
    tce.set_memory_bus(&bus);
    tce.reset();

    tce.write(devices::avr16la28.timers_tce[0].ctrla_address, 0x01);
    tce.write(devices::avr16la28.timers_tce[0].period_address, 20);
    tce.write(devices::avr16la28.timers_tce[0].period_address + 1, 0);
    tce.write(devices::avr16la28.timers_tce[0].cmp0_address, 7);
    tce.write(devices::avr16la28.timers_tce[0].cmp0_address + 1, 0);
    tce.write(devices::avr16la28.timers_tce[0].intctrl_address, 0x10);

    tce.tick(7);
    CHECK(tce.read(devices::avr16la28.timers_tce[0].tcnt_address) == 7);

    u8 intflags = tce.read(devices::avr16la28.timers_tce[0].intflags_address);
    CHECK((intflags & 0x10) != 0);

    InterruptRequest req;
    CHECK(tce.pending_interrupt_request(req));
    CHECK(req.vector_index == devices::avr16la28.timers_tce[0].cmp0_vector_index);

    tce.write(devices::avr16la28.timers_tce[0].intflags_address, 0x10);
    CHECK((tce.read(devices::avr16la28.timers_tce[0].intflags_address) & 0x10) == 0);
}

TEST_CASE("AVR8X TCE Fidelity: Dual Slope Mode") {
    Tce tce("TCE0", devices::avr16la28.timers_tce[0]);
    tce.reset();

    tce.write(devices::avr16la28.timers_tce[0].ctrla_address, 0x01);
    tce.write(devices::avr16la28.timers_tce[0].ctrlb_address, 0x05);

    tce.tick(1);
    CHECK(tce.read(devices::avr16la28.timers_tce[0].tcnt_address) == 1);

    tce.tick(1);
    CHECK(tce.read(devices::avr16la28.timers_tce[0].tcnt_address) == 2);

    tce.write(devices::avr16la28.timers_tce[0].period_address, 10);
    tce.write(devices::avr16la28.timers_tce[0].period_address + 1, 0);

    tce.tick(1);
    CHECK(tce.read(devices::avr16la28.timers_tce[0].tcnt_address) == 3);

    tce.tick(7);
    CHECK(tce.read(devices::avr16la28.timers_tce[0].tcnt_address) == 10);
    // Now counting down
    tce.tick(1);
    CHECK(tce.read(devices::avr16la28.timers_tce[0].tcnt_address) <= 10);

    // Continue counting down
    tce.tick(5);
    CHECK(tce.read(devices::avr16la28.timers_tce[0].tcnt_address) == 4);

    tce.tick(4);
    u8 tcnt_val = tce.read(devices::avr16la28.timers_tce[0].tcnt_address);
    CHECK_MESSAGE(tcnt_val == 0, "tcnt should reach 0 after counting down");

    u8 intflags = tce.read(devices::avr16la28.timers_tce[0].intflags_address);
    CHECK((intflags & 0x01) != 0);
}
