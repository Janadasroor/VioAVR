#include "doctest.h"
#include "vioavr/core/ircom.hpp"
#include "vioavr/core/pin_mux.hpp"

using namespace vioavr::core;

static IrcomDescriptor make_test_desc() noexcept {
    IrcomDescriptor d{};
    d.ctrl_address = 0x10;
    d.txplctrl_address = 0x11;
    d.rxplctrl_address = 0x12;
    d.pin_address = 0x404;
    d.pin_bit_index = 5;
    d.vector_index = 15;
    return d;
}

TEST_CASE("XMEGA IRCOM — Register access") {
    Ircom ircom("IRCOM", make_test_desc());
    ircom.reset();

    CHECK(ircom.read(0x10) == 0);
    ircom.write(0x10, 0x03U);
    CHECK(ircom.read(0x10) == 0x03U);

    ircom.write(0x11, 0x0AU);
    CHECK(ircom.read(0x11) == 0x0AU);

    ircom.write(0x12, 0x14U);
    CHECK(ircom.read(0x12) == 0x14U);
}

TEST_CASE("XMEGA IRCOM — Modulator toggles with TX enabled") {
    Ircom ircom("IRCOM", make_test_desc());
    ircom.reset();

    ircom.write(0x11, 3U);
    ircom.write(0x10, 0x01U);

    CHECK(!ircom.modulator_state());
    ircom.tick(3);
    CHECK(ircom.modulator_state());
    ircom.tick(3);
    CHECK(!ircom.modulator_state());
}

TEST_CASE("XMEGA IRCOM — Short pulse length (1 cycle)") {
    Ircom ircom("IRCOM", make_test_desc());
    ircom.reset();

    ircom.write(0x11, 1U);
    ircom.write(0x10, 0x01U);

    CHECK(!ircom.modulator_state());
    ircom.tick(1);
    CHECK(ircom.modulator_state());
    ircom.tick(1);
    CHECK(!ircom.modulator_state());
}

TEST_CASE("XMEGA IRCOM — TX disabled stops modulator") {
    Ircom ircom("IRCOM", make_test_desc());
    ircom.reset();

    ircom.write(0x11, 3U);
    ircom.write(0x10, 0x01U);
    ircom.tick(3);
    CHECK(ircom.modulator_state());

    ircom.write(0x10, 0x00U);
    CHECK(!ircom.modulator_state());

    ircom.tick(10);
    CHECK(!ircom.modulator_state());
}

TEST_CASE("XMEGA IRCOM — PinMux output driven when enabled") {
    PinMux pin_mux(6);
    pin_mux.register_port(0x404, 0);
    bool pin_changed = false;
    PinState changed_state;
    pin_mux.set_callback([&](u8, u8, const PinState& s) {
        pin_changed = true;
        changed_state = s;
    });

    Ircom ircom("IRCOM", make_test_desc());
    ircom.set_pin_mux(&pin_mux);
    ircom.set_output_pin(0, 5);
    ircom.reset();

    // Enable with TXPLCTRL=3
    ircom.write(0x11, 3U);
    ircom.write(0x10, 0x01U);

    // First tick should drive HIGH via PinMux
    ircom.tick(3);
    CHECK(ircom.modulator_state());
    CHECK(pin_changed);
    CHECK(changed_state.owner == PinOwner::ircom);
    CHECK(changed_state.is_output);
    CHECK(changed_state.drive_level);

    // Next tick drives LOW
    pin_changed = false;
    ircom.tick(3);
    CHECK(!ircom.modulator_state());
    CHECK(pin_changed);
    CHECK(changed_state.owner == PinOwner::ircom);
    CHECK(changed_state.is_output);
    CHECK(!changed_state.drive_level);
}

TEST_CASE("XMEGA IRCOM — Disable releases PinMux pin") {
    PinMux pin_mux(6);
    pin_mux.register_port(0x404, 0);
    bool pin_changed = false;
    pin_mux.set_callback([&](u8, u8, const PinState& s) {
        pin_changed = true;
    });

    Ircom ircom("IRCOM", make_test_desc());
    ircom.set_pin_mux(&pin_mux);
    ircom.set_output_pin(0, 5);
    ircom.reset();

    ircom.write(0x11, 3U);
    ircom.write(0x10, 0x01U);
    ircom.tick(3);
    CHECK(ircom.modulator_state());

    // Disable IRCOM → pin should be released
    pin_changed = false;
    ircom.write(0x10, 0x00U);
    CHECK(!ircom.modulator_state());

    auto state = pin_mux.get_state(0, 5);
    CHECK(state.owner == PinOwner::gpio);
    CHECK(!state.is_output);
}

TEST_CASE("XMEGA IRCOM — Address range covers all registers") {
    Ircom ircom("IRCOM", make_test_desc());

    auto ranges = ircom.mapped_ranges();
    REQUIRE(!ranges.empty());

    u16 r_begin = ranges[0].begin;
    u16 r_end = ranges[0].end;

    CHECK(0x10 >= r_begin);
    CHECK(0x10 <= r_end);
    CHECK(0x11 >= r_begin);
    CHECK(0x11 <= r_end);
    CHECK(0x12 >= r_begin);
    CHECK(0x12 <= r_end);
}

