#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/ccl.hpp"
#include <vector>

using namespace vioavr::core;

TEST_CASE("AVR8X CCL - LUT Logic Fidelity") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // Find CCL
    auto ccl_list = machine.peripherals_of_type<Ccl>();
    REQUIRE(!ccl_list.empty());
    auto* ccl = ccl_list[0];

    // ATmega4809 CCL Config:
    // LUT0 TRUTH at 0x0128
    // LUT0 CTRLA at 0x0124 (ENABLE, FILTSEL)
    // LUT0 CTRLB at 0x0125 (INSEL1, INSEL0)
    // LUT0 CTRLC at 0x0126 (INSEL2)
    // CCL CTRLA at 0x0120 (ENABLE)

    const u16 ccl_ctrla = 0x01C0;
    const u16 lut0_ctrla = 0x01C8;
    const u16 lut0_ctrlb = 0x01C9;
    const u16 lut0_truth = 0x01CB;

    // 1. Configure LUT0 as AND Gate (Truth Table: 0x80 for 3-input AND)
    // Inputs: IN0=1, IN1=1, IN2=1 -> Output=1. pattern 111 (7) -> bit 7 = 1.
    // However, if we only use 2 inputs (IN0, IN1) and MASK IN2:
    // Pattern 011 (3) -> bit 3 = 1. Pattern 00x, 0x0 -> bit = 0.
    // Truth table for 2-input AND (IN0, IN1) where IN2 is 0 or ignored:
    // 000 -> 0 (bit 0)
    // 001 -> 0 (bit 1, IN0=1)
    // 010 -> 0 (bit 2, IN1=1)
    // 011 -> 1 (bit 3, IN0=1, IN1=1)
    // Result: 0x08.
    
    bus.write_data(lut0_truth, 0x08);
    
    // 2. Configure INSELs
    // INSEL0 = IO (0x05), INSEL1 = IO (0x05), INSEL2 = MASK (0x00)
    bus.write_data(lut0_ctrlb, 0x55); 
    
    // 3. Enable Peripheral and LUT
    bus.write_data(ccl_ctrla, 0x01);
    bus.write_data(lut0_ctrla, 0x01);
    
    // 4. Test Logic via set_pin_input
    // Initially output should be 0
    CHECK(ccl->get_lut_output(0) == false);
    
    // IN0 = 1
    ccl->set_pin_input(0, 0, true);
    CHECK(ccl->get_lut_output(0) == false);
    
    // IN1 = 1 -> AND gate fulfills
    ccl->set_pin_input(0, 1, true);
    CHECK(ccl->get_lut_output(0) == true);
    
    // IN0 = 0 -> AND gate drops
    ccl->set_pin_input(0, 0, false);
    CHECK(ccl->get_lut_output(0) == false);
}

TEST_CASE("AVR8X CCL - LUT Linkage Fidelity") {
    auto device = DeviceCatalog::find("ATmega4809");
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    auto* ccl = machine.peripherals_of_type<Ccl>()[0];

    // LUT0 Truth: Address 0x1CB
    bus.write_data(0x01CB, 0x55); 
    bus.write_data(0x01C9, 0x05); // LUT0 INSEL0=IO (0x05)
    bus.write_data(0x01C8, 0x01); // LUT0 Enable
    
    // LUT1: Buffer of LINK (LUT0 output)
    // LUT1 Truth: Address 0x1CF, CTRLB: 0x1CD, CTRLA: 0x1CC
    bus.write_data(0x01CF, 0xAA); 
    bus.write_data(0x01CD, 0x02); // LUT1 INSEL0=LINK (0x02)
    bus.write_data(0x01CC, 0x01); // LUT1 Enable
    
    bus.write_data(0x01C0, 0x01); // CCL Enable
    
    // IN0 of LUT0 = 0 -> LUT0 = 1 -> LUT1 = 1
    ccl->set_pin_input(0, 0, false);
    CHECK(ccl->get_lut_output(0) == true);
    CHECK(ccl->get_lut_output(1) == true);
    
    // IN0 of LUT0 = 1 -> LUT0 = 0 -> LUT1 = 0
    ccl->set_pin_input(0, 0, true);
    CHECK(ccl->get_lut_output(0) == false);
    CHECK(ccl->get_lut_output(1) == false);
}
