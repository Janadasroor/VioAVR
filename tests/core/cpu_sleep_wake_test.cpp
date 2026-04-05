#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/devices/atmega328.hpp"

namespace {

constexpr vioavr::core::u16 encode_ldi(const vioavr::core::u8 destination, const vioavr::core::u8 immediate)
{
    return static_cast<vioavr::core::u16>(
        0xE000U |
        ((static_cast<vioavr::core::u16>(immediate & 0xF0U)) << 4U) |
        (static_cast<vioavr::core::u16>(destination - 16U) << 4U) |
        (immediate & 0x0FU)
    );
}

constexpr vioavr::core::u16 encode_out(const vioavr::core::u8 io_offset, const vioavr::core::u8 source)
{
    return static_cast<vioavr::core::u16>(
        0xB800U |
        ((static_cast<vioavr::core::u16>(source) & 0x1FU) << 4U) |
        ((static_cast<vioavr::core::u16>(io_offset) & 0x30U) << 5U) |
        (io_offset & 0x0FU)
    );
}

constexpr vioavr::core::u16 encode_sts(const vioavr::core::u8 source)
{
    return static_cast<vioavr::core::u16>(0x9200U | (static_cast<vioavr::core::u16>(source) << 4U));
}

constexpr vioavr::core::u16 kSei = 0x9478U;
constexpr vioavr::core::u16 kSleep = 0x9588U;
constexpr vioavr::core::u16 kReti = 0x9518U;
constexpr vioavr::core::u16 kNop = 0x0000U;

using namespace vioavr::core;
void step_to(AvrCpu& cpu, u32 target_pc) { while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) { cpu.step(); } }
}  // namespace

TEST_CASE("CPU Sleep and Wake Test")
{
    using vioavr::core::AvrCpu;
    using vioavr::core::CpuState;
    using vioavr::core::HexImage;
    using vioavr::core::MemoryBus;
    using vioavr::core::SregFlag;
    using vioavr::core::Timer8;
    using vioavr::core::devices::atmega328;

    MemoryBus bus {atmega328};
    Timer8 timer0 {"TIMER0", atmega328};
    bus.attach_peripheral(timer0);
    AvrCpu cpu {bus};

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16U, 0x01U),   // 0 OCR0A compare threshold
            encode_out(0x27U, 16U),   // 1 OCR0 (0x33 -> 0x13)
            encode_ldi(19U, 0x02U),   // 2 OCIE0A
            encode_sts(19U), atmega328.timer0.timsk_address,  // 3,4 TIMSK0
            encode_ldi(20U, 0x01U),   // 5 CS00
            encode_out(0x25U, 20U),   // 6 TCCR0B (0x45 -> 0x25)
            kSei,                     // 7
            kSleep,                   // 8
            encode_ldi(18U, 0x55U),   // 9 executes after wake/RETI
            kNop,                     // 10
            kNop,                     // 11
            kNop,                     // 12
            kNop,                     // 13
            encode_ldi(17U, 0x77U),   // 14 ISR body
            kReti                     // 15
        },
        .entry_word = 0U
    });

    cpu.reset();

    SUBCASE("Setup instructions") {
        for (int i = 0; i < 8; ++i) cpu.step();
        const auto snapshot = cpu.snapshot();
        CHECK_FALSE(snapshot.interrupt_pending);
        CHECK(snapshot.program_counter == 9U);
        CHECK((snapshot.sreg & (1U << static_cast<vioavr::core::u8>(SregFlag::interrupt))) != 0U);
    }

    SUBCASE("Enter sleep") {
        for (int i = 0; i < 8; ++i) cpu.step();
        cpu.step(); // SLEEP
        const auto snapshot = cpu.snapshot();
        CHECK(snapshot.program_counter == 9U);
        CHECK(snapshot.cycles == 10U);
        CHECK(snapshot.state == CpuState::sleeping);
    }

    SUBCASE("Wake from sleep via Timer interrupt") {
        for (int i = 0; i < 10; ++i) cpu.step();
        
        cpu.run(6U); // Wait for timer to reach OCR0=8
        const auto snapshot = cpu.snapshot();
        const auto ramend = atmega328.sram_range().end;
        
        CHECK(snapshot.program_counter == 14U); // Jumped to ISR at OCR0A vector
        CHECK(snapshot.stack_pointer == static_cast<vioavr::core::u16>(ramend - 2U));
        CHECK(snapshot.cycles == 18U);
        CHECK(snapshot.state == CpuState::running);
        CHECK(snapshot.in_interrupt_handler);
        CHECK_FALSE((snapshot.sreg & (1U << static_cast<vioavr::core::u8>(SregFlag::interrupt))));
    }

    SUBCASE("ISR Execution and RETI") {
        for (int i = 0; i < 10; ++i) cpu.step();
        cpu.run(6U);
        
        cpu.step(); // LDI R17, 0x77
        auto snapshot = cpu.snapshot();
        CHECK(snapshot.gpr[17] == 0x77U);
        CHECK(snapshot.program_counter == 15U);

        cpu.step(); // RETI
        snapshot = cpu.snapshot();
        const auto ramend = atmega328.sram_range().end;
        CHECK(snapshot.program_counter == 9U); // Returned to instruction after SLEEP
        CHECK(snapshot.stack_pointer == ramend);
        CHECK_FALSE(snapshot.in_interrupt_handler);
        CHECK((snapshot.sreg & (1U << static_cast<vioavr::core::u8>(SregFlag::interrupt))) != 0U);

        cpu.step(); // LDI R18, 0x55
        snapshot = cpu.snapshot();
        CHECK(snapshot.gpr[18] == 0x55U);
        CHECK(snapshot.program_counter == 10U);
    }
}
