#include "doctest.h"
#include "vioavr/core/bodctrl.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

static const auto& D = atmega4809.bod;

TEST_CASE("BodCtrl — CTRLA register SLEEP and ACTIVE fields") {
    BodCtrl bod{D};
    bod.reset();

    // Default: SLEEP=01 (ENABLED), ACTIVE=01 (ENABLED), SAMPFREQ=0
    CHECK(bod.read(D.ctrla_address) == 0x05U);

    // Write all 8 bits — test each field
    bod.write(D.ctrla_address, 0x00U); // SLEEP=00 (DISABLED), ACTIVE=00 (DISABLED)
    CHECK(bod.read(D.ctrla_address) == 0x00U);

    bod.write(D.ctrla_address, 0x01U); // SLEEP=01 (ENABLED)
    CHECK((bod.read(D.ctrla_address) & 0x03U) == 0x01U);

    bod.write(D.ctrla_address, 0x02U); // SLEEP=10 (SAMPLED)
    CHECK((bod.read(D.ctrla_address) & 0x03U) == 0x02U);

    bod.write(D.ctrla_address, 0x03U); // SLEEP=11
    CHECK((bod.read(D.ctrla_address) & 0x03U) == 0x03U);

    bod.write(D.ctrla_address, 0x04U); // ACTIVE=01 (ENABLED), SLEEP=00
    CHECK((bod.read(D.ctrla_address) & 0x0CU) == 0x04U);

    bod.write(D.ctrla_address, 0x08U); // ACTIVE=10 (SAMPLED)
    CHECK((bod.read(D.ctrla_address) & 0x0CU) == 0x08U);

    bod.write(D.ctrla_address, 0x0CU); // ACTIVE=11
    CHECK((bod.read(D.ctrla_address) & 0x0CU) == 0x0CU);
}

TEST_CASE("BodCtrl — CTRLA SAMPFREQ bit") {
    BodCtrl bod{D};
    bod.reset();

    bod.write(D.ctrla_address, 0x80U); // SAMPFREQ=1 (bit 7)
    CHECK((bod.read(D.ctrla_address) & 0x80U) != 0U);

    bod.write(D.ctrla_address, 0x00U); // Clear all
    CHECK((bod.read(D.ctrla_address) & 0x80U) == 0U);
}

TEST_CASE("BodCtrl — CTRLB register") {
    BodCtrl bod{D};
    bod.reset();

    CHECK(bod.read(D.ctrlb_address) == 0x00U);

    bod.write(D.ctrlb_address, 0xFFU);
    CHECK(bod.read(D.ctrlb_address) == 0xFFU);

    bod.write(D.ctrlb_address, 0x00U);
    CHECK(bod.read(D.ctrlb_address) == 0x00U);
}

TEST_CASE("BodCtrl — VLM rising edge interrupt (VLMCFG=1)") {
    BodCtrl bod{D};
    bod.reset();

    bod.write(D.vlmctrla_address, 0x00U); // VLMLVL=0
    bod.write(D.intctrl_address, 0x03U);  // VLMIE=1, VLMCFG=0b01=rising

    // Start below threshold
    bod.update_vdd(1.0, 5.0);

    InterruptRequest req{};
    CHECK_FALSE(bod.pending_interrupt_request(req)); // No rising yet

    // Rise above → trigger
    bod.update_vdd(5.0, 5.0);
    CHECK(bod.pending_interrupt_request(req));

    // Consume
    CHECK(bod.consume_interrupt_request(req));
    CHECK_FALSE(bod.pending_interrupt_request(req));

    // Staying above does not re-trigger
    bod.update_vdd(5.0, 5.0);
    CHECK_FALSE(bod.pending_interrupt_request(req));
}

TEST_CASE("BodCtrl — VLMLVL=1 (25% threshold)") {
    BodCtrl bod{D};
    bod.reset();

    // VLMLVL=1 → threshold = 5.0 * (1 - 0.25) = 3.75V
    bod.write(D.vlmctrla_address, 0x01U);
    bod.write(D.intctrl_address, 0x01U); // VLMIE=1

    InterruptRequest req{};
    bod.update_vdd(5.0, 5.0); // Above 3.75V
    CHECK_FALSE(bod.pending_interrupt_request(req));

    // Drop to 3.5V → below 3.75V → falling edge
    bod.update_vdd(3.5, 5.0);
    CHECK(bod.pending_interrupt_request(req));
    CHECK((bod.read(D.status_address) & 0x01U) != 0U); // VLMS=1

    bod.consume_interrupt_request(req);

    // Switch to both-edge mode, then rise back
    bod.write(D.intctrl_address, 0x05U); // VLMCFG=both
    bod.update_vdd(4.0, 5.0);
    CHECK((bod.read(D.status_address) & 0x01U) == 0U); // VLMS=0
    bod.consume_interrupt_request(req);
}

