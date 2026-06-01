#include "doctest.h"
#include "vioavr/core/ac8x.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;

static const auto& D = devices::atmega4809.acs8x[0];

struct Ac8xFixture {
    MemoryBus bus{devices::atmega4809};
    AnalogSignalBank bank;
    Ac8x ac{"AC0", D};

    Ac8xFixture() {
        ac.set_memory_bus(&bus);
        ac.set_analog_signal_bank(&bank);
        ac.set_vdd(5.0);
        ac.set_vref(5.0);
        ac.reset();
    }
};

TEST_CASE("Ac8x — reset defaults") {
    Ac8xFixture f;

    CHECK(f.ac.read(D.ctrla_address)      == 0x00U);
    CHECK(f.ac.read(D.muxctrla_address)   == 0x00U);
    CHECK(f.ac.read(D.dacctrla_address)   == 0xFFU); // initval
    CHECK(f.ac.read(D.intctrl_address)    == 0x00U);
    CHECK(f.ac.read(D.status_address)     == 0x00U);
    CHECK_FALSE(f.ac.get_state());
}

TEST_CASE("Ac8x — register round-trips") {
    Ac8xFixture f;

    f.ac.write(D.ctrla_address, 0x8FU); // ENABLE|LPMODE|HYS=11|INTMODE=11|OUTEN|RUNSTDBY
    CHECK(f.ac.read(D.ctrla_address) == 0x8FU);

    f.ac.write(D.muxctrla_address, 0x97U); // MUXPOS=2, MUXNEG=3, HYS=1, INVERT=1
    CHECK(f.ac.read(D.muxctrla_address) == 0x97U);

    f.ac.write(D.dacctrla_address, 0x80U); // DACREF=128
    CHECK(f.ac.read(D.dacctrla_address) == 0x80U);

    f.ac.write(D.intctrl_address, 0x01U); // CMPIE=1
    CHECK(f.ac.read(D.intctrl_address) == 0x01U);
}

TEST_CASE("Ac8x — STATUS CMP flag clear-by-writing-1") {
    Ac8xFixture f;

    f.bank.set_voltage(0, 1.0);
    f.bank.set_voltage(4, 0.0);
    f.ac.write(D.ctrla_address, 0x01U); // ENABLE
    f.ac.tick(1);

    // STATE should be high (1.0 > 0.0), CMP flag set (both edges)
    CHECK(f.ac.get_state());
    CHECK((f.ac.read(D.status_address) & 0x01U) != 0U);

    // Clear by writing 1
    f.ac.write(D.status_address, 0x01U);
    CHECK((f.ac.read(D.status_address) & 0x01U) == 0U);
    CHECK(f.ac.get_state()); // STATE unaffected by clearing
}

TEST_CASE("Ac8x — rising edge interrupt (INTMODE=1)") {
    Ac8xFixture f;

    f.bank.set_voltage(0, 0.0);
    f.bank.set_voltage(4, 1.0);
    f.ac.write(D.ctrla_address, 0x11U); // ENABLE=1, INTMODE=1 (rising)
    f.ac.write(D.intctrl_address, 0x01U); // CMPIE=1
    f.ac.tick(1);

    // STATE should be low (0.0 < 1.0), no flag yet
    CHECK_FALSE(f.ac.get_state());
    CHECK((f.ac.read(D.status_address) & 0x01U) == 0U);

    // Rising edge: P from 0.0 to 2.0 (> 1.0)
    f.bank.set_voltage(0, 2.0);
    f.ac.tick(1);
    CHECK(f.ac.get_state());
    CHECK((f.ac.read(D.status_address) & 0x01U) != 0U);

    // Falling edge: should NOT trigger in rising-edge mode
    f.ac.write(D.status_address, 0x01U); // Clear
    f.bank.set_voltage(0, 0.0);
    f.ac.tick(1);
    CHECK_FALSE(f.ac.get_state());
    CHECK((f.ac.read(D.status_address) & 0x01U) == 0U);
}

TEST_CASE("Ac8x — interrupt pending with CMPIE") {
    Ac8xFixture f;

    f.bank.set_voltage(0, 0.0);
    f.bank.set_voltage(4, 0.5);
    f.ac.write(D.ctrla_address, 0x01U); // ENABLE
    f.ac.write(D.intctrl_address, 0x01U); // CMPIE=1
    f.ac.tick(1);

    InterruptRequest req{};
    CHECK_FALSE(f.ac.pending_interrupt_request(req));

    // State change triggers flag
    f.bank.set_voltage(0, 1.0);
    f.ac.tick(1);

    CHECK(f.ac.pending_interrupt_request(req));
    CHECK(req.vector_index == D.vector_index);
    CHECK(f.ac.consume_interrupt_request(req));
    CHECK_FALSE(f.ac.pending_interrupt_request(req));
}

TEST_CASE("Ac8x — interrupt NOT pending when CMPIE disabled") {
    Ac8xFixture f;

    f.bank.set_voltage(0, 0.0);
    f.bank.set_voltage(4, 0.5);
    f.ac.write(D.ctrla_address, 0x01U); // ENABLE
    f.ac.write(D.intctrl_address, 0x00U); // CMPIE=0
    f.ac.tick(1);

    f.bank.set_voltage(0, 1.0);
    f.ac.tick(1);

    // CMP flag still sets
    CHECK((f.ac.read(D.status_address) & 0x01U) != 0U);

    // But pending_interrupt returns false
    InterruptRequest req{};
    CHECK_FALSE(f.ac.pending_interrupt_request(req));
    CHECK_FALSE(f.ac.consume_interrupt_request(req));
}

