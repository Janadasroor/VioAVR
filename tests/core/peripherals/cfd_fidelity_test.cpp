#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/cfd.hpp"

using namespace vioavr::core;

static Cfd* get_cfd(Machine& machine) {
    auto cfds = machine.peripherals_of_type<Cfd>();
    return cfds.empty() ? nullptr : cfds[0];
}

TEST_CASE("CFD: ATmega324PB - XFDCSR initial state") {
    auto machine = Machine::create_for_device("ATmega324PB");
    REQUIRE(machine != nullptr);
    CHECK(machine->bus().read_data(0x62) == 0x00);
}

TEST_CASE("CFD: ATmega324PB - write XFDIE, read back") {
    auto machine = Machine::create_for_device("ATmega324PB");
    REQUIRE(machine != nullptr);
    machine->bus().write_data(0x62, 0x01);
    CHECK(machine->bus().read_data(0x62) == 0x01);
}

TEST_CASE("CFD: ATmega324PB - write XFDIF clears XFDIF, preserves XFDIE") {
    auto machine = Machine::create_for_device("ATmega324PB");
    REQUIRE(machine != nullptr);
    auto* cfd = get_cfd(*machine);
    REQUIRE(cfd != nullptr);
    machine->bus().write_data(0x62, 0x01);
    cfd->trigger_failure();
    CHECK(machine->bus().read_data(0x62) == 0x03);
    machine->bus().write_data(0x62, 0x03);
    CHECK(machine->bus().read_data(0x62) == 0x01);
}

TEST_CASE("CFD: ATmega324PB - trigger_failure via Cfd pointer") {
    auto machine = Machine::create_for_device("ATmega324PB");
    REQUIRE(machine != nullptr);
    auto* cfd = get_cfd(*machine);
    REQUIRE(cfd != nullptr);
    cfd->trigger_failure();
    CHECK((machine->bus().read_data(0x62) & 0x02) != 0);
}

TEST_CASE("CFD: ATmega324PB - interrupt when XFDIE and XFDIF both set") {
    auto machine = Machine::create_for_device("ATmega324PB");
    REQUIRE(machine != nullptr);
    machine->bus().write_data(0x62, 0x01);
    auto* cfd = get_cfd(*machine);
    REQUIRE(cfd != nullptr);
    cfd->trigger_failure();
    InterruptRequest req;
    CHECK(cfd->pending_interrupt_request(req));
    CHECK(req.vector_index == 38);
}

TEST_CASE("CFD: ATmega324PB - no interrupt without XFDIE") {
    auto machine = Machine::create_for_device("ATmega324PB");
    REQUIRE(machine != nullptr);
    auto* cfd = get_cfd(*machine);
    REQUIRE(cfd != nullptr);
    cfd->trigger_failure();
    InterruptRequest req;
    CHECK(!cfd->pending_interrupt_request(req));
}

TEST_CASE("CFD: ATmega324PB - consume clears XFDIF") {
    auto machine = Machine::create_for_device("ATmega324PB");
    REQUIRE(machine != nullptr);
    machine->bus().write_data(0x62, 0x01);
    auto* cfd = get_cfd(*machine);
    REQUIRE(cfd != nullptr);
    cfd->trigger_failure();
    InterruptRequest req;
    CHECK(cfd->consume_interrupt_request(req));
    CHECK((machine->bus().read_data(0x62) & 0x02) == 0);
}

TEST_CASE("CFD: ATmega324PB - writing 0 clears XFDIE") {
    auto machine = Machine::create_for_device("ATmega324PB");
    REQUIRE(machine != nullptr);
    machine->bus().write_data(0x62, 0x01);
    CHECK(machine->bus().read_data(0x62) == 0x01);
    machine->bus().write_data(0x62, 0x00);
    CHECK(machine->bus().read_data(0x62) == 0x00);
}

TEST_CASE("CFD: ATmega328PB - XFDCSR initial state") {
    auto machine = Machine::create_for_device("ATmega328PB");
    REQUIRE(machine != nullptr);
    CHECK(machine->bus().read_data(0x62) == 0x00);
}

TEST_CASE("CFD: ATmega328PB - interrupt vector 36") {
    auto machine = Machine::create_for_device("ATmega328PB");
    REQUIRE(machine != nullptr);
    machine->bus().write_data(0x62, 0x01);
    auto* cfd = get_cfd(*machine);
    REQUIRE(cfd != nullptr);
    cfd->trigger_failure();
    InterruptRequest req;
    CHECK(cfd->pending_interrupt_request(req));
    CHECK(req.vector_index == 36);
}

TEST_CASE("CFD: ATmega328PB - reset clears XFDCSR") {
    auto machine = Machine::create_for_device("ATmega328PB");
    REQUIRE(machine != nullptr);
    auto* cfd = get_cfd(*machine);
    REQUIRE(cfd != nullptr);
    machine->bus().write_data(0x62, 0x01);
    cfd->trigger_failure();
    CHECK(machine->bus().read_data(0x62) == 0x03);
    machine->reset();
    CHECK(machine->bus().read_data(0x62) == 0x00);
}
