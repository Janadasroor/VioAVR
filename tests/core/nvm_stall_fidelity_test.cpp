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
        
        // Total should be 2 + 54400 = 54402 cycles.
        for(int i=0; i<54400; ++i) cpu.step();
        CHECK(cpu.cycles() - start_cycles == 54402);
    }
}

TEST_CASE("NVM Stalling Fidelity: Legacy SPM and EEPE")
{
    const auto* desc = DeviceCatalog::find("ATmega328P");
    REQUIRE(desc != nullptr);
    MemoryBus bus(*desc);
    AvrCpu cpu(bus);
    
    // Attach EEPROM
    auto eeprom = std::make_unique<Eeprom>("EEPROM", desc->eeproms[0]);
    bus.attach_peripheral(*eeprom);

    cpu.reset();

    SUBCASE("EEPROM Write Initiation Stall") {
        // ATmega328P EEPROM write requires EEMPE set before EEPE within 4 cycles.
        // LDI R16, 0x04 = 0xE004  (EEMPE bit)
        // OUT EECR, R16 = 0xBB0F  (I/O addr 0x1F -> data addr 0x3F, sets EEMPE)
        // LDI R16, 0x06 = 0xE006  (EEMPE | EEPE)
        // OUT EECR, R16 = 0xBB0F  (triggers write: 1 cycle + 2-cycle stall)
        std::vector<u16> code = { 0xE004, 0xBB0F, 0xE006, 0xBB0F, 0x0000, 0x0000 };
        bus.load_flash(code);
        cpu.set_program_counter(0);
        
        // 1st step: LDI R16, 0x04 (1 cycle)
        u64 start_cycles = cpu.cycles();
        cpu.step();
        CHECK(cpu.cycles() - start_cycles == 1);
        
        // 2nd step: OUT sets EEMPE only (1 cycle, no write triggered)
        start_cycles = cpu.cycles();
        cpu.step();
        CHECK(cpu.cycles() - start_cycles == 1);
        
        // 3rd step: LDI R16, 0x06 (1 cycle)
        start_cycles = cpu.cycles();
        cpu.step();
        CHECK(cpu.cycles() - start_cycles == 1);
        
        // 4th step: OUT sets EEPE with EEMPE active -> triggers 2-cycle stall
        // step() 1: OUT instruction (1 cycle) + sets stall of 2
        // step() 2: stall cycle 1 (1 cycle)
        // step() 3: stall cycle 2 (1 cycle)
        // Total = 3 cycles over 3 step() calls
        start_cycles = cpu.cycles();
        cpu.step(); // OUT executes + stall registered
        cpu.step(); // stall cycle 1
        cpu.step(); // stall cycle 2
        CHECK(cpu.cycles() - start_cycles == 3);
    }
}
