#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;

TEST_CASE("GPIO — Reset defaults for all registers including VPORT")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    const auto& d = devices::atmega4809.ports[0];
    CHECK(port.read(d.ddr_address) == 0U);
    CHECK(port.read(d.port_address) == 0U);
    CHECK(port.read(d.pin_address) == 0U);
    CHECK(port.read(d.intflags_address) == 0U);
    for (u8 i = 0; i < 8; ++i)
        CHECK(port.read(d.pin_ctrl_base + i) == 0U);
    CHECK(port.read(d.vport_base + 0) == 0U);
    CHECK(port.read(d.vport_base + 1) == 0U);
    CHECK(port.read(d.vport_base + 2) == 0U);
    CHECK(port.read(d.vport_base + 3) == 0U);
}

TEST_CASE("GPIO — DDR register round-trip")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    const auto& d = devices::atmega4809.ports[0];
    port.write(d.ddr_address, 0xA5U);
    CHECK(port.read(d.ddr_address) == 0xA5U);
    port.write(d.ddr_address, 0x5AU);
    CHECK(port.read(d.ddr_address) == 0x5AU);
}

TEST_CASE("GPIO — PORT register round-trip")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    const auto& d = devices::atmega4809.ports[0];
    port.write(d.port_address, 0x5AU);
    CHECK(port.read(d.port_address) == 0x5AU);
    port.write(d.port_address, 0xA5U);
    CHECK(port.read(d.port_address) == 0xA5U);
}

TEST_CASE("GPIO — PIN register read with input levels")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    const auto& d = devices::atmega4809.ports[0];
    port.set_input_levels(0x55U);
    CHECK(port.read(d.pin_address) == 0x55U);
    port.set_input_levels(0xAAU);
    CHECK(port.read(d.pin_address) == 0xAAU);
}

TEST_CASE("GPIO — PIN toggle semantics")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    const auto& d = devices::atmega4809.ports[0];
    port.write(d.port_address, 0x0FU);
    CHECK(port.read(d.port_address) == 0x0FU);
    port.write(d.pin_address, 0x0FU);
    CHECK(port.read(d.port_address) == 0x00U);
    port.write(d.pin_address, 0x0FU);
    CHECK(port.read(d.port_address) == 0x0FU);
}

TEST_CASE("GPIO — PORT output levels calculation")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    const auto& d = devices::atmega4809.ports[0];
    port.write(d.ddr_address, 0xF0U);
    port.write(d.port_address, 0xFFU);
    CHECK(port.output_levels() == 0xF0U);
    port.write(d.ddr_address, 0x0FU);
    CHECK(port.output_levels() == 0x0FU);
}

TEST_CASE("GPIO — ISC=3 FALLING edge interrupt")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    const auto& d = devices::atmega4809.ports[0];
    port.write(d.pin_ctrl_base + 0, 0x03U);
    InterruptRequest req;
    (void)port.on_external_pin_change(d.pin_address, 0, PinLevel::high);
    CHECK_FALSE(port.pending_interrupt_request(req));
    (void)port.on_external_pin_change(d.pin_address, 0, PinLevel::low);
    CHECK(port.pending_interrupt_request(req));
    CHECK((port.read(d.intflags_address) & 0x01U) != 0U);
}

TEST_CASE("GPIO — Multiple pins with different ISC simultaneous")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    const auto& d = devices::atmega4809.ports[0];
    port.write(d.pin_ctrl_base + 0, 0x02U);
    port.write(d.pin_ctrl_base + 1, 0x03U);
    (void)port.on_external_pin_change(d.pin_address, 1, PinLevel::high);
    (void)port.on_external_pin_change(d.pin_address, 0, PinLevel::high);
    (void)port.on_external_pin_change(d.pin_address, 1, PinLevel::low);
    InterruptRequest req;
    CHECK(port.pending_interrupt_request(req));
    CHECK((port.read(d.intflags_address) & 0x03U) == 0x03U);
}

TEST_CASE("GPIO — supports_interrupt_mask returns true")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    CHECK(port.supports_interrupt_mask());
}

TEST_CASE("GPIO — consume_pin_change single pin")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    const auto& d = devices::atmega4809.ports[0];
    port.write(d.ddr_address, 0xFFU);
    port.write(d.port_address, 0x00U);
    port.write(d.port_address, 0x01U);
    PinStateChange ch;
    CHECK(port.consume_pin_change(ch));
    CHECK(ch.bit_index == 0);
    CHECK(ch.level);
    CHECK_FALSE(port.consume_pin_change(ch));
}

