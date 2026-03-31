#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/ext_interrupt.hpp"
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

TEST_CASE("ADC External Interrupt Auto-Trigger Firmware Integration Test")
{
    using vioavr::core::Adc;
    using vioavr::core::AvrCpu;
    using vioavr::core::ExtInterrupt;
    using vioavr::core::HexImage;
    using vioavr::core::MemoryBus;
    using vioavr::core::devices::atmega328;

    MemoryBus bus {atmega328};
    Adc adc0 {"ADC0", atmega328, 6U, 4U};
    ExtInterrupt exti {"EXTINT", atmega328, 4U};
    
    adc0.connect_external_interrupt_0_auto_trigger(exti);
    adc0.set_channel_voltage(0U, 0.42); // Expected result: 430
    
    bus.attach_peripheral(adc0);
    bus.attach_peripheral(exti);
    AvrCpu cpu {bus};

    // Firmware steps:
    // 1. Setup ADMUX
    // 2. Setup ADCSRB (ADTS=2 -> Ext Int 0)
    // 3. Setup EICRA (Falling edge)
    // 4. Setup ADCSRA (ADEN=1, ADATE=1)
    // 5. LDS r18, ADCSRA (before trigger)
    // 6. LDS r19, ADCSRA (after trigger)
    // 7. LDS r20, ADCL
    // 8. LDS r21, ADCH
    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16U, 0x00U),                     // 0
            encode_sts(16U), atmega328.adc.admux_address, // 1, 2
            encode_ldi(16U, 0x02U),                     // 3
            encode_sts(16U), atmega328.adc.adcsrb_address, // 4, 5
            encode_ldi(16U, 0x02U),                     // 6 (Falling edge)
            encode_sts(16U), atmega328.ext_interrupt.eicra_address, // 7, 8
            encode_ldi(17U, 0xA0U),                     // 9 (ADEN | ADATE)
            encode_sts(17U), atmega328.adc.adcsra_address, // 10, 11
            encode_lds(18U), atmega328.adc.adcsra_address, // 12, 13
            encode_lds(19U), atmega328.adc.adcsra_address, // 14, 15
            encode_lds(20U), atmega328.adc.adcl_address,   // 16, 17
            encode_lds(21U), atmega328.adc.adch_address,   // 18, 19
            0x0000U
        },
        .entry_word = 0U
    });

    cpu.reset();

    SUBCASE("Triggering conversion via INT0 falling edge in firmware") {
        // Run setup (8 instructions: LDI, STS x 4)
        // Cycles: 1+2 + 1+2 + 1+2 + 1+2 = 12
        cpu.run(12);
        CHECK(cpu.cycles() == 12U);
        CHECK(cpu.program_counter() == 12U);
        
        // LDS r18, ADCSRA
        cpu.step(); // 2 cycles
        // ADEN=1, ADATE=1, ADSC=0, ADIF=0
        CHECK((cpu.snapshot().gpr[18] & 0xF0U) == 0xA0U);

        // Trigger INT0 falling edge
        exti.set_int0_level(true);
        exti.set_int0_level(false); // Trigger!
        
        // LDS r19, ADCSRA
        cpu.step(); // 2 cycles
        // ADSC should be set
        CHECK((cpu.snapshot().gpr[19] & 0x40U) != 0U);

        // Tick to complete conversion (4 cycles)
        // We already ticked 2 during LDS r19.
        cpu.step(); // LDS r20, ADCL (2 cycles)
        // ADIF should be set now (checked after conversion)
        
        cpu.step(); // LDS r21, ADCH
        const auto snapshot = cpu.snapshot();
        const auto result = static_cast<vioavr::core::u16>(
            snapshot.gpr[20] | (static_cast<vioavr::core::u16>(snapshot.gpr[21]) << 8U)
        );
        
        CHECK(result >= 429U);
        CHECK(result <= 431U);
        CHECK(bus.read_data(atmega328.ext_interrupt.eifr_address) == 0x01U);
    }
}