TEST_CASE("XMEGA IRCOM — Default pulse length is 1 when TXPLCTRL=0") {
    Ircom ircom("IRCOM", make_test_desc());
    ircom.reset();

    // Enable without setting TXPLCTRL (default 0)
    ircom.write(0x10, 0x01U);

    CHECK(!ircom.modulator_state());
    ircom.tick(1);
    CHECK(ircom.modulator_state());
    ircom.tick(1);
    CHECK(!ircom.modulator_state());
}

TEST_CASE("XMEGA IRCOM — RX basic pulse detection") {
    Ircom ircom("IRCOM", make_test_desc());
    ircom.reset();

    // Enable RX with threshold=5
    ircom.write(0x12, 5U);
    ircom.write(0x10, 0x02U);

    // Advance time, then simulate a pin change after 10 cycles
    ircom.tick(10);
    ircom.on_external_pin_change(0x404, 5, PinLevel::high);

    // Pulse should be detected (10 >= 5)
    CHECK(ircom.rx_pulse_counter() == 1);
    CHECK(ircom.rx_busy());

    // Second pulse after 3 cycles (below threshold)
    ircom.tick(3);
    ircom.on_external_pin_change(0x404, 5, PinLevel::low);

    CHECK(ircom.rx_pulse_counter() == 1); // not counted
}

TEST_CASE("XMEGA IRCOM — RX disabled ignores pin changes") {
    Ircom ircom("IRCOM", make_test_desc());
    ircom.reset();

    ircom.write(0x12, 5U);
    // RX not enabled (CTRL bit 1 = 0)

    ircom.tick(10);
    ircom.on_external_pin_change(0x404, 5, PinLevel::high);

    CHECK(ircom.rx_pulse_counter() == 0);
    CHECK(!ircom.rx_busy());
}

TEST_CASE("XMEGA IRCOM — RX wrong pin address ignored") {
    Ircom ircom("IRCOM", make_test_desc());
    ircom.reset();

    ircom.write(0x12, 5U);
    ircom.write(0x10, 0x02U);

    ircom.tick(10);
    ircom.on_external_pin_change(0x400, 5, PinLevel::high); // wrong address

    CHECK(ircom.rx_pulse_counter() == 0);
}

TEST_CASE("XMEGA IRCOM — RX interrupt dispatch") {
    auto d = make_test_desc();
    Ircom ircom("IRCOM", d);
    ircom.reset();

    // No vector index → no interrupt
    ircom.write(0x12, 5U);
    ircom.write(0x10, 0x02U);
    ircom.tick(10);
    ircom.on_external_pin_change(0x404, 5, PinLevel::high);

    InterruptRequest req;
    CHECK(ircom.pending_interrupt_request(req));
    CHECK(req.vector_index == d.vector_index);

    CHECK(ircom.consume_interrupt_request(req));
    CHECK(req.vector_index == d.vector_index);
    CHECK(!ircom.rx_busy());
    CHECK(!ircom.pending_interrupt_request(req));
}

TEST_CASE("XMEGA IRCOM — RX reset clears state") {
    Ircom ircom("IRCOM", make_test_desc());
    ircom.reset();

    ircom.write(0x12, 5U);
    ircom.write(0x10, 0x02U);
    ircom.tick(10);
    ircom.on_external_pin_change(0x404, 5, PinLevel::high);
    CHECK(ircom.rx_pulse_counter() == 1);

    ircom.reset();
    CHECK(ircom.rx_pulse_counter() == 0);
    CHECK(!ircom.rx_busy());
}

TEST_CASE("XMEGA IRCOM — RX disable on CTRL write clears busy") {
    Ircom ircom("IRCOM", make_test_desc());
    ircom.reset();

    ircom.write(0x12, 5U);
    ircom.write(0x10, 0x02U);
    ircom.tick(10);
    ircom.on_external_pin_change(0x404, 5, PinLevel::high);
    CHECK(ircom.rx_busy());

    // Disable RX
    ircom.write(0x10, 0x00U);
    CHECK(!ircom.rx_busy());
    CHECK(ircom.rx_pulse_counter() == 0);
}

TEST_CASE("XMEGA IRCOM — Reset clears modulator state") {
    Ircom ircom("IRCOM", make_test_desc());
    ircom.reset();

    ircom.write(0x11, 3U);
    ircom.write(0x10, 0x01U);
    ircom.tick(3);
    CHECK(ircom.modulator_state());

    ircom.reset();
    CHECK(!ircom.modulator_state());
    CHECK(ircom.read(0x10) == 0);
    CHECK(ircom.read(0x11) == 0);
    CHECK(ircom.read(0x12) == 0);
}
