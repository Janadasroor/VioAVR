#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/nvm_ctrl.hpp"
#include "vioavr/core/eeprom.hpp"
#include <vector>

using namespace vioavr::core;

TEST_CASE("AVR8X NVM - EEPROM Page Buffering Fidelity") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // Load NOPs
    std::vector<u16> program(1000, 0x0000);
    bus.load_flash(program);
    machine.cpu().reset();

    // 1. Write to EEPROM Mapped Address (0x1400)
    // This should NOT change the physical EEPROM immediately, but go to Page Buffer.
    bus.write_data(0x1400, 0xAA);
    
    // Read back immediately - should be 0xAA (because we read back from the same memory map? 
    // Actually datasheet: "Reading from the EEPROM mapped space returns the physical EEPROM content."
    // Wait! If I write to 0x1400, it's just a buffer. Reading from 0x1400 should return OLD value (0xFF).
    CHECK(bus.read_data(0x1400) == 0xFF); // Fails if we write direct!

    // 2. Clear Page Buffer command
    // CTRLA address for NVMCTRL is 0x1000
    bus.write_data(0x1000, 0x09); // EEBUFCLR
    // MemoryBus should process it immediately (cycles = 1)
    machine.run(1);

    // 3. Write to Page Buffer again
    bus.write_data(0x1400, 0xBB);
    CHECK(bus.read_data(0x1400) == 0xFF);

    // 4. Execute ERWEE (Erase and Write EEPROM)
    bus.write_data(0x1000, 0x08); // ee_erase_write (0x08 in our enum)
    
    // Check Status: BUSY
    CHECK((bus.read_data(0x1002) & 0x03) != 0);

    // 5. Wait for completion (~4ms = 64000 cycles at 16MHz)
    machine.run(65000);
    
    // Check Status: Not BUSY
    CHECK((bus.read_data(0x1002) & 0x03) == 0);
    
    // Now physical EEPROM should have 0xBB
    CHECK(bus.read_data(0x1400) == 0xBB);
}
