#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

namespace {

using namespace vioavr::core;

constexpr u16 encode_ldi(u8 destination, u8 immediate) {
    return static_cast<u16>(0xE000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_sts(u8 source) {
    return static_cast<u16>(0x9200U | (static_cast<u16>(source & 0x1FU) << 4U));
}

constexpr u16 kSpm = 0x95E8U;
constexpr u16 kLpmZ = 0x9004U;

} // namespace

TEST_CASE("CPU SPM (Store Program Memory) Firmware Lifecycle Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328p};
    AvrCpu cpu {bus};
    const u16 spmcsr_addr = atmega328p.spmcsr_address;

    // Firmware sequence:
    // 1. Load Z with 0x0000 (starting Page 0)
    // 2. Fill Buffer Word 0: 0x1122
    //    r0=0x22, r1=0x11, r16=0x01 (SELFPRGEN)
    //    STS SPMCSR, r16; SPM
    // 3. Fill Buffer Word 1: 0x3344 (Z=0x0002)
    //    r0=0x44, r1=0x33, r16=0x01 (SELFPRGEN)
    //    STS SPMCSR, r16; SPM
    // 4. Erase Page 0 (Z=0x0000)
    //    r16=0x03 (SELFPRGEN | PGERS)
    //    STS SPMCSR, r16; SPM
    // 5. Write Page 0 (Z=0x0000)
    //    r16=0x05 (SELFPRGEN | PGWRT)
    //    STS SPMCSR, r16; SPM
    // 6. Verification Loop (LPM)
    
    // Put the test code at the end of flash (Bootloader section) to avoid RWW conflicts
    const u32 code_start_word = 16384U - 128U;
    std::vector<u16> flash(16384, 0x0000U);
    
    // Jump to code from reset
    flash[0] = 0x940CU; // JMP
    flash[1] = static_cast<u16>(code_start_word);
    
    // Helper to encode MOV Rd, Rr
    const auto encode_mov = [](u8 dst, u8 src) {
        return static_cast<u16>(0x2C00U | ((src & 0x10U) << 5U) | ((dst & 0x10U) << 4U) | ((dst & 0x0FU) << 4U) | (src & 0x0FU));
    };

    u32 pc = code_start_word;
    
    // Setup Z = 0 (Page 0)
    flash[pc++] = encode_ldi(16, 0x00);
    flash[pc++] = encode_mov(30, 16); // ZL = 0
    flash[pc++] = encode_mov(31, 16); // ZH = 0
    
    // Word 0: 0x1122
    flash[pc++] = encode_ldi(16, 0x22); flash[pc++] = encode_mov(0, 16);
    flash[pc++] = encode_ldi(16, 0x11); flash[pc++] = encode_mov(1, 16);
    flash[pc++] = encode_ldi(16, 0x01); // SELFPRGEN
    flash[pc++] = encode_sts(16); flash[pc++] = spmcsr_addr;
    flash[pc++] = kSpm;

    // Word 1: 0x3344 (Z=2)
    flash[pc++] = encode_ldi(16, 0x44); flash[pc++] = encode_mov(0, 16);
    flash[pc++] = encode_ldi(16, 0x33); flash[pc++] = encode_mov(1, 16);
    flash[pc++] = encode_ldi(16, 0x02); flash[pc++] = encode_mov(30, 16); // ZL = 2
    flash[pc++] = encode_ldi(16, 0x01); // SELFPRGEN
    flash[pc++] = encode_sts(16); flash[pc++] = spmcsr_addr;
    flash[pc++] = kSpm;
    
    // Erase Page 0 (Z=0)
    flash[pc++] = encode_ldi(16, 0x00); flash[pc++] = encode_mov(30, 16);
    flash[pc++] = encode_ldi(16, 0x03); // SELFPRGEN | PGERS
    flash[pc++] = encode_sts(16); flash[pc++] = spmcsr_addr;
    flash[pc++] = kSpm;
    // Wait for SPM done
    flash[pc++] = encode_lds(16, spmcsr_addr); flash[pc++] = 0xFD00; flash[pc++] = 0xCFFD;

    // Must clear RWWSB before next operation on RWW section
    flash[pc++] = encode_ldi(16, 0x11); // SELFPRGEN | RWWSRE
    flash[pc++] = encode_sts(16); flash[pc++] = spmcsr_addr;
    flash[pc++] = kSpm;

    // Write Page 0 (Z=0)
    flash[pc++] = encode_ldi(16, 0x05); // SELFPRGEN | PGWRT
    flash[pc++] = encode_sts(16); flash[pc++] = spmcsr_addr;
    flash[pc++] = kSpm;
    
    // Final RWWSRE to allow reading back from RWW section
    flash[pc++] = encode_ldi(16, 0x11); // SELFPRGEN | RWWSRE
    flash[pc++] = encode_sts(16); flash[pc++] = spmcsr_addr;
    flash[pc++] = kSpm;

    // Read back Word 0 into r2, Word 1 into r3
    flash[pc++] = encode_ldi(16, 0x00); flash[pc++] = encode_mov(30, 16);
    flash[pc++] = 0x9024; // LPM r2, Z
    flash[pc++] = encode_ldi(16, 0x02); flash[pc++] = encode_mov(30, 16);
    flash[pc++] = 0x9034; // LPM r3, Z
    
    flash[pc++] = 0x0000U; // NOP
    
    bus.load_image(HexImage {
        .flash_words = flash,
        .entry_word = 0U
    });
    cpu.reset();

    // Execute the lifecycle
    // Clear and fill word 0, word 1, erase, write, verify.
    // Total instructions ~40. But Erase and Write take 64k cycles each!
    // Plus JMP. 150,000 cycles is needed.
    cpu.run(150000);

    CHECK(cpu.registers()[2] == 0x22U);
    CHECK(cpu.registers()[3] == 0x44U);
    
    // Verify flash directly
    CHECK(bus.read_program_word(0) == 0x1122U);
    CHECK(bus.read_program_word(1) == 0x3344U);
    
    // SPMCSR should be 0
    CHECK(bus.read_data(spmcsr_addr) == 0U);
}
