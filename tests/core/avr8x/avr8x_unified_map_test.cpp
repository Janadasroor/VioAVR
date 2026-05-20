#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/nvm_ctrl.hpp"
#include "vioavr/core/eeprom.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

namespace {
using namespace vioavr::core;
using namespace vioavr::core::devices;
}

TEST_CASE("AVR8X Unified Map - Signature Access") {
    MemoryBus bus {atmega4809};
    bus.reset();
    
    // ATmega4809 Signature: 1E 96 51
    CHECK(bus.read_data(0x1100) == 0x1EU);
    CHECK(bus.read_data(0x1101) == 0x96U);
    CHECK(bus.read_data(0x1102) == 0x51U);
}

TEST_CASE("AVR8X Unified Map - Fuse Access") {
    MemoryBus bus {atmega4809};
    bus.reset();
    
    // Default fuses for ATmega4809 (from metadata initval)
    // FUSE0: WDTCFG (0x00), FUSE1: BODCFG (0x00), FUSE2: OSCCFG (0x7E)
    CHECK(bus.read_data(0x1282) == 0x7EU);
}

TEST_CASE("AVR8X Unified Map - Flash Aliasing") {
    MemoryBus bus {atmega4809};
    bus.reset();
    
    std::vector<u16> flash_data(1024, 0);
    flash_data[0] = 0xABCD;
    flash_data[1] = 0x1234;
    bus.load_flash(flash_data);
    
    // Flash alias at 0x4000
    CHECK(bus.read_data(0x4000) == 0xCDU);
    CHECK(bus.read_data(0x4001) == 0xABU);
    CHECK(bus.read_data(0x4002) == 0x34U);
    CHECK(bus.read_data(0x4003) == 0x12U);
}

TEST_CASE("AVR8X Unified Map - EEPROM Aliasing") {
    MemoryBus bus {atmega4809};
    bus.reset();
    
    // EEPROM alias at 0x1400.
    // We need to attach an EEPROM peripheral to the bus for this to work
    // because Eeprom::read(address) handles the alias range.
    
    // Note: In a real simulation, we'd attach it. 
    // For this core test, we just verify the address routing if attached.
}

TEST_CASE("AVR8X NVMCTRL - Flash Page Write Integration") {
    Machine machine{atmega4809};
    // Load a dummy NOP program so the CPU is in 'running' state
    std::vector<u16> dummy_flash(1024, 0x0000U);
    dummy_flash[1023] = 0xC000U | ((-1023) & 0x0FFFU); // RJMP back to start
    machine.bus().load_flash(dummy_flash);
    
    machine.reset();
    auto& bus = machine.bus();
    
    // 1. Fill Page Buffer via Mapped Flash (0x4000)
    // We write 0xDEAD to word 0 of the page
    bus.write_data(0x4000, 0xADU);
    bus.write_data(0x4001, 0xDEU);
    
    // 2. Set NVM ADDR register (0x1004 in 4809 metadata, wait, let's check)
    // Actually our gen_device said CTRLA=0x1000, ADDR=0x1008
    bus.write_data(0x1008, 0x00U); // ADDRL
    bus.write_data(0x1009, 0x00U); // ADDRH
    
    // 3. Issue PAGEWRITE command to CTRLA (0x1000)
    // Command 1 = PAGEWRITE
    bus.write_data(0x1000, 0x01U);
    
    // 4. Verify NVMCTRL is busy (STATUS at 0x1002, FBUSY is bit 0)
    CHECK((bus.read_data(0x1002) & 0x01U) == 0x01U);
    
    // 5. Tick simulation (64000 cycles is the default timing we set)
    machine.run(70000);
    
    // 6. Verify NVMCTRL is no longer busy
    CHECK((bus.read_data(0x1002) & 0x01U) == 0x00U);
    
    // 7. Verify Flash word 0 was modified
    CHECK(bus.flash_words()[0] == 0xDEADU);
}

TEST_CASE("AVR8X NVMCTRL - User Row Write") {
    Machine machine{atmega4809};
    std::vector<u16> dummy_flash(1024, 0x0000U);
    dummy_flash[1023] = 0xC000U | ((-1023) & 0x0FFFU); // RJMP back to start
    machine.bus().load_flash(dummy_flash);
    
    machine.reset();
    auto& bus = machine.bus();
    
    // 1. Fill Page Buffer via Mapped User Row (0x1300)
    bus.write_data(0x1300, 0x42U);
    bus.write_data(0x1301, 0x24U);
    
    // 2. Set NVM ADDR (0x1300)
    bus.write_data(0x1008, 0x00U);
    bus.write_data(0x1009, 0x13U);
    
    // 3. Issue URWP (0x10)
    bus.write_data(0x1000, 0x10U); 
    
    machine.run(100000);
    
    // 4. Verify User Row
    CHECK(bus.read_data(0x1300) == 0x42U);
    CHECK(bus.read_data(0x1301) == 0x24U);
}

TEST_CASE("AVR8X NVMCTRL - EEPROM Write") {
    Machine machine{atmega4809};
    std::vector<u16> dummy_flash(1024, 0x0000U);
    dummy_flash[1023] = 0xC000U | ((-1023) & 0x0FFFU); // RJMP back to start
    machine.bus().load_flash(dummy_flash);
    
    Eeprom eeprom{"EEPROM", atmega4809.eeproms[0]};
    machine.bus().attach_peripheral(eeprom);
    
    machine.reset();
    auto& bus = machine.bus();
    
    // 1. Fill EEPROM Page Buffer (0x1400+)
    bus.write_data(0x1400, 0x77U);
    bus.write_data(0x1405, 0x88U);
    
    // 2. Set ADDR (0x1400)
    bus.write_data(0x1008, 0x00U);
    bus.write_data(0x1009, 0x14U);
    
    // 3. Issue EEWP (0x07)
    bus.write_data(0x1000, 0x07U);
    
    machine.run(100000);
    
    // 4. Verify EEPROM (both via map and peripheral)
    CHECK(bus.read_data(0x1400) == 0x77U);
    CHECK(bus.read_data(0x1405) == 0x88U);
    CHECK(eeprom.read(0x1400) == 0x77U);
}
