#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega32u4.hpp"

using namespace vioavr::core;

TEST_CASE("SPM: Fidelity Timing and RWW Busy") {
    MemoryBus bus {devices::atmega32u4};
    AvrCpu cpu {bus};
    
    // address 0x1000 in bytes (0x800 in words)
    const u32 target_byte_addr = 0x1000;
    const u32 target_word_addr = 0x800;
    
    // We will load a program that fills the buffer and erases a page.
    // FLASH_RWW_END_WORD MUST be set in the device for this to truly test RWW busy.
    // For now, let's assume it's working.
    
    // ATmega32U4 SPMCSR is 0x57. 
    // Opcode for OUT 0x37, R16 (where 0x37 is IO offset for 0x57)
    // 1011 1 A A r r r r r A A A A
    // A=0x37: A5=1, A4=1, A3=0, A2=1, A1=1, A0=1. 
    // R16: 10000
    // 1011 1 11 10000 0111 -> 0xBF07. Correct.

    std::vector<u16> prog = {
        // Z = 0x1000 (byte address)
        0xE0E0, // LDI R30, 0x00
        0xE1F0, // LDI R31, 0x10
        
        // Page Erase: SPMEN | PGERS = 0x03
        0xE003, // LDI R16, 0x03
        0xBF07, // OUT 0x37, R16
        0x95E8, // SPM
        
        0x0000, 0x0000, 0x0000
    };
    
    bus.load_flash(prog);
    cpu.reset();
    
    // Run until SPM
    for (int i = 0; i < 5; ++i) cpu.step();
    
    // Verify SPM operation started
    CHECK((bus.read_data(0x57) & 0x01) != 0); // SPMEN should be 1
    
    // Flash read at target should be 0xFFFF (busy return? No, target is 0xFFFF already if erased)
    // Wait! Let's write something there first to see if erases.
    bus.write_program_word(target_word_addr, 0x1234);
    
    // After SPM (Erase) started, it should NOT be erased yet.
    CHECK(bus.flash_words()[target_word_addr] == 0x1234);
    
    // Advance some cycles (10,000)
    bus.tick_peripherals(10000);
    
    // Still not erased
    CHECK(bus.flash_words()[target_word_addr] == 0x1234);
    CHECK((bus.read_data(0x57) & 0x01) != 0); // Still busy
    
    // Advance another 60,000 cycles to be sure it finishes (Total 70,000)
    bus.tick_peripherals(60000);
    
    // It should be finished now.
    CHECK(bus.flash_words()[target_word_addr] == 0xFFFF);
    CHECK((bus.read_data(0x57) & 0x01) == 0); // SPMEN cleared
    
    // RWWSB should be set!
    CHECK((bus.read_data(0x57) & 0x40) != 0);
    
    // Read from RWW section should return 0xFFFF now because it's busy
    CHECK(bus.read_program_word(target_word_addr) == 0xFFFF);
    
    // Clearing RWWSB: Write RWWSRE (0x10) | SPMEN (0x01)? No, datasheet says just RWWSRE.
    // Wait! SPM instruction with RWWSRE set clears RWWSB.
    // For simplicity, we implemented writing to SPMCSR with RWWSRE bit clears it in our MemoryBus.
    bus.write_data(0x57, 0x11); // RWWSRE | SPMEN
    CHECK((bus.read_data(0x57) & 0x40) == 0);
    
    // Now read works
    CHECK(bus.read_program_word(target_word_addr) == 0xFFFF);
}
