#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/analog_comparator.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/atmega328.hpp"

using namespace vioavr::core;

namespace {

using namespace vioavr::core;

constexpr u16 encode_ldi(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0xE000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_sts(const u8 source) {
    return static_cast<u16>(0x9200U | (static_cast<u16>(source) << 4U));
}

constexpr u16 kSei = 0x9478U;
constexpr u16 kReti = 0x9518U;
constexpr u16 kNop = 0x0000U;

void step_to(AvrCpu& cpu, u32 target_pc) { while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) { cpu.step(); } }
} // namespace

using namespace vioavr::core;
TEST_CASE("Analog Comparator Firmware Interrupt Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    const u16 acsr_addr = atmega328.ac.acsr_address;
    const u8 comparator_vector = atmega328.ac.vector_index; // 23 on 328P

    PinMux pin_mux(8);
    MemoryBus bus {atmega328};
    AnalogComparator comparator {"AC", atmega328.ac, pin_mux, 9U, 0.01};
    comparator.set_negative_input_voltage(0.80);
    comparator.set_positive_input_voltage(0.20);
    bus.attach_peripheral(comparator);
    AvrCpu cpu {bus};

    // Construct image with ISR at Vector 23
    // At vector_size = 4, vector 23 is at word address 23 * 2 = 46
    std::vector<u16> flash(128, kNop);
    flash[46] = encode_ldi(19U, 0xA5U); // ISR body
    flash[47] = kReti;
    
    // Entry point at offset 64
    flash[64] = encode_ldi(16U, 0x0BU);   // Entry point: ACIE | rising edge
    flash[65] = encode_sts(16U); flash[66] = acsr_addr; // Write to ACSR
    flash[67] = kSei;                     // Enable interrupts
    flash[68] = kNop;                     // Point to be interrupted
    flash[69] = encode_ldi(18U, 0x55U);   // Mainline after ISR
    flash[70] = kNop;

    bus.load_image(HexImage {
        .flash_words = flash,
        .entry_word = 64U
    });
    cpu.reset();

    SUBCASE("Interrupt Trigger and ISR Execution") {
        for (int i = 0; i < 3; ++i) cpu.step();
        auto s = cpu.snapshot();
        CHECK(s.program_counter == 68U);
        CHECK((s.sreg & (1U << static_cast<u8>(SregFlag::interrupt))) != 0U);

        // Trigger comparator
        comparator.set_positive_input_voltage(0.83);

        // First step executes the NOP at 68 (SEI delay)
        cpu.step(); 
        
        // Next step should enter ISR
        cpu.step();
        s = cpu.snapshot();
        // Vector 23 is at byte address 23 * 4 = 92. Word address 46.
        CHECK(s.program_counter == 46U); 
        CHECK(s.stack_pointer == static_cast<u16>(atmega328.sram_range().end - 2U));
        CHECK(s.in_interrupt_handler);

        // Step through ISR body
        cpu.step(); // LDI r19, 0xA5
        CHECK(cpu.snapshot().gpr[19] == 0xA5U);

        cpu.step(); // RETI
        s = cpu.snapshot();
        CHECK(s.program_counter == 69U); // Back to instruction after NOP
        CHECK(s.stack_pointer == atmega328.sram_range().end);
        CHECK_FALSE(s.in_interrupt_handler);

        // Mainline continues
        cpu.step(); // LDI r18, 0x55
        CHECK(cpu.snapshot().gpr[18] == 0x55U);
        
        // ACI should be cleared by hardware on ISR entry
        CHECK((bus.read_data(acsr_addr) & 0x10U) == 0U);
    }
}
