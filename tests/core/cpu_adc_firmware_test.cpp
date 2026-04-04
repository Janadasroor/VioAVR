#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
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

constexpr vioavr::core::u16 encode_lds(const vioavr::core::u8 destination)
{
    return static_cast<vioavr::core::u16>(0x9000U | (static_cast<vioavr::core::u16>(destination) << 4U));
}

constexpr vioavr::core::u16 encode_sts(const vioavr::core::u8 source)
{
    return static_cast<vioavr::core::u16>(0x9200U | (static_cast<vioavr::core::u16>(source) << 4U));
}

}  // namespace

using namespace vioavr::core;
void step_to(AvrCpu& cpu, u32 target_pc) { while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) { cpu.step(); } }

TEST_CASE("ADC Firmware Integration Test")
{
    using vioavr::core::Adc;
    using vioavr::core::AvrCpu;
    using vioavr::core::HexImage;
    using vioavr::core::MemoryBus;
    using vioavr::core::devices::atmega328;

    MemoryBus bus {atmega328};
    Adc adc0 {"ADC0", atmega328, 6U, 4U};
    adc0.set_channel_voltage(0U, 0.25);
    bus.attach_peripheral(adc0);
    AvrCpu cpu {bus};

    // Firmware steps:
    // 1. Write 0x00 to ADMUX (channel 0)
    // 2. Write 0xC0 to ADCSRA (ADEN=1, ADSC=1)
    // 3. Read ADCSRA (check ADSC)
    // 4. Read ADCSRA (check ADIF)
    // 5. Read ADCL
    // 6. Read ADCH
    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16U, 0x00U),
            encode_sts(16U), atmega328.adc.admux_address,
            encode_ldi(17U, 0xC0U),   // ADEN | ADSC
            encode_sts(17U), atmega328.adc.adcsra_address,
            encode_lds(18U), atmega328.adc.adcsra_address,
            encode_lds(19U), atmega328.adc.adcsra_address,
            encode_lds(20U), atmega328.adc.adcl_address,
            encode_lds(21U), atmega328.adc.adch_address,
            0x0000U
        },
        .entry_word = 0U
    });

    cpu.reset();

    SUBCASE("Program execution flow") {
        // Run until LDI/STS for ADMUX and LDI/STS for ADCSRA (4 instructions)
        // LDI (1), STS (2), LDI (1), STS (2) = 6 cycles
        step_to(cpu, 6U);
        CHECK(cpu.cycles() == 6U);
        CHECK(cpu.program_counter() == 4U); // PC in words (LDI+STS=2 words x 2 = 4 blocks, but STS is 2 words)
        // Wait, STS is 2 words. 
        // Instructions: LDI(0), STS(1,2), LDI(3), STS(4,5)
        // Next PC: 6
        CHECK(cpu.program_counter() == 6U);
        
        // Next instruction: LDS r18, ADCSRA (2 words)
        cpu.step(); // LDS occupies 2 cycles
        // ADSC should be set, ADIF should be 0 (conversion in progress)
        CHECK((cpu.snapshot().gpr[18] & 0x40U) != 0U);
        CHECK((cpu.snapshot().gpr[18] & 0x10U) == 0U);

        // Tick peripherals to complete conversion (4 cycles)
        // LDS was 2 cycles. We need 2 more. Next LDS is another 2 cycles.
        cpu.step(); // LDS r19, ADCSRA
        const auto s1 = cpu.snapshot();
        // Now conversion should be DONE. ADSC=0, ADIF=1
        CHECK((s1.gpr[19] & 0x50U) == 0x10U);

        // Read ADCL and ADCH
        cpu.step(); // LDS r20, ADCL
        cpu.step(); // LDS r21, ADCH
        const auto s2 = cpu.snapshot();
        const auto result = static_cast<vioavr::core::u16>(
            s2.gpr[20] | (static_cast<vioavr::core::u16>(s2.gpr[21]) << 8U)
        );
        // 0.25V -> 256
        CHECK(result >= 254U);
        CHECK(result <= 258U);
    }
}
