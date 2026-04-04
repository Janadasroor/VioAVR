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

constexpr vioavr::core::u16 encode_sts(const vioavr::core::u8 source)
{
    return static_cast<vioavr::core::u16>(0x9200U | (static_cast<vioavr::core::u16>(source) << 4U));
}

constexpr vioavr::core::u16 kSei = 0x9478U;
constexpr vioavr::core::u16 kReti = 0x9518U;
constexpr vioavr::core::u16 kNop = 0x0000U;

using namespace vioavr::core;
void step_to(AvrCpu& cpu, u32 target_pc) { while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) { cpu.step(); } }
}  // namespace

TEST_CASE("CPU Interrupt Priority and Chaining Test")
{
    using vioavr::core::AvrCpu;
    using vioavr::core::HexImage;
    using vioavr::core::MemoryBus;
    using vioavr::core::SregFlag;
    using vioavr::core::Timer8;
    using vioavr::core::Timer8Descriptor;
    using vioavr::core::devices::atmega328;

    constexpr auto low_tcnt = static_cast<vioavr::core::u16>(0x50U);
    constexpr auto low_ocr = static_cast<vioavr::core::u16>(0x51U);
    constexpr auto low_tifr = static_cast<vioavr::core::u16>(0x52U);
    constexpr auto low_timsk = static_cast<vioavr::core::u16>(0x53U);
    constexpr auto low_tccra = static_cast<vioavr::core::u16>(0x54U);
    constexpr auto low_tccrb = static_cast<vioavr::core::u16>(0x55U);
    
    constexpr auto high_tcnt = static_cast<vioavr::core::u16>(0x60U);
    constexpr auto high_ocr = static_cast<vioavr::core::u16>(0x61U);
    constexpr auto high_tifr = static_cast<vioavr::core::u16>(0x62U);
    constexpr auto high_timsk = static_cast<vioavr::core::u16>(0x63U);
    constexpr auto high_tccra = static_cast<vioavr::core::u16>(0x64U);
    constexpr auto high_tccrb = static_cast<vioavr::core::u16>(0x65U);
    
    constexpr auto low_vector = static_cast<vioavr::core::u8>(14U);
    constexpr auto high_vector = static_cast<vioavr::core::u8>(20U);

    MemoryBus bus {atmega328};
    
    Timer8Descriptor high_desc {
        .tcnt_address = high_tcnt,
        .ocra_address = high_ocr,
        .ocrb_address = 0U,
        .tifr_address = high_tifr,
        .timsk_address = high_timsk,
        .tccra_address = high_tccra,
        .tccrb_address = high_tccrb,
        .compare_a_vector_index = high_vector,
        .compare_b_vector_index = 0U,
        .overflow_vector_index = 21U,
        .compare_a_enable_mask = 0x02U,
        .compare_b_enable_mask = 0x00U,
        .overflow_enable_mask = 0x01U
    };
    
    Timer8Descriptor low_desc {
        .tcnt_address = low_tcnt,
        .ocra_address = low_ocr,
        .ocrb_address = 0U,
        .tifr_address = low_tifr,
        .timsk_address = low_timsk,
        .tccra_address = low_tccra,
        .tccrb_address = low_tccrb,
        .compare_a_vector_index = low_vector,
        .compare_b_vector_index = 0U,
        .overflow_vector_index = 15U,
        .compare_a_enable_mask = 0x02U,
        .compare_b_enable_mask = 0x00U,
        .overflow_enable_mask = 0x01U
    };

    Timer8 high_timer {"TIMER_HIGH", high_desc};
    Timer8 low_timer {"TIMER_LOW", low_desc};
    bus.attach_peripheral(high_timer);
    bus.attach_peripheral(low_timer);
    AvrCpu cpu {bus};

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16U, 0x05U),                 // 0
            encode_sts(16U), low_ocr,              // 1,2
            encode_sts(16U), high_ocr,             // 3,4
            encode_ldi(17U, 0x02U),                // 5
            encode_sts(17U), low_timsk,            // 6,7
            encode_sts(17U), high_timsk,           // 8,9
            kSei,                                  // 10
            kNop,                                  // 11 mainline resumes here after both ISRs
            encode_ldi(20U, 0x55U),                // 12
            kNop,                                  // 13
            encode_ldi(18U, 0xA1U),                // 14 low-priority-number ISR body
            kReti,                                 // 15
            kNop,                                  // 16
            kNop,                                  // 17
            kNop,                                  // 18
            kNop,                                  // 19
            encode_ldi(19U, 0xB2U),                // 20 higher-priority (wait, lower vector = higher priority)
            kReti                                  // 21
        },
        .entry_word = 0U
    });

    cpu.reset();

    SUBCASE("Simultaneous interrupt trigger - lower vector first") {
        for (int i = 0; i < 7; ++i) cpu.step(); // Up to SEI
        
        // Trigger both interrupts simultaneously before SEI finishes?
        // SEI takes 1 cycle, then one instruction after SEI executes before interrupt.
        cpu.step(); // Execute SEI
        cpu.step(); // Execute NOP (one instruction rule)
        
        const auto snapshot = cpu.snapshot();
        CHECK(snapshot.interrupt_pending);
        
        cpu.step(); // Entry to first ISR (Vector 14 has higher priority than 20)
        CHECK(cpu.snapshot().program_counter == low_vector);
        CHECK(cpu.snapshot().in_interrupt_handler);
        
        cpu.step(); // LDI R18, 0xA1
        cpu.step(); // RETI
        
        // After RETI, one mainline instruction should execute
        CHECK(cpu.snapshot().program_counter == 11U); // back to NOP
        CHECK_FALSE(cpu.snapshot().in_interrupt_handler);
        CHECK(cpu.snapshot().interrupt_pending); // High vector still pending
        
        cpu.step(); // Entry to second ISR (Vector 20)
        CHECK(cpu.snapshot().program_counter == high_vector);
        
        cpu.step(); // LDI R19, 0xB2
        cpu.step(); // RETI
        
        CHECK(cpu.snapshot().program_counter == 11U);
        cpu.step(); // LDI R20, 0x55
        CHECK(cpu.snapshot().gpr[20] == 0x55U);
    }
}
