#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/viospice.hpp"
#include "vioavr/core/device_catalog.hpp"

TEST_CASE("VioSpice exposes Timer2 async ticks")
{
    using vioavr::core::DeviceCatalog;
    using vioavr::core::VioSpice;

    const auto* device = DeviceCatalog::find("ATmega328P");
    REQUIRE(device != nullptr);

    VioSpice spice {*device};
    auto& bus = spice.bus();

    bus.write_data(device->timer2.assr_address, 0x20U);  // AS2
    bus.write_data(device->timer2.tccrb_address, 0x01U); // clk/1

    spice.step_duration(1e-6);
    CHECK(bus.read_data(device->timer2.tcnt_address) == 0U);

    spice.tick_timer2_async(4U);
    CHECK(bus.read_data(device->timer2.tcnt_address) == 4U);
}
