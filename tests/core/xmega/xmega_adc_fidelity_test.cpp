#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/xmegadc.hpp"

using namespace vioavr::core;

TEST_CASE("XMEGA ADC — Conversion timing") {
    auto device = DeviceCatalog::find("ATxmega128A1");
    REQUIRE(device != nullptr);
    REQUIRE(device->adc_xmega_count >= 1);

    XmegaAdc adc(device->adcs_xmega[0]);
    const auto& d = device->adcs_xmega[0];

    // Enable, prescaler=0 (DIV4, divider=16)
    adc.write(d.ctrla_address, 0x01U);
    adc.write(d.prescaler_address, 0x00U);
    adc.write(d.ctrla_address, 0x41U); // STCONV

    // 12-bit * DIV4 = 21 * 4 = 84 cycles. Check before completion.
    adc.tick(50);
    CHECK((adc.read(d.intflags_address) & 0x01U) == 0);

    adc.tick(50);
    CHECK((adc.read(d.intflags_address) & 0x01U) != 0);

    u8 low = adc.read(d.ch0res_address);
    u8 high = adc.read(d.ch0res_address + 1);
    CHECK((static_cast<u16>(high) << 8 | low) <= 4095);
}

TEST_CASE("XMEGA ADC — Register access") {
    auto device = DeviceCatalog::find("ATxmega128A1");
    REQUIRE(device != nullptr);
    XmegaAdc adc(device->adcs_xmega[0]);
    const auto& d = device->adcs_xmega[0];

    CHECK(adc.read(d.ctrla_address) == 0);
    adc.write(d.ctrla_address, 0x03U);
    CHECK(adc.read(d.ctrla_address) == 0x03U);

    adc.write(d.prescaler_address, 0x07U);
    CHECK(adc.read(d.prescaler_address) == 0x07U);

    adc.write(d.cmp_address, 0x34U);
    adc.write(d.cmp_address + 1, 0x12U);
    CHECK(adc.read(d.cmp_address) == 0x34U);
    CHECK(adc.read(d.cmp_address + 1) == 0x12U);
}

TEST_CASE("XMEGA ADC — Free-running") {
    auto device = DeviceCatalog::find("ATxmega128A1");
    REQUIRE(device != nullptr);
    XmegaAdc adc(device->adcs_xmega[0]);
    const auto& d = device->adcs_xmega[0];

    adc.write(d.ctrla_address, 0x03U); // enable + free-run
    adc.write(d.prescaler_address, 0x00U);
    adc.write(d.ctrla_address, 0x43U); // also start
    adc.tick(200);
    CHECK((adc.read(d.intflags_address) & 0x01U) != 0);

    // Clear flag by writing 1 to INTFLAGS bit 0
    adc.write(d.intflags_address, 0x01U);
    CHECK((adc.read(d.intflags_address) & 0x01U) == 0);
}

TEST_CASE("XMEGA ADC — Compare") {
    auto device = DeviceCatalog::find("ATxmega128A1");
    REQUIRE(device != nullptr);
    XmegaAdc adc(device->adcs_xmega[0]);
    const auto& d = device->adcs_xmega[0];

    adc.write(d.ctrla_address, 0x01U);
    adc.write(d.prescaler_address, 0x00U);
    adc.write(d.cmp_address, 0x00U);
    adc.write(d.cmp_address + 1, 0x00U);
    adc.write(d.ctrla_address, 0x41U);
    adc.tick(500);

    // CMP = 0, result >= 0 always → CMPIF should fire
    CHECK((adc.read(d.intflags_address) & 0x10U) != 0);
}

TEST_CASE("XMEGA ADC — Interrupt request") {
    auto device = DeviceCatalog::find("ATxmega128A1");
    REQUIRE(device != nullptr);
    XmegaAdc adc(device->adcs_xmega[0]);
    const auto& d = device->adcs_xmega[0];

    adc.write(d.ctrla_address, 0x01U);
    adc.write(d.prescaler_address, 0x00U);
    adc.write(d.ctrla_address, 0x41U);
    adc.tick(500);

    InterruptRequest req;
    CHECK(adc.pending_interrupt_request(req));
    CHECK(req.vector_index == d.vector_index);

    CHECK(adc.consume_interrupt_request(req));
    CHECK_FALSE(adc.pending_interrupt_request(req));
}

TEST_CASE("XMEGA ADC — CMP and CH0 interrupt independently") {
    auto device = DeviceCatalog::find("ATxmega128A1");
    REQUIRE(device != nullptr);
    XmegaAdc adc(device->adcs_xmega[0]);
    const auto& d = device->adcs_xmega[0];

    // Enable only, no free-run
    adc.write(d.ctrla_address, 0x01U);
    adc.write(d.prescaler_address, 0x00U);
    adc.write(d.cmp_address, 0x00U);
    adc.write(d.cmp_address + 1, 0x00U); // CMP=0, result always >= 0
    adc.write(d.ctrla_address, 0x41U); // STCONV
    adc.tick(500);

    // Both CH0IF and CMPIF should be set
    CHECK((adc.read(d.intflags_address) & 0x01U) != 0);
    CHECK((adc.read(d.intflags_address) & 0x10U) != 0);
}
