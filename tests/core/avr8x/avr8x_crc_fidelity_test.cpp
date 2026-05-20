#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/memory_bus.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X CRC Fidelity Test") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    REQUIRE(device->crc8x_count >= 1);

    Machine machine(*device);
    auto& bus = machine.bus();

    const u16 CRC_BASE = 0x0120;
    const u16 CRC_CTRLA = CRC_BASE + 0x00;
    const u16 CRC_STATUS = CRC_BASE + 0x02;

    // 1. Check initial state
    CHECK((bus.read_data(CRC_STATUS) & 0x01) == 0); // Not busy

    // 2. Start Scan
    bus.write_data(CRC_CTRLA, 0x01); // ENABLE
    
    // 3. Check Busy
    CHECK((bus.read_data(CRC_STATUS) & 0x01) == 0x01); // BUSY
    
    // 4. Wait for completion (directly tick peripherals)
    bus.tick_peripherals(device->flash_words * 2); 
    
    // 5. Check OK
    u8 status = bus.read_data(CRC_STATUS);
    CHECK((status & 0x01) == 0);    // Not busy
    CHECK((status & 0x02) == 0x02); // OK flag set
}
