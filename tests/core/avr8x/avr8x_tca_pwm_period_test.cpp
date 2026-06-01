#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include "vioavr/core/tca.hpp"

using namespace vioavr::core;

namespace {
    u16 read_tcnt(Tca& tca, const TcaDescriptor& desc) {
        u8 l = tca.read(desc.tcnt_address);
        u8 h = tca.read(desc.tcnt_address + 1);
        return static_cast<u16>(l) | (static_cast<u16>(h) << 8U);
    }
}

TEST_CASE("ATmega4809 TCA0: Single-Slope Counter Sequence") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();

    Tca* tca = nullptr;
    for (auto* p : bus.peripherals()) {
        if (auto* t = dynamic_cast<Tca*>(p)) tca = t;
    }
    REQUIRE(tca != nullptr);

    const auto& desc = devices::atmega4809.timers_tca[0];

    std::vector<u16> prog(2048, 0x0000);
    bus.load_flash(prog);
    machine.reset();

    bus.write_data(desc.period_address, 0x0F);
    bus.write_data(desc.period_address + 1, 0x00);
    bus.write_data(desc.ctrla_address, 0x01);

    // tcnt=0 at reset; first tick: tcnt=1
    bus.tick_peripherals(1);
    CHECK(read_tcnt(*tca, desc) == 1);

    bus.tick_peripherals(14);
    CHECK(read_tcnt(*tca, desc) == 15);

    bus.tick_peripherals(1);
    CHECK(read_tcnt(*tca, desc) == 0);
    CHECK((tca->read(desc.intflags_address) & 0x01) != 0);
}

TEST_CASE("ATmega4809 TCA0: Fast PWM WO Output Duty Cycle") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();

    Tca* tca = nullptr;
    for (auto* p : bus.peripherals()) {
        if (auto* t = dynamic_cast<Tca*>(p)) tca = t;
    }
    REQUIRE(tca != nullptr);

    const auto& desc = devices::atmega4809.timers_tca[0];

    std::vector<u16> prog(2048, 0x0000);
    bus.load_flash(prog);
    machine.reset();

    bus.write_data(desc.period_address, 0xFF);
    bus.write_data(desc.period_address + 1, 0x00);
    bus.write_data(desc.cmp0_address, 0x40);
    bus.write_data(desc.cmp0_address + 1, 0x00);
    bus.write_data(desc.ctrlb_address, 0x10);
    bus.write_data(desc.ctrla_address, 0x01);

    // tcnt=0 initially; after 63 ticks: tcnt=63, WO0=1 (63<64)
    for (int i = 0; i < 63; ++i) {
        bus.tick_peripherals(1);
        CHECK_MESSAGE(tca->get_wo_level(0), "WO0 high at tcnt=" << (i + 1));
    }

    // tick 64: tcnt=64, CMP0 match, WO0 goes low (64<64=false)
    bus.tick_peripherals(1);
    CHECK_FALSE(tca->get_wo_level(0));

    // tcnt=65..255: WO0=0
    for (int i = 65; i < 256; ++i) {
        bus.tick_peripherals(1);
        CHECK_FALSE_MESSAGE(tca->get_wo_level(0), "WO0 low at tcnt=" << i);
    }

    // tick 256: overflow, tcnt=0, WO0=1
    bus.tick_peripherals(1);
    CHECK(tca->get_wo_level(0));
}

TEST_CASE("ATmega4809 TCA0: Overflow Interrupt Cycle-Precise") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();

    Tca* tca = nullptr;
    for (auto* p : bus.peripherals()) {
        if (auto* t = dynamic_cast<Tca*>(p)) tca = t;
    }
    REQUIRE(tca != nullptr);

    const auto& desc = devices::atmega4809.timers_tca[0];

    std::vector<u16> prog(2048, 0x0000);
    bus.load_flash(prog);
    machine.reset();

    bus.write_data(desc.intctrl_address, 0x01);
    bus.write_data(desc.period_address, 0x04);
    bus.write_data(desc.period_address + 1, 0x00);
    bus.write_data(desc.ctrla_address, 0x01);

    InterruptRequest req;

    // 4 ticks: tcnt=1,2,3,4 — no overflow yet
    for (int i = 0; i < 4; ++i)
        bus.tick_peripherals(1);
    CHECK_FALSE(tca->pending_interrupt_request(req));

    // tick 5: tcnt=0 (overflow)
    bus.tick_peripherals(1);
    CHECK(tca->pending_interrupt_request(req));
    CHECK(req.vector_index == desc.luf_ovf_vector_index);

    CHECK(tca->consume_interrupt_request(req));
    CHECK_FALSE(tca->pending_interrupt_request(req));

    // second period: 5 more ticks
    for (int i = 0; i < 5; ++i)
        bus.tick_peripherals(1);
    CHECK(tca->pending_interrupt_request(req));
}

TEST_CASE("ATmega4809 TCA0: Multiple Period Consistency") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();

    Tca* tca = nullptr;
    for (auto* p : bus.peripherals()) {
        if (auto* t = dynamic_cast<Tca*>(p)) tca = t;
    }
    REQUIRE(tca != nullptr);

    const auto& desc = devices::atmega4809.timers_tca[0];

    std::vector<u16> prog(2048, 0x0000);
    bus.load_flash(prog);
    machine.reset();

    bus.write_data(desc.period_address, 0xFF);
    bus.write_data(desc.period_address + 1, 0x00);
    bus.write_data(desc.ctrla_address, 0x01);

    for (int p = 0; p < 10; ++p) {
        bus.tick_peripherals(256);
        CHECK(read_tcnt(*tca, desc) == 0);
        CHECK((tca->read(desc.intflags_address) & 0x01) != 0);
        tca->write(desc.intflags_address, 0x01);
    }
}
