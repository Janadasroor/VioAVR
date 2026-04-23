#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;

TEST_CASE("GpioPort (AVR8X) — Reset defaults")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm}; // PORTA
    port.reset();

    const auto& desc = devices::atmega4809.ports[0];
    CHECK(port.read(desc.ddr_address) == 0U);
    CHECK(port.read(desc.port_address) == 0U);
    CHECK(port.read(desc.intflags_address) == 0U);
    for(u8 i=0; i<8; ++i) {
        CHECK(port.read(desc.pin_ctrl_base + i) == 0U);
    }
}

TEST_CASE("GpioPort (AVR8X) — PIN_xCTRL PULLUPEN")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();

    const auto& desc = devices::atmega4809.ports[0];
    
    // Enable pull-up on pin 0
    port.write(desc.pin_ctrl_base + 0, 0x08U); // PULLUPEN=1
    
    auto state = pm.get_state(0, 0); // Port A, Pin 0
    CHECK_FALSE(state.is_output);
    CHECK(state.pullup_enabled);
}

TEST_CASE("GpioPort (AVR8X) — PIN_xCTRL INVEN (Output)")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();

    const auto& desc = devices::atmega4809.ports[0];
    
    // Set as output
    port.write(desc.ddr_address, 0x01U);
    port.write(desc.port_address, 0x01U); // Output High
    
    auto state = pm.get_state(0, 0);
    CHECK(state.drive_level);
    
    // Enable inversion
    port.write(desc.pin_ctrl_base + 0, 0x80U); // INVEN=1
    state = pm.get_state(0, 0);
    CHECK_FALSE(state.drive_level); // Should be inverted
}

TEST_CASE("GpioPort (AVR8X) — PIN_xCTRL INVEN (Input)")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();

    const auto& desc = devices::atmega4809.ports[0];
    
    port.set_input_levels(0x01U); // PA0 High
    CHECK((port.read(desc.pin_address) & 0x01U) != 0U);
    
    // Enable inversion
    port.write(desc.pin_ctrl_base + 0, 0x80U); // INVEN=1
    CHECK((port.read(desc.pin_address) & 0x01U) == 0U); // Should read low
}

TEST_CASE("GpioPort (AVR8X) — PIN_xCTRL ISC=4 (INPUT_DISABLE)")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();

    const auto& desc = devices::atmega4809.ports[0];
    
    port.set_input_levels(0x01U); // PA0 High
    CHECK((port.read(desc.pin_address) & 0x01U) != 0U);
    
    // Disable input buffer
    port.write(desc.pin_ctrl_base + 0, 0x04U); // ISC=4
    CHECK((port.read(desc.pin_address) & 0x01U) == 0U); // Should read low regardless of input
}

TEST_CASE("GpioPort (AVR8X) — Interrupt triggering (RISING)")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();

    const auto& desc = devices::atmega4809.ports[0];
    
    // Configure PA0 for RISING interrupt
    port.write(desc.pin_ctrl_base + 0, 0x02U); // ISC=2
    
    InterruptRequest req;
    CHECK_FALSE(port.pending_interrupt_request(req));
    
    // Toggle PA0 Low -> High
    (void)port.on_external_pin_change(0, PinLevel::low);
    (void)port.on_external_pin_change(0, PinLevel::high);
    
    CHECK(port.pending_interrupt_request(req));
    CHECK(req.vector_index == desc.vector_index);
    CHECK((port.read(desc.intflags_address) & 0x01U) != 0U);
    
    // Clear flag
    port.write(desc.intflags_address, 0x01U);
    CHECK_FALSE(port.pending_interrupt_request(req));
    CHECK(port.read(desc.intflags_address) == 0U);
}

TEST_CASE("GpioPort (AVR8X) — Interrupt triggering (BOTHEDGES)")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();

    const auto& desc = devices::atmega4809.ports[0];
    
    // Configure PA0 for BOTHEDGES interrupt
    port.write(desc.pin_ctrl_base + 0, 0x01U); // ISC=1
    
    // Low -> High
    (void)port.on_external_pin_change(0, PinLevel::high);
    CHECK((port.read(desc.intflags_address) & 0x01U) != 0U);
    port.write(desc.intflags_address, 0x01U);
    
    // High -> Low
    (void)port.on_external_pin_change(0, PinLevel::low);
    CHECK((port.read(desc.intflags_address) & 0x01U) != 0U);
}

TEST_CASE("GpioPort (AVR8X) — Aliases (DIRSET/CLR/TGL, OUTSET/CLR/TGL)")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();

    const auto& desc = devices::atmega4809.ports[0];
    
    // DIRSET
    port.write(desc.dirset_address, 0xAAU);
    CHECK(port.read(desc.ddr_address) == 0xAAU);
    
    // DIRCLR
    port.write(desc.dirclr_address, 0x0AU);
    CHECK(port.read(desc.ddr_address) == 0xA0U);
    
    // DIRTGL
    port.write(desc.dirtgl_address, 0xFFU);
    CHECK(port.read(desc.ddr_address) == 0x5FU);
    
    // OUTSET
    port.write(desc.outset_address, 0x33U);
    CHECK(port.read(desc.port_address) == 0x33U);
    
    // OUTCLR
    port.write(desc.outclr_address, 0x03U);
    CHECK(port.read(desc.port_address) == 0x30U);
    
    // OUTTGL
    port.write(desc.outtgl_address, 0xF0U);
    CHECK(port.read(desc.port_address) == 0xC0U);
}

TEST_CASE("GpioPort (AVR8X) — VPORT access")
{
    PinMux pm{6};
    GpioPort port{devices::atmega4809.ports[0], pm};
    port.reset();

    const auto& desc = devices::atmega4809.ports[0];
    const u16 vbase = desc.vport_base;
    
    // VPORT DIR
    port.write(vbase + 0, 0x55U);
    CHECK(port.read(desc.ddr_address) == 0x55U);
    CHECK(port.read(vbase + 0) == 0x55U);
    
    // VPORT OUT
    port.write(vbase + 1, 0xAAU);
    CHECK(port.read(desc.port_address) == 0xAAU);
    CHECK(port.read(vbase + 1) == 0xAAU);
    
    // VPORT IN (read) — set all pins as inputs first for clean test
    port.write(vbase + 0, 0x00U); // DDR = all inputs
    port.set_input_levels(0x0FU);
    CHECK(port.read(vbase + 2) == 0x0FU);
    
    // Restore DDR for toggle test
    port.write(vbase + 0, 0x55U);
    port.write(vbase + 1, 0xAAU);
    
    // VPORT IN (write) -> OUT toggle
    port.write(vbase + 2, 0xFFU);
    CHECK(port.read(desc.port_address) == 0x55U);
    
    // VPORT INTFLAGS
    (void)port.on_external_pin_change(0, PinLevel::high); // ISC=0 still? Wait default ISC is 0.
    port.write(desc.pin_ctrl_base + 0, 0x01U); // BOTHEDGES
    (void)port.on_external_pin_change(0, PinLevel::low);
    
    CHECK((port.read(vbase + 3) & 0x01U) != 0U);
    
    // Clear via VPORT
    port.write(vbase + 3, 0x01U);
    CHECK(port.read(vbase + 3) == 0U);
}
