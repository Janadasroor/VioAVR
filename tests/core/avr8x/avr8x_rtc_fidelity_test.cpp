#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/rtc.hpp"
#include "doctest.h"

using namespace vioavr::core;

// ATmega4809 RTC Base: 0x0140
// CTRLA: 0x140, STATUS: 0x141, PER: 0x14A, CNT: 0x148
// PITCTRLA: 0x150, PITSTATUS: 0x151

TEST_CASE("AVR8X RTC - Synchronization Fidelity") {
    const auto* device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // Load NOPs to keep CPU running
    std::vector<u16> program(1000, 0x0000);
    bus.load_flash(program);
    machine.cpu().reset();
    
    // 0. CHECK PER default value (should be 0xFFFF)
    u16 per_default = bus.read_data(0x14A) | (bus.read_data(0x14B) << 8);
    CHECK(per_default == 0xFFFF);


    // 1. Write to PERL
    bus.write_data(0x14A, 10);
    
    // 2. CHECK STATUS.PERBUSY (Bit 1)
    u8 status = bus.read_data(0x141);
    CHECK((status & 0x02) != 0);

    // 3. Tick 32 cycles. Should still be busy (Sync delay is 64 in our impl)
    machine.run(32);
    status = bus.read_data(0x141);
    CHECK((status & 0x02) != 0);

    // 4. Tick 33 more cycles (Total 65)
    machine.run(33);
    status = bus.read_data(0x141);
    CHECK((status & 0x02) == 0);
}

TEST_CASE("AVR8X RTC - OVF Timing Fidelity") {
    const auto* device = DeviceCatalog::find("ATmega4809");
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // Load NOPs to keep CPU running
    std::vector<u16> program(1000, 0x0000);
    bus.load_flash(program);
    machine.cpu().reset();

    // 1. Configure RTC: PER=4 (5 counts), Prescaler=1 (DIV1)
    bus.write_data(0x14A, 4); // PERL=4
    bus.write_data(0x14B, 0); // PERH=0
    bus.write_data(0x140, 0x01); // ENABLE=1, PRESCALER=0 (DIV1)
    
    // Verify PER was written correctly
    u16 per = bus.read_data(0x14A) | (bus.read_data(0x14B) << 8);
    CHECK(per == 4);

    // 2. Run 4 cycles
    for (int i = 0; i < 4; ++i) {
        machine.run(1);
        u16 current_cnt = bus.read_data(0x148) | (bus.read_data(0x149) << 8);
        CHECK(current_cnt == (i + 1));
    }
    
    u16 cnt = bus.read_data(0x148) | (bus.read_data(0x149) << 8);
    CHECK(cnt == 4);
    CHECK((bus.read_data(0x143) & 0x01) == 0); // OVF flag not set yet

    // 3. Cycle 5: should wrap to 0 because cnt (4) == per (4)
    machine.run(1);
    cnt = bus.read_data(0x148) | (bus.read_data(0x149) << 8);
    CHECK(cnt == 0);
    CHECK((bus.read_data(0x143) & 0x01) != 0); // OVF flag set
}

TEST_CASE("AVR8X RTC - 16-bit Register Atomicity") {
    const auto* device = DeviceCatalog::find("ATmega4809");
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // 1. Write PERH=0x12, then PERL=0x34. PER should become 0x1234
    // Note: Do NOT read in between as it would clobber TEMP if reading Low byte
    bus.write_data(0x14B, 0x12); // PERH
    bus.write_data(0x14A, 0x34); // PERL triggers update
    u16 per = bus.read_data(0x14A) | (bus.read_data(0x14B) << 8);
    CHECK(per == 0x1234);

    // 2. Read CNTL first, it should latch CNTH into TEMP
    // Let's set some value first
    bus.write_data(0x141, 0); // Clear busy bits if any
    bus.write_data(0x149, 0xAA); // CNTH (via TEMP)
    bus.write_data(0x148, 0x55); // CNTL triggers update
    
    // Now read Low then High
    u8 low = bus.read_data(0x148);
    u8 high = bus.read_data(0x149);
    CHECK(low == 0x55);
    CHECK(high == 0xAA);
    
    // 3. Indirect access via TEMP register (0x144)
    bus.write_data(0x144, 0xCC); // Write 0xCC to TEMP
    bus.write_data(0x14A, 0xDD); // Write 0xDD to PERL. Should use 0xCC as High
    per = bus.read_data(0x14A) | (bus.read_data(0x14B) << 8);
    CHECK(per == 0xCCDD);
}


TEST_CASE("AVR8X RTC - PIT Timing Fidelity") {
    const auto* device = DeviceCatalog::find("ATmega4809");
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // Load NOPs
    std::vector<u16> program(1000, 0x0000);
    bus.load_flash(program);
    machine.cpu().reset();

    // 1. Configure PIT: PERIOD=1 (4 cycles in real hw, 4 cycles in our impl for p=0, but p=1 means 4 too in some models?)
    // Actually check my get_pit_period: if p=1 -> 1 << (1+1) = 4.
    bus.write_data(0x150, (0x01 << 3) | 0x01); // PERIOD=1, ENABLE=1
    
    u8 pitctrla = bus.read_data(0x150);
    CHECK(pitctrla == 0x09);

    // 2. Run 3 cycles
    machine.run(3);
    CHECK((bus.read_data(0x153) & 0x01) == 0); // PIT flag not set

    // 3. Cycle 4
    machine.run(1);
    CHECK((bus.read_data(0x153) & 0x01) != 0); // PIT flag set
}