TEST_CASE("BodCtrl — VLMLVL=2 (45% threshold)") {
    BodCtrl bod{D};
    bod.reset();

    // VLMLVL=2 → threshold = 5.0 * (1 - 0.45) = 2.75V
    bod.write(D.vlmctrla_address, 0x02U);
    bod.write(D.intctrl_address, 0x01U);

    InterruptRequest req{};
    bod.update_vdd(5.0, 5.0);
    CHECK_FALSE(bod.pending_interrupt_request(req));

    // Drop to 2.5V → below 2.75V
    bod.update_vdd(2.5, 5.0);
    CHECK(bod.pending_interrupt_request(req));
    CHECK((bod.read(D.status_address) & 0x01U) != 0U);
}

TEST_CASE("BodCtrl — VLMLVL=3 (special: always at threshold)") {
    BodCtrl bod{D};
    bod.reset();

    // VLMLVL=3 → vlm_thresholds[3] = 0.0 → threshold = vdd_nominal * 1.0
    // So at vdd=5.0 and nominal=5.0, threshold = 5.0*1.0 = 5.0
    // vdd<5.0 means below threshold
    bod.write(D.vlmctrla_address, 0x03U);
    bod.write(D.intctrl_address, 0x01U);

    InterruptRequest req{};
    bod.update_vdd(5.0, 5.0); // vdd == threshold (not below)
    CHECK_FALSE(bod.pending_interrupt_request(req));

    // vdd slightly below nominal → below threshold (5.0 < 5.0*1.0 is false without float help)
    // threshold = vdd_nominal * (1.0 - 0.0) = vdd_nominal
    // So vdd=4.9 < 5.0 → below!
    bod.update_vdd(4.9, 5.0);
    CHECK(bod.pending_interrupt_request(req));
    CHECK((bod.read(D.status_address) & 0x01U) != 0U);
}

TEST_CASE("BodCtrl — Non-5V nominal VDD") {
    BodCtrl bod{D};
    bod.reset();

    // VLMLVL=0 → threshold = 3.3 * (1 - 0.05) = 3.135V
    bod.write(D.vlmctrla_address, 0x00U);
    bod.write(D.intctrl_address, 0x01U);

    InterruptRequest req{};
    bod.update_vdd(3.3, 3.3); // Above 3.135V
    CHECK_FALSE(bod.pending_interrupt_request(req));

    // Drop to 3.0V → below 3.135V
    bod.update_vdd(3.0, 3.3);
    CHECK(bod.pending_interrupt_request(req));
    CHECK((bod.read(D.status_address) & 0x01U) != 0U);

    // Rise back above
    bod.consume_interrupt_request(req);
    bod.write(D.intctrl_address, 0x03U); // VLMCFG=rising
    bod.update_vdd(3.3, 3.3);
    CHECK(bod.pending_interrupt_request(req));
    CHECK((bod.read(D.status_address) & 0x01U) == 0U);
}

TEST_CASE("BodCtrl — VLMIF persists after consume when IE disabled") {
    BodCtrl bod{D};
    bod.reset();

    bod.write(D.vlmctrla_address, 0x00U);
    bod.write(D.intctrl_address, 0x00U); // VLMIE=0 (no HW interrupt)

    bod.update_vdd(5.0, 5.0); // Init above
    bod.update_vdd(1.0, 5.0); // Fall below

    // INTFLAGS should be set even without IE
    CHECK((bod.read(D.intflags_address) & 0x01U) != 0U);

    // No pending_interrupt_request (IE disabled)
    InterruptRequest req{};
    CHECK_FALSE(bod.pending_interrupt_request(req));

    // But VLMIF still set until explicitly cleared
    CHECK((bod.read(D.intflags_address) & 0x01U) != 0U);
}

TEST_CASE("BodCtrl — Multiple crossings with VLMCFG=both") {
    BodCtrl bod{D};
    bod.reset();

    bod.write(D.vlmctrla_address, 0x00U);
    bod.write(D.intctrl_address, 0x05U); // VLMIE=1, VLMCFG=both

    // Sequence: above → below → above → below
    bod.update_vdd(5.0, 5.0); // Init above (no trigger since prev_below_ = false, below = false)

    InterruptRequest req{};
    CHECK_FALSE(bod.pending_interrupt_request(req));

    bod.update_vdd(1.0, 5.0); // Falling edge → trigger
    CHECK(bod.consume_interrupt_request(req));

    bod.update_vdd(5.0, 5.0); // Rising edge → trigger
    CHECK(bod.consume_interrupt_request(req));

    bod.update_vdd(1.0, 5.0); // Falling edge → trigger
    CHECK(bod.consume_interrupt_request(req));
}
