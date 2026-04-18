#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/memory_bus.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X TWI Fidelity Test") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    REQUIRE(device->twi8x_count >= 1);

    Machine machine(*device);
    auto& bus = machine.bus();

    const u16 TWI0_BASE = 0x08A0;
    const u16 TWI0_MCTRLA   = TWI0_BASE + 0x03;
    const u16 TWI0_MSTATUS  = TWI0_BASE + 0x05;
    const u16 TWI0_MADDR    = TWI0_BASE + 0x07;
    const u16 TWI0_MDATA    = TWI0_BASE + 0x08;

    // 1. Enable Host
    bus.write_data(TWI0_MCTRLA, 0x01); // ENABLE=1
    CHECK((bus.read_data(TWI0_MSTATUS) & 0x03) == 0x00); // Unknown state initially

    // 2. Start Write Transaction (Address 0x20, Write bit 0)
    bus.write_data(TWI0_MADDR, 0x20 << 1); 
    
    // Check State: OWNER (0x02) and WIF (0x40)
    u8 status = bus.read_data(TWI0_MSTATUS);
    CHECK((status & 0x03) == 0x02); // OWNER
    CHECK((status & 0x40) == 0x40); // WIF
    CHECK((status & 0x20) == 0x20); // CLKHOLD

    // 3. Write Data
    bus.write_data(TWI0_MDATA, 0xAB);
    status = bus.read_data(TWI0_MSTATUS);
    CHECK((status & 0x20) == 0);    // CLKHOLD released
    CHECK((status & 0x40) == 0x40); // WIF set for next byte
}
