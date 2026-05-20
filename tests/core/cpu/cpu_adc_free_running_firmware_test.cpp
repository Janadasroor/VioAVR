#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

using namespace vioavr::core;

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

TEST_CASE("ADC Free Running Firmware Integration Test")
{
    using vioavr::core::Adc;
    using vioavr::core::AvrCpu;
    using vioavr::core::HexImage;
    using vioavr::core::MemoryBus;
    using vioavr::core::devices::atmega328p;

    PinMux pin_mux(8);
    MemoryBus bus {atmega328p};
    Adc adc0 {"ADC0", atmega328p.adcs[0], pin_mux, 6U, 4U};
    adc0.set_bus(bus);
    adc0.set_channel_voltage(0U, 0.25);
    bus.attach_peripheral(adc0);
    AvrCpu cpu {bus};

    // Firmware steps:
    // 1. LDI r16, 0. STS r16, ADMUX
    // 2. STS r16, ADCSRB (ADTS=0, Free Running)
    // 3. LDI r17, 0xE0 (ADEN=1, ADSC=1, ADATE=1)
    // 4. STS r17, ADCSRA
    // 5. LDS r18, ADCSRA (check ADSC)
    // 6. LDS r19, ADCSRA (check ADIF)
    // 7. LDS r20, ADCL
    // 8. LDS r21, ADCH
    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16U, 0x00U),                     // 0
            encode_sts(16U), atmega328p.adcs[0].admux_address, // 1, 2
            encode_sts(16U), atmega328p.adcs[0].adcsrb_address, // 3, 4
            encode_ldi(17U, 0xE0U),                     // 5
            encode_sts(17U), atmega328p.adcs[0].adcsra_address, // 6, 7
            encode_lds(18U), atmega328p.adcs[0].adcsra_address, // 8, 9
            encode_lds(19U), atmega328p.adcs[0].adcsra_address, // 10, 11
            encode_lds(20U), atmega328p.adcs[0].adcl_address,   // 12, 13
            encode_lds(21U), atmega328p.adcs[0].adch_address,   // 14, 15
            0x0000U
        },
        .entry_word = 0U
    });

    cpu.reset();

    SUBCASE("Continuous conversion in firmware") {
        // Run setup (5 instructions: LDI, STS, STS, LDI, STS)
        // Cycles: 1 + 2 + 2 + 1 + 2 = 8
        cpu.run(8);
        CHECK(cpu.cycles() == 8U);
        CHECK(cpu.program_counter() == 8U);
        
        // LDS r18, ADCSRA
        cpu.step(); // 2 cycles
        // ADSC should be set, ADIF not yet
        CHECK((cpu.snapshot().gpr[18] & 0x40U) != 0U);
        CHECK((cpu.snapshot().gpr[18] & 0x10U) == 0U);

        // Tick to complete first conversion
        // Conversion is 4 cycles. We already ticked 2 during LDS.
        cpu.step(); // LDS r19, ADCSRA (2 cycles)
        const auto s2 = cpu.snapshot();
        // Now ADSC=1 (free running), ADIF=1
        CHECK((s2.gpr[19] & 0x50U) == 0x50U);

        // Read results
        cpu.step(); // LDS r20, ADCL
        cpu.step(); // LDS r21, ADCH
        const auto s3 = cpu.snapshot();
        const auto result = static_cast<vioavr::core::u16>(
            s3.gpr[20] | (static_cast<vioavr::core::u16>(s3.gpr[21]) << 8U)
        );
        CHECK(result >= 254U);
        CHECK(result <= 258U);
    }
}
