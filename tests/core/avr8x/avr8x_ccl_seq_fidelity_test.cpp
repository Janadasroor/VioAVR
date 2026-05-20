#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/ccl.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X CCL - D Flip-Flop Fidelity") {
    auto device = DeviceCatalog::find("ATmega4809");
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    auto* ccl = machine.peripherals_of_type<Ccl>()[0];

    // LUT1 (Input 0 for SEQ0) -> D input
    bus.write_data(0x01CF, 0xAA); // Buffer
    bus.write_data(0x01CD, 0x05); // IO
    bus.write_data(0x01CC, 0x01); // Enable

    // LUT0 (Input 1 for SEQ0) -> CLK input
    bus.write_data(0x01CB, 0xAA); // Buffer
    bus.write_data(0x01C9, 0x05); // IO
    bus.write_data(0x01C8, 0x01); // Enable

    // SEQ0: D-FF Mode (0x01)
    bus.write_data(0x01C1, 0x01);
    bus.write_data(0x01C0, 0x01); // CCL Enable

    // Initial Q=0
    CHECK(ccl->get_seq_output(0) == false);

    // D=1, CLK=0
    ccl->set_pin_input(1, 0, true); // LUT1 IN0 = D
    ccl->set_pin_input(0, 0, false); // LUT0 IN0 = CLK
    CHECK(ccl->get_seq_output(0) == false);

    // CLK -> 1 (Rising edge)
    ccl->set_pin_input(0, 0, true);
    CHECK(ccl->get_seq_output(0) == true);

    // D=0, CLK=1
    ccl->set_pin_input(1, 0, false);
    CHECK(ccl->get_seq_output(0) == true); // Should hold

    // CLK -> 0
    ccl->set_pin_input(0, 0, false);
    CHECK(ccl->get_seq_output(0) == true);

    // CLK -> 1 (Rising edge, D=0)
    ccl->set_pin_input(0, 0, true);
    CHECK(ccl->get_seq_output(0) == false);
}

TEST_CASE("AVR8X CCL - JK Flip-Flop Fidelity") {
    auto device = DeviceCatalog::find("ATmega4809");
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    auto* ccl = machine.peripherals_of_type<Ccl>()[0];

    // LUT1 -> J (In0)
    bus.write_data(0x01CF, 0xAA); bus.write_data(0x01CD, 0x05); bus.write_data(0x01CC, 0x01);
    // LUT0 -> K (In1)
    bus.write_data(0x01CB, 0xAA); bus.write_data(0x01C9, 0x05); bus.write_data(0x01C8, 0x01);
    
    // CLK -> IN2 of LUT0 (0x05)
    bus.write_data(0x01CA, 0x05); // LUT0 INSEL2=IO

    // SEQ0: JK-FF Mode (0x02)
    bus.write_data(0x01C1, 0x02);
    bus.write_data(0x01C0, 0x01); 

    // J=1, K=0
    ccl->set_pin_input(1, 0, true);
    ccl->set_pin_input(0, 0, false);
    
    // Toggle CLK (rising edge)
    ccl->set_pin_input(0, 2, false);
    ccl->set_pin_input(0, 2, true);
    CHECK(ccl->get_seq_output(0) == true);

    // J=1, K=1 (Toggle mode)
    ccl->set_pin_input(0, 0, true);
    ccl->set_pin_input(0, 2, false);
    ccl->set_pin_input(0, 2, true);
    CHECK(ccl->get_seq_output(0) == false); // Toggled from 1 to 0
}