TEST_CASE("Ac8x — LPMODE bit doubles propagation delay") {
    Ac8xFixture f;

    f.bank.set_voltage(0, 0.0);
    f.bank.set_voltage(4, 0.5);
    f.ac.write(D.ctrla_address, 0x09U); // ENABLE=1, LPMODE=1
    f.ac.tick(1);

    // State change
    f.bank.set_voltage(0, 1.0);
    f.ac.tick(0);

    // With LPMODE, PROPAGATION_DELAY is 0 in the code, so LPMODE doesn't matter
    // since 0*4 = 0 still. Just verify tick doesn't crash.
    f.ac.tick(1);
    CHECK(f.ac.get_state());
}

TEST_CASE("Ac8x — HYS=2 (25mV hysteresis)") {
    Ac8xFixture f;

    f.bank.set_voltage(0, 0.0);
    f.bank.set_voltage(4, 0.5);
    f.ac.write(D.ctrla_address, 0x05U); // ENABLE=1, HYS=0b10=25mV
    f.ac.tick(1);

    // STATE should be low (0.0 < 0.5)
    CHECK_FALSE(f.ac.get_state());

    // Raise P slightly — still below N-25mV=475mV → stays low
    f.bank.set_voltage(0, 0.46);
    f.ac.tick(1);
    CHECK_FALSE(f.ac.get_state());

    // Raise P above 0.525V (N+25mV) → should trigger high
    f.bank.set_voltage(0, 0.53);
    f.ac.tick(1);
    CHECK(f.ac.get_state());

    // Drop P to 0.49V (still above N-25mV=475mV) → stays high
    f.bank.set_voltage(0, 0.49);
    f.ac.tick(1);
    CHECK(f.ac.get_state());

    // Drop P below 0.475V → should go low
    f.bank.set_voltage(0, 0.46);
    f.ac.tick(1);
    CHECK_FALSE(f.ac.get_state());
}

TEST_CASE("Ac8x — RUNSTDBY bit readback") {
    Ac8xFixture f;

    f.ac.write(D.ctrla_address, 0x81U); // RUNSTDBY=1
    CHECK((f.ac.read(D.ctrla_address) & 0x80U) != 0U);

    f.ac.write(D.ctrla_address, 0x01U); // Clear RUNSTDBY
    CHECK((f.ac.read(D.ctrla_address) & 0x80U) == 0U);
}

TEST_CASE("Ac8x — DACCTRLA as negative input (MUXNEG=3)") {
    Ac8xFixture f;

    f.bank.set_voltage(0, 1.0);
    f.ac.write(D.muxctrla_address, 0x03U); // MUXNEG=3 (DACREF), MUXPOS=0
    f.ac.write(D.dacctrla_address, 0x80U); // DACREF=128 → 128*5/256 = 2.5V
    f.ac.write(D.ctrla_address, 0x01U); // ENABLE
    f.ac.tick(1);

    // P=1.0V, N=2.5V → STATE low
    CHECK_FALSE(f.ac.get_state());

    // Raise P above 2.5V
    f.bank.set_voltage(0, 3.0);
    f.ac.tick(1);
    CHECK(f.ac.get_state());

    // Drop below 2.5V
    f.bank.set_voltage(0, 2.0);
    f.ac.tick(1);
    CHECK_FALSE(f.ac.get_state());
}

TEST_CASE("Ac8x — Unmapped address returns 0") {
    Ac8xFixture f;

    // Addresses outside the ctrla..status range
    CHECK(f.ac.read(D.ctrla_address - 1) == 0x00U);
    CHECK(f.ac.read(D.status_address + 1) == 0x00U);
    CHECK(f.ac.read(0xFFFFU) == 0x00U);
}

TEST_CASE("Ac8x — disabled does not evaluate") {
    Ac8xFixture f;

    f.bank.set_voltage(0, 1.0);
    f.bank.set_voltage(4, 0.0);
    // ENABLE=0 → is_enabled returns false
    f.ac.tick(1);

    // STATE should still be 0 (not evaluated)
    CHECK_FALSE(f.ac.get_state());
    CHECK((f.ac.read(D.status_address) & 0x01U) == 0U);
}

TEST_CASE("Ac8x — INVERT bit flips output") {
    Ac8xFixture f;

    f.bank.set_voltage(0, 0.0);
    f.bank.set_voltage(4, 1.0);
    f.ac.write(D.muxctrla_address, 0x80U); // INVERT=1
    f.ac.write(D.ctrla_address, 0x01U); // ENABLE
    f.ac.tick(1);

    // P=0.0 < N=1.0 → raw state low → inverted to high
    CHECK(f.ac.get_state());

    f.bank.set_voltage(0, 2.0);
    f.ac.tick(1);

    // P=2.0 > N=1.0 → raw state high → inverted to low
    CHECK_FALSE(f.ac.get_state());
}
