#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/eeprom.hpp"
#include "vioavr/core/nvm_ctrl.hpp"

using namespace vioavr::core;

TEST_CASE("NVM Stalling Fidelity: EEPROM and Flash Wait States")
{
    // Use ATmega4809 (AVR8X) for mapped EEPROM testing
    const auto* desc = DeviceCatalog::find("ATmega4809");
    REQUIRE(desc != nullptr);
    MemoryBus bus(*desc);
    AvrCpu cpu(bus);
    
    // Attach EEPROM
    auto eeprom = std::make_unique<Eeprom>("EEPROM", desc->eeproms[0]);
    bus.attach_peripheral(*eeprom);
    
    // Attach NVMCTRL
    auto nvmctrl = std::make_unique<NvmCtrl>(desc->nvm_ctrls[0]);
    bus.attach_peripheral(*nvmctrl);

    cpu.reset();

    SUBCASE("Mapped EEPROM Read Stalling") {
        // LDS R16, 0x1400 (EEPROM start on 4809)
        std::vector<u16> code = { 0x9100, 0x1400, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }; 
        bus.load_flash(code);
        cpu.set_program_counter(0);
        
        u64 start_cycles = cpu.cycles();
        cpu.step(); // Execute instruction (2 cycles)
        
        // At this point, LDS is done, but the bus has 4 stall cycles pending.
        for(int i=0; i<4; ++i) cpu.step();
        
        // Total should be 2 + 4 = 6 cycles.
        CHECK(cpu.cycles() - start_cycles == 6);
    }

    SUBCASE("Mapped EEPROM Write Stalling") {
        // STS 0x1400, R16
        std::vector<u16> code = { 0x9300, 0x1400, 0x0000, 0x0000, 0x0000 };
        bus.load_flash(code);
        cpu.set_program_counter(0);
        
        u64 start_cycles = cpu.cycles();
        cpu.step(); // Execute instruction (2 cycles)
        
        // We need 2 more steps to clear the stall.
        for(int i=0; i<2; ++i) cpu.step();
        
        // Total should be 2 + 2 = 4 cycles.
        CHECK(cpu.cycles() - start_cycles == 4);
    }
}

TEST_CASE("NVM Stalling Fidelity: Legacy SPM and EEPE")
{
    // Use ATmega328P for legacy testing
    const auto* desc = DeviceCatalog::find("ATmega328P");
    REQUIRE(desc != nullptr);
    MemoryBus bus(*desc);
    AvrCpu cpu(bus);
    
    auto eeprom = std::make_unique<Eeprom>("EEPROM", desc->eeproms[0]);
    bus.attach_peripheral(*eeprom);
    
    cpu.reset();

    SUBCASE("EEPROM Write Initiation Stall") {
        cpu.write_register(16, 0x04); // EEMPE
        std::vector<u16> code_eempe = { 0xBF0F, 0x0000 }; 
        bus.load_flash(code_eempe);
        cpu.set_program_counter(0);
        cpu.step();
        
        cpu.write_register(16, 0x06); // EEMPE | EEPE
        std::vector<u16> code_eepe = { 0xBF0F, 0x0000, 0x0000, 0x0000 };
        bus.load_flash(code_eepe);
        cpu.set_program_counter(0);
        
        u64 start_cycles = cpu.cycles();
        cpu.step(); // Execute instruction (1 cycle). This triggers the 2-cycle stall.
        
        // Instruction cycle count
        CHECK(cpu.cycles() - start_cycles == 1);
        
        // Now 2 stall steps
        cpu.step();
        cpu.step();
        
        CHECK(cpu.cycles() - start_cycles == 3);
    }
}
