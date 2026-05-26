#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/zcd.hpp"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/memory_bus.hpp"

using namespace vioavr::core;

static Zcd make_zcd() {
    ZcdDescriptor d;
    d.ctrla_address = 0x0080;
    d.vector_index = 5;
    d.pin_address = 0x1234;
    d.pin_bit = 0;
    return Zcd{d};
}

TEST_CASE("ZCD — Reset defaults") {
    auto z = make_zcd();
    z.reset();
    CHECK(z.read(0x0080) == 0x00U);
    InterruptRequest irq;
    CHECK(z.pending_interrupt_request(irq) == false);
}

TEST_CASE("ZCD — CTRLA register round-trip") {
    auto z = make_zcd();
    z.reset();

    z.write(0x0080, 0x01U); // ENABLE
    CHECK((z.read(0x0080) & 0x7FU) == 0x01U);

    z.write(0x0080, 0x3EU); // Bits 1-5
    CHECK((z.read(0x0080) & 0x7FU) == 0x3EU);

    z.write(0x0080, 0x7FU); // All writable bits
    CHECK((z.read(0x0080) & 0x7FU) == 0x7FU);
}

TEST_CASE("ZCD — STATE bit read-only") {
    auto z = make_zcd();
    z.reset();

    // STATE bit reflects pin, not writes
    z.write(0x0080, 0x80U); // Attempt to set STATE
    CHECK((z.read(0x0080) & 0x80U) == 0U); // STATE still 0

    // Enable + set pin high
    z.write(0x0080, 0x01U);
    z.on_external_pin_change(0x1234, 0, PinLevel::high);
    CHECK((z.read(0x0080) & 0x80U) != 0U); // STATE=1

    // Writing 0x80 should not clear it
    z.write(0x0080, 0x01U); // No STATE bit in write value
    CHECK((z.read(0x0080) & 0x80U) != 0U); // STATE still 1
}

TEST_CASE("ZCD — Disabled does not generate interrupts") {
    auto z = make_zcd();
    z.reset();

    // ENABLE=0, pin change should not trigger interrupt
    z.on_external_pin_change(0x1234, 0, PinLevel::high);
    InterruptRequest irq;
    CHECK(z.pending_interrupt_request(irq) == false);

    // But pin state is still tracked
    CHECK((z.read(0x0080) & 0x80U) != 0U);
}

TEST_CASE("ZCD — Wrong pin does not trigger") {
    auto z = make_zcd();
    z.reset();
    z.write(0x0080, 0x01U); // ENABLE

    // Different pin address
    z.on_external_pin_change(0x5678, 0, PinLevel::high);
    InterruptRequest irq;
    CHECK(z.pending_interrupt_request(irq) == false);
    CHECK((z.read(0x0080) & 0x80U) == 0U);

    // Wrong bit index
    z.on_external_pin_change(0x1234, 1, PinLevel::high);
    CHECK(z.pending_interrupt_request(irq) == false);
}

TEST_CASE("ZCD — Interrupt on crossing") {
    auto z = make_zcd();
    z.reset();
    z.write(0x0080, 0x01U); // ENABLE

    z.on_external_pin_change(0x1234, 0, PinLevel::high);
    InterruptRequest irq;
    CHECK(z.pending_interrupt_request(irq) == true);
    CHECK(irq.vector_index == 5);

    // Consume
    CHECK(z.consume_interrupt_request(irq) == true);
    CHECK(z.pending_interrupt_request(irq) == false);
}

TEST_CASE("ZCD — Interrupt re-arms on second crossing") {
    auto z = make_zcd();
    z.reset();
    z.write(0x0080, 0x01U); // ENABLE

    z.on_external_pin_change(0x1234, 0, PinLevel::high);
    InterruptRequest irq;
    CHECK(z.pending_interrupt_request(irq) == true);
    z.consume_interrupt_request(irq);

    // Cross back
    z.on_external_pin_change(0x1234, 0, PinLevel::low);
    CHECK(z.pending_interrupt_request(irq) == true);

    // Cross again
    z.consume_interrupt_request(irq);
    z.on_external_pin_change(0x1234, 0, PinLevel::high);
    CHECK(z.pending_interrupt_request(irq) == true);
}

TEST_CASE("ZCD — Same pin level does not retrigger") {
    auto z = make_zcd();
    z.reset();
    z.write(0x0080, 0x01U); // ENABLE

    z.on_external_pin_change(0x1234, 0, PinLevel::high);
    InterruptRequest irq;
    CHECK(z.pending_interrupt_request(irq) == true);
    z.consume_interrupt_request(irq);

    // Same level again — no change
    z.on_external_pin_change(0x1234, 0, PinLevel::high);
    CHECK(z.pending_interrupt_request(irq) == false);
}

TEST_CASE("ZCD — Enable after crossing triggers interrupt") {
    auto z = make_zcd();
    z.reset();

    // Cross while disabled
    z.on_external_pin_change(0x1234, 0, PinLevel::high);
    InterruptRequest irq;
    CHECK(z.pending_interrupt_request(irq) == false);

    // Now enable — should not retroactively trigger
    z.write(0x0080, 0x01U);
    CHECK(z.pending_interrupt_request(irq) == false);
}

TEST_CASE("ZCD — Unmapped address returns 0") {
    auto z = make_zcd();
    z.reset();
    CHECK(z.read(0x0081) == 0U);
    CHECK(z.read(0xFFFF) == 0U);
    CHECK(z.read(0x0000) == 0U);
}

TEST_CASE("ZCD — Consume without pending returns false") {
    auto z = make_zcd();
    z.reset();
    InterruptRequest irq;
    CHECK(z.consume_interrupt_request(irq) == false);
}

TEST_CASE("ZCD — Enable toggle preserves pin state") {
    auto z = make_zcd();
    z.reset();

    z.on_external_pin_change(0x1234, 0, PinLevel::high);
    CHECK((z.read(0x0080) & 0x80U) != 0U);

    z.write(0x0080, 0x01U); // ENABLE=1
    CHECK((z.read(0x0080) & 0x80U) != 0U); // STATE still 1

    z.write(0x0080, 0x00U); // Disable
    CHECK((z.read(0x0080) & 0x80U) != 0U); // STATE still tracked
}
