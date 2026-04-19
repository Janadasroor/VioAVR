#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

TEST_CASE("AVR8X NVMCTRL - EEPROM Fidelity") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    // 4809 NVMCTRL: CTRLA=0x1000, ADDR=0x1006, EEPROM=0x1400
    const u16 NVM_CTRLA = 0x1000;
    const u16 NVM_ADDR  = 0x1006;
    const u16 NVM_STATUS = 0x1002;
    const u16 NVM_INTFLAGS = 0x1004;
    const u16 EEPROM_START = 0x1400;

    // 1. Initial State: EEPROM should be 0xFF
    CHECK(bus.read_data(EEPROM_START) == 0xFFU);

    // Load an infinite loop to keep the CPU running
    std::vector<u16> program = {0xCFFF}; // RJMP -1
    bus.load_flash(program);
    machine.cpu().reset();

    // 2. Write to EEPROM range (hits page buffer, not storage)
    bus.write_data(EEPROM_START, 0x42U);
    
    // Read back should STILL be 0xFF (Unified Map reads from storage)
    CHECK(bus.read_data(EEPROM_START) == 0xFFU);

    // 3. Set up NVM Command
    // Set ADDR to point to the EEPROM location (relative to data space)
    bus.write_data(NVM_ADDR, (EEPROM_START & 0xFFU));
    bus.write_data(NVM_ADDR + 1, (EEPROM_START >> 8U));

    // Execute ERASEWRITE (0x08)
    bus.write_data(NVM_CTRLA, 0x08U);

    // 4. Verify Busy Status
    // STATUS bit 1 is EEBUSY
    CHECK((bus.read_data(NVM_STATUS) & 0x02U) != 0U);

    // 5. Progress Simulation
    machine.run(32000);
    CHECK((bus.read_data(NVM_STATUS) & 0x02U) != 0U); // Still busy

    machine.run(33000); // Crosses the 64000 mark
    
    // 6. Verify Completion
    CHECK((bus.read_data(NVM_STATUS) & 0x02U) == 0U); // Not busy
    CHECK((bus.read_data(NVM_INTFLAGS) & 0x01U) != 0U); // DONE flag set

    // 7. Verify Data in Storage
    CHECK(bus.read_data(EEPROM_START) == 0x42U);
}

TEST_CASE("AVR8X NVMCTRL - Page Buffer Clear") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    const u16 NVM_CTRLA = 0x1000;
    const u16 EEPROM_START = 0x1400;

    // Load an infinite loop
    std::vector<u16> program = {0xCFFF}; // RJMP -1
    bus.load_flash(program);
    machine.cpu().reset();

    // 1. Fill buffer
    bus.write_data(EEPROM_START, 0xAAU);

    // 2. Clear buffer command (0x09 - EEPBC)
    bus.write_data(NVM_CTRLA, 0x09U);
    machine.run(10);

    // 3. Command EEPROM write
    bus.write_data(0x1006, (EEPROM_START & 0xFFU)); // ADDR L
    bus.write_data(0x1007, (EEPROM_START >> 8U));   // ADDR H
    bus.write_data(NVM_CTRLA, 0x08U); // ERASEWRITE
    
    machine.run(70000);

    // 4. Should be 0xFF (buffer was cleared)
    CHECK(bus.read_data(EEPROM_START) == 0xFFU);
}
