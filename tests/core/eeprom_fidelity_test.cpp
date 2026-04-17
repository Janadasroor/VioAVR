#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/eeprom.hpp"
#include "vioavr/core/devices/atmega32u4.hpp"

using namespace vioavr::core;

TEST_CASE("EEPROM: Fidelity Timing and Interrupts") {
    MemoryBus bus {devices::atmega32u4};
    AvrCpu cpu {bus};
    
    // Instantiate and attach EEPROM
    auto eeprom_ptr = std::make_unique<Eeprom>("EEPROM0", devices::atmega32u4.eeproms[0]);
    Eeprom* eeprom = eeprom_ptr.get();
    bus.attach_peripheral(*eeprom_ptr.release());
    
    REQUIRE(eeprom != nullptr);
    
    // EECR bits
    const u8 kEerie = 0x08U;
    const u8 kEempe = 0x04U;
    const u8 kEepe  = 0x02U;
    const u8 kEere  = 0x01U;
    
    const u16 eecr_addr = 0x3F;
    const u16 eedr_addr = 0x40;
    const u16 eearl_addr = 0x41;
    
    // 1. Test Read Timing (Immediate)
    eeprom->reset();
    bus.write_data(eearl_addr, 0x10);
    bus.write_data(eedr_addr, 0xAA);
    // Note: To write to storage, we need to complete a write operation.
    // Let's manually inject some data for read testing.
    // Actually, let's do a write first.
    
    // 2. Test Write Timing (Atomic)
    bus.write_data(eearl_addr, 0x05);
    bus.write_data(eedr_addr, 0x42);
    
    // Set EEMPE
    bus.write_data(eecr_addr, kEempe);
    CHECK((bus.read_data(eecr_addr) & kEempe) != 0);
    
    // Set EEPE (within 4 cycles)
    bus.write_data(eecr_addr, kEepe);
    
    // Verify EEPE is set and EEMPE is cleared
    CHECK((bus.read_data(eecr_addr) & kEepe) != 0);
    CHECK((bus.read_data(eecr_addr) & kEempe) == 0);
    
    // Advance 10,000 cycles
    bus.tick_peripherals(10000);
    CHECK((bus.read_data(eecr_addr) & kEepe) != 0); // Still busy
    
    // Advance to 55,000 cycles (Total 65k, limit is 54.4k)
    bus.tick_peripherals(55000);
    CHECK((bus.read_data(eecr_addr) & kEepe) == 0); // Done
    
    // 3. Verify Read
    bus.write_data(eecr_addr, kEere);
    CHECK(bus.read_data(eedr_addr) == 0x42);
    
    // 4. Test Interrupt
    eeprom->reset();
    bus.write_data(eecr_addr, kEerie); // Enable interrupt
    
    InterruptRequest req;
    CHECK(eeprom->pending_interrupt_request(req) == true);
    CHECK(req.vector_index == 30);
    
    // Start a write
    bus.write_data(eecr_addr, kEerie | kEempe);
    bus.write_data(eecr_addr, kEerie | kEepe);
    
    // Interrupt should NOT be pending while writing
    CHECK(eeprom->pending_interrupt_request(req) == false);
    
    // Finish write
    bus.tick_peripherals(60000);
    CHECK(eeprom->pending_interrupt_request(req) == true);
}
