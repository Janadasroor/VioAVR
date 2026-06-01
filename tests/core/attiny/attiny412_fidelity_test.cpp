#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/devices/attiny412.hpp"

using namespace vioavr::core;

TEST_CASE("ATtiny412 — GPIO DIRSET/CLR/TGL aliases") {
    PinMux pm{6};
    GpioPort port{devices::attiny412.ports[0], pm};
    port.reset();

    const auto& desc = devices::attiny412.ports[0];

    port.write(desc.dirset_address, 0xAAU);
    CHECK(port.read(desc.ddr_address) == 0xAAU);

    port.write(desc.dirclr_address, 0x0AU);
    CHECK(port.read(desc.ddr_address) == 0xA0U);

    port.write(desc.dirtgl_address, 0xFFU);
    CHECK(port.read(desc.ddr_address) == 0x5FU);
}

TEST_CASE("ATtiny412 — GPIO OUTSET/CLR/TGL") {
    PinMux pm{6};
    GpioPort port{devices::attiny412.ports[0], pm};
    port.reset();

    const auto& desc = devices::attiny412.ports[0];

    port.write(desc.outset_address, 0x33U);
    CHECK(port.read(desc.port_address) == 0x33U);

    port.write(desc.outclr_address, 0x03U);
    CHECK(port.read(desc.port_address) == 0x30U);

    port.write(desc.outtgl_address, 0xF0U);
    CHECK(port.read(desc.port_address) == 0xC0U);
}

TEST_CASE("ATtiny412 — TCA basic PWM via full Machine") {
    auto machine = Machine::create_for_device("ATtiny412");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();
    auto& mux = machine->pin_mux();

    const auto& tca_desc = devices::attiny412.timers_tca[0];

    bus.write_data(tca_desc.ctrla_address, 0x01U);
    bus.write_data(tca_desc.period_address, 0xFFU);
    bus.write_data(tca_desc.period_address + 1, 0x00U);
    bus.write_data(tca_desc.cmp0_address, 0x40U);
    bus.write_data(tca_desc.cmp0_address + 1, 0x00U);
    bus.write_data(tca_desc.ctrlb_address, 0x10U);

    bus.tick_peripherals(512);

    // ATtiny412: WO0 maps to PA3 (tca_wo_pin_bit[0] = 3)
    auto state = mux.get_state(0, 3);
    CHECK(state.owner == PinOwner::timer);
}