TEST_CASE("GPIO — consume_pin_change multiple pins in sequence")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    const auto& d = devices::atmega4809.ports[0];
    port.write(d.ddr_address, 0xFFU);
    port.write(d.port_address, 0x00U);
    port.write(d.port_address, 0xFFU);
    PinStateChange ch;
    for (u8 i = 0; i < 8; ++i) {
        CHECK(port.consume_pin_change(ch));
        CHECK(ch.bit_index == i);
    }
    CHECK_FALSE(port.consume_pin_change(ch));
}

TEST_CASE("GPIO — PIN_CTRL full byte round-trip")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    const auto& d = devices::atmega4809.ports[0];
    port.write(d.pin_ctrl_base + 5, 0x8BU);
    CHECK(port.read(d.pin_ctrl_base + 5) == 0x8BU);
    port.write(d.pin_ctrl_base + 5, 0x00U);
    CHECK(port.read(d.pin_ctrl_base + 5) == 0x00U);
}

TEST_CASE("GPIO — VPORT DIR write + readback")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    const auto& d = devices::atmega4809.ports[0];
    port.write(d.vport_base + 0, 0x55U);
    CHECK(port.read(d.vport_base + 0) == 0x55U);
    CHECK(port.read(d.ddr_address) == 0x55U);
}

TEST_CASE("GPIO — VPORT IN read tied to PIN")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    const auto& d = devices::atmega4809.ports[0];
    port.set_input_levels(0xAAU);
    CHECK(port.read(d.vport_base + 2) == 0xAAU);
    CHECK(port.read(d.pin_address) == port.read(d.vport_base + 2));
}

TEST_CASE("GPIO — VPORT OUT write + readback")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    const auto& d = devices::atmega4809.ports[0];
    port.write(d.vport_base + 1, 0x5AU);
    CHECK(port.read(d.vport_base + 1) == 0x5AU);
    CHECK(port.read(d.port_address) == 0x5AU);
}

TEST_CASE("GPIO — VPORT write to IN toggles PORT bits")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    const auto& d = devices::atmega4809.ports[0];
    port.write(d.port_address, 0x0FU);
    port.write(d.vport_base + 2, 0xFFU);
    CHECK(port.read(d.port_address) == 0xF0U);
}

TEST_CASE("GPIO — INTFLAGS write-1-to-clear from VPORT")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    const auto& d = devices::atmega4809.ports[0];
    port.write(d.pin_ctrl_base + 0, 0x01U);
    (void)port.on_external_pin_change(d.pin_address, 0, PinLevel::high);
    CHECK((port.read(d.intflags_address) & 0x01U) != 0U);
    CHECK((port.read(d.vport_base + 3) & 0x01U) != 0U);
    port.write(d.vport_base + 3, 0x01U);
    CHECK(port.read(d.intflags_address) == 0U);
    CHECK(port.read(d.vport_base + 3) == 0U);
}

TEST_CASE("GPIO — set_input_levels after output config")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    const auto& d = devices::atmega4809.ports[0];
    port.write(d.ddr_address, 0xFFU);
    port.write(d.port_address, 0xFFU);
    port.set_input_levels(0x00U);
    CHECK(port.read(d.pin_address) == 0xFFU);
}

TEST_CASE("GPIO — Unmapped addresses return 0")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    CHECK(port.read(0xFFFF) == 0U);
    CHECK(port.read(0x500) == 0U);
    CHECK(port.read(0x04) == 0U);
}

TEST_CASE("GPIO — INTFLAGS after multiple pins trigger")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();
    const auto& d = devices::atmega4809.ports[0];
    port.write(d.pin_ctrl_base + 0, 0x02U);
    port.write(d.pin_ctrl_base + 2, 0x02U);
    port.write(d.pin_ctrl_base + 4, 0x01U);
    (void)port.on_external_pin_change(d.pin_address, 0, PinLevel::high);
    (void)port.on_external_pin_change(d.pin_address, 2, PinLevel::high);
    (void)port.on_external_pin_change(d.pin_address, 4, PinLevel::high);
    InterruptRequest req;
    CHECK(port.pending_interrupt_request(req));
    CHECK((port.read(d.intflags_address) & 0x15U) == 0x15U);
    port.write(d.intflags_address, 0x15U);
    CHECK(port.read(d.intflags_address) == 0U);
    CHECK_FALSE(port.pending_interrupt_request(req));
}
