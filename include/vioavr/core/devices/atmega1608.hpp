#pragma once
#include "vioavr/core/device.hpp"
namespace vioavr::core::devices {
inline constexpr DeviceDescriptor atmega1608 {
    .name = "ATmega1608",
    .flash_words = 0U, .sram_bytes = 2048U, .eeprom_bytes = 256U,
    .interrupt_vector_count = 36U, .interrupt_vector_size = 2U,
    .adc = { .adcl_address = 0x0U, .adch_address = 0x0U, .adcsra_address = 0x0U, .adcsrb_address = 0x0U, .admux_address = 0x0U, .vector_index = 0U },
    .timer0 = { .tcnt_address = 0x0U, .ocra_address = 0x0U, .ocrb_address = 0x0U, .tifr_address = 0x0U, .timsk_address = 0x0U, .tccra_address = 0x0U, .tccrb_address = 0x0U, .assr_address = 0x0U, .compare_a_vector_index = 0U, .compare_b_vector_index = 0U, .overflow_vector_index = 0U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U },
    .timer2 = { .tcnt_address = 0x0U, .ocra_address = 0x0U, .ocrb_address = 0x0U, .tifr_address = 0x0U, .timsk_address = 0x0U, .tccra_address = 0x0U, .tccrb_address = 0x0U, .assr_address = 0x0U, .compare_a_vector_index = 0U, .compare_b_vector_index = 0U, .overflow_vector_index = 0U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U },
    .timer1 = { .tcnt_address = 0x0U, .ocra_address = 0x0U, .ocrb_address = 0x0U, .icr_address = 0x0U, .tifr_address = 0x0U, .timsk_address = 0x0U, .tccra_address = 0x0U, .tccrb_address = 0x0U, .tccrc_address = 0x0U, .capture_vector_index = 0U, .compare_a_vector_index = 0U, .compare_b_vector_index = 0U, .overflow_vector_index = 0U, .capture_enable_mask = 0x20U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U },
    .ext_interrupt = { .eicra_address = 0x0U, .eimsk_address = 0x0U, .eifr_address = 0x0U, .int0_vector_index = 0U },
    .uart0 = { .udr_address = 0x0U, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .rx_vector_index = 0U, .udre_vector_index = 0U, .tx_vector_index = 0U },
    .pin_change_interrupt_0 = { .pcicr_address = 0x0U, .pcifr_address = 0x0U, .pcmsk_address = 0x0U, .pcicr_enable_mask = 0x1U, .pcifr_flag_mask = 0x1U, .vector_index = 0U },
    .pin_change_interrupt_1 = { .pcicr_address = 0x0U, .pcifr_address = 0x0U, .pcmsk_address = 0x0U, .pcicr_enable_mask = 0x2U, .pcifr_flag_mask = 0x2U, .vector_index = 0U },
    .pin_change_interrupt_2 = { .pcicr_address = 0x0U, .pcifr_address = 0x0U, .pcmsk_address = 0x0U, .pcicr_enable_mask = 0x4U, .pcifr_flag_mask = 0x4U, .vector_index = 0U },
    .spi = { .spcr_address = 0x0U, .spsr_address = 0x0U, .spdr_address = 0x0U, .vector_index = 0U },
    .twi = { .twbr_address = 0x0U, .twsr_address = 0x0U, .twar_address = 0x0U, .twdr_address = 0x0U, .twcr_address = 0x0U, .twamr_address = 0x0U, .vector_index = 14U },
    .eeprom = { .eecr_address = 0x0U, .eedr_address = 0x0U, .eearl_address = 0x0U, .eearh_address = 0x0U, .vector_index = 0U },
    .wdt = { .wdtcsr_address = 0x0U, .vector_index = 0U },
    .ports = {{
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U }
    }},
};
} // namespace vioavr::core::devices
