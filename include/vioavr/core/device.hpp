#pragma once

#include "vioavr/core/types.hpp"

#include <array>
#include <string_view>

namespace vioavr::core {

enum class AdcAutoTriggerSource : u8 {
    free_running = 0,
    analog_comparator = 1,
    external_interrupt_0 = 2,
    timer0_compare = 3,
    timer0_overflow = 4,
    timer1_compare_b = 5,
    timer1_overflow = 6,
    timer1_capture = 7,
    unsupported = 0xFFU
};

struct AdcDescriptor {
    u16 adcl_address {};
    u16 adch_address {};
    u16 adcsra_address {};
    u16 adcsrb_address {};
    u16 admux_address {};
    u8 vector_index {};
    std::array<AdcAutoTriggerSource, 8> auto_trigger_map {};
};

struct Timer8Descriptor {
    u16 tcnt_address {};
    u16 ocra_address {};
    u16 ocrb_address {};
    u16 tifr_address {};
    u16 timsk_address {};
    u16 tccra_address {};
    u16 tccrb_address {};
    u16 assr_address {};
    u8 compare_a_vector_index {};
    u8 compare_b_vector_index {};
    u8 overflow_vector_index {};
    u8 compare_a_enable_mask {};
    u8 compare_b_enable_mask {};
    u8 overflow_enable_mask {};
    u16 t0_pin_address {};
    u8 t0_pin_bit {};
    u16 ocra_pin_address {};
    u8 ocra_pin_bit {};
    u16 ocrb_pin_address {};
    u8 ocrb_pin_bit {};
    u16 tosc1_pin_address {};
    u8 tosc1_pin_bit {};
    u16 tosc2_pin_address {};
    u8 tosc2_pin_bit {};
};

struct ExtInterruptDescriptor {
    u16 eicra_address {};
    u16 eimsk_address {};
    u16 eifr_address {};
    u8 int0_vector_index {};
};

struct Uart0Descriptor {
    u16 udr_address {};
    u16 ucsra_address {};
    u16 ucsrb_address {};
    u16 ucsrc_address {};
    u8 rx_vector_index {};
    u8 udre_vector_index {};
    u8 tx_vector_index {};
};

struct PinChangeInterruptDescriptor {
    u16 pcicr_address {};
    u16 pcifr_address {};
    u16 pcmsk_address {};
    u8 pcicr_enable_mask {};
    u8 pcifr_flag_mask {};
    u8 vector_index {};
};

struct SpiDescriptor {
    u16 spcr_address {};
    u16 spsr_address {};
    u16 spdr_address {};
    u8 vector_index {};
};

struct TwiDescriptor {
    u16 twbr_address {};
    u16 twsr_address {};
    u16 twar_address {};
    u16 twdr_address {};
    u16 twcr_address {};
    u16 twamr_address {};
    u8 vector_index {};
};

struct EepromDescriptor {
    u16 eecr_address {};
    u16 eedr_address {};
    u16 eearl_address {};
    u16 eearh_address {};
    u8 vector_index {};
};

struct WdtDescriptor {
    u16 wdtcsr_address {};
    u8 vector_index {};
};

struct Timer16Descriptor {
    u16 tcnt_address {};
    u16 ocra_address {};
    u16 ocrb_address {};
    u16 icr_address {};
    u16 tifr_address {};
    u16 timsk_address {};
    u16 tccra_address {};
    u16 tccrb_address {};
    u16 tccrc_address {};
    u8 capture_vector_index {};
    u8 compare_a_vector_index {};
    u8 compare_b_vector_index {};
    u8 overflow_vector_index {};
    u8 capture_enable_mask {};
    u8 compare_a_enable_mask {};
    u8 compare_b_enable_mask {};
    u8 overflow_enable_mask {};
    u16 t1_pin_address {};
    u8 t1_pin_bit {};
    u16 icp1_pin_address {};
    u8 icp1_pin_bit {};
};

struct PortDescriptor {
    std::string_view name {};
    u16 pin_address {};
    u16 ddr_address {};
    u16 port_address {};
};

struct DeviceDescriptor {
    std::string_view name {};
    u32 flash_words {};
    u16 sram_bytes {};
    u16 eeprom_bytes {};
    u8 interrupt_vector_count {};
    u8 interrupt_vector_size {2U};
    AddressRange register_file_range {0x0000, 0x001F};
    AddressRange io_range {0x0020, 0x005F};
    AddressRange extended_io_range {0x0060, 0x00FF};
    u16 spl_address {0x005DU};
    u16 sph_address {0x005EU};
    u16 sreg_address {0x005FU};
    u64 cpu_frequency_hz {16'000'000U};
    AdcDescriptor adc {};
    Timer8Descriptor timer0 {};
    Timer8Descriptor timer2 {};
    Timer16Descriptor timer1 {};
    ExtInterruptDescriptor ext_interrupt {};
    Uart0Descriptor uart0 {};
    PinChangeInterruptDescriptor pin_change_interrupt_0 {};
    PinChangeInterruptDescriptor pin_change_interrupt_1 {};
    PinChangeInterruptDescriptor pin_change_interrupt_2 {};
    SpiDescriptor spi {};
    TwiDescriptor twi {};
    EepromDescriptor eeprom {};
    WdtDescriptor wdt {};
    std::array<PortDescriptor, 8> ports {}; // Max 8 ports (A..H)

    [[nodiscard]] constexpr u16 data_end_address() const noexcept
    {
        return static_cast<u16>(extended_io_range.end + sram_bytes);
    }

    [[nodiscard]] constexpr AddressRange sram_range() const noexcept
    {
        return {
            static_cast<u16>(extended_io_range.end + 1U),
            data_end_address()
        };
    }
};

}  // namespace vioavr::core
