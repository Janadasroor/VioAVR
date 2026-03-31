#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/analog_comparator.hpp"
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

TEST_CASE("ADC Auto-Trigger via Comparator Firmware Integration Test")
{
    using vioavr::core::Adc;
    using vioavr::core::AnalogComparator;
    using vioavr::core::AvrCpu;
    using vioavr::core::HexImage;
    using vioavr::core::MemoryBus;
    using vioavr::core::devices::atmega328;

    constexpr auto acsr_addr = static_cast<vioavr::core::u16>(0x50U);

    MemoryBus bus {atmega328};
    Adc adc0 {"ADC0", atmega328, 6U, 4U};
    AnalogComparator comparator {"AC", acsr_addr, 8U, 9U, 1.1};
    
    adc0.connect_comparator_auto_trigger(comparator);
    adc0.set_channel_voltage(0U, 0.75); // Target value: 768
    
    comparator.set_negative_input_voltage(0.80);
    comparator.set_positive_input_voltage(0.20); // ACO = 0 (0.20 < 0.80)
    
    bus.attach_peripheral(adc0);
    bus.attach_peripheral(comparator);
    AvrCpu cpu {bus};

    // Firmware steps:
    // 1. Setup ADMUX (Channel 0)
    // 2. Setup ADCSRB (ADTS=1 -> Analog Comparator)
    // 3. Setup ADCSRA (ADEN=1, ADATE=1)
    // 4. Poll ADCSRA until ADIF set
    // 5. Read result
    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16U, 0x00U),                     // 0
            encode_sts(16U), atmega328.adc.admux_address, // 1, 2
            encode_ldi(16U, 0x01U),                     // 3
            encode_sts(16U), atmega328.adc.adcsrb_address, // 4, 5
            encode_ldi(17U, 0xA0U),                     // 6 (ADEN | ADATE)
            encode_sts(17U), atmega328.adc.adcsra_address, // 7, 8
            encode_lds(18U), atmega328.adc.adcsra_address, // 9, 10
            encode_lds(19U), atmega328.adc.adcsra_address, // 11, 12
            encode_lds(20U), atmega328.adc.adcsra_address, // 13, 14
            encode_lds(21U), atmega328.adc.adcl_address,   // 15, 16
            encode_lds(22U), atmega328.adc.adch_address,   // 17, 18
            0x0000U
        },
        .entry_word = 0U
    });

    cpu.reset();

    SUBCASE("Triggering conversion via comparator edge") {
        // Run setup (6 instructions: LDI, STS, LDI, STS, LDI, STS)
        // Cycles: 1 + 2 + 1 + 2 + 1 + 2 = 9
        cpu.run(9);
        CHECK(cpu.cycles() == 9U);
        CHECK(cpu.program_counter() == 9U);
        
        // LDS r18, ADCSRA
        cpu.step(); // 2 cycles
        // ADEN=1, ADATE=1, ADSC=0 (not triggered yet), ADIF=0
        CHECK((cpu.snapshot().gpr[18] & 0xF0U) == 0xA0U);

        // Raise comparator input to trigger edge
        comparator.set_positive_input_voltage(0.83); // ACO becomes 1 -> Trigger ADC
        
        // LDS r19, ADCSRA
        cpu.step(); // 2 cycles
        // ADSC should be set now (triggered)
        CHECK((cpu.snapshot().gpr[19] & 0x40U) != 0U);

        // Tick to complete conversion (4 cycles)
        // We already ticked 2 during LDS r19.
        cpu.step(); // LDS r20, ADCSRA (2 cycles)
        const auto s2 = cpu.snapshot();
        // ADIF should be set now
        CHECK((s2.gpr[20] & 0x10U) != 0U);

        // Read results
        cpu.step(); // LDS r21, ADCL
        cpu.step(); // LDS r22, ADCH
        const auto s3 = cpu.snapshot();
        const auto result = static_cast<vioavr::core::u16>(
            s3.gpr[21] | (static_cast<vioavr::core::u16>(s3.gpr[22]) << 8U)
        );
        // 0.75V -> 768
        CHECK(result >= 766U);
        CHECK(result <= 769U);
    }
}
