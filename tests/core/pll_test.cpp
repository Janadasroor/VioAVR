#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/pll.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega32u4.hpp"

namespace {
using namespace vioavr::core;
using namespace vioavr::core::devices;
}

TEST_CASE("PLL Lock Simulation")
{
    PllController pll {atmega32u4.usbs[0].pllcsr_address};
    MemoryBus bus {atmega32u4};
    bus.attach_peripheral(pll);

    // Initial state
    CHECK((bus.read_data(0x49) & 0x01) == 0);

    // Enable PLL
    bus.write_data(0x49, 0x02); // PLLE
    CHECK((bus.read_data(0x49) & 0x02) != 0);
    CHECK((bus.read_data(0x49) & 0x01) == 0); // Still locking

    // Tick half way
    pll.tick(500);
    CHECK((bus.read_data(0x49) & 0x01) == 0);

    // Tick till lock
    pll.tick(501);
    CHECK((bus.read_data(0x49) & 0x01) != 0); // LOCKED!

    // Disable PLL
    bus.write_data(0x49, 0x00);
    CHECK((bus.read_data(0x49) & 0x01) == 0);
}
