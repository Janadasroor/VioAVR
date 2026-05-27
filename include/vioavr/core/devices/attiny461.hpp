#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor attiny461 {
    .name = "ATtiny461",
    .flash_words = 2048U,
    .sram_bytes = 256U,
    .sram_start = 0x0060U,
    .eeprom_bytes = 256U,
    .interrupt_vector_count = 19U,
    .interrupt_vector_size = 2U,
    .flash_page_size = 0x40U,
    .register_file_range = { 0x0000, 0x001F },
    .io_range = { 0x0020, 0x005F },
    .extended_io_range = { 0x0060, 0x005F },

    .spl_address = 0x5DU,
    .sph_address = 0x5EU,
    .sreg_address = 0x5FU,
    .spmcsr_address = 0x57U,
    .sigrd_mask = 0x20U,
    .spmen_mask = 0x01U,
    .cpu_frequency_hz = 16'000'000U,

    .adc_count = 1U,
    .adcs = {{
        {
            .adcsra_address = 0x26U,
            .adcsrb_address = 0x23U,
            .admux_address = 0x27U,
            .vector_index = 11U,
            .didr0_address = 0x21U,
        }
    }},

    .ac_count = 1U,
    .acs = {{
        {
            .acsr_address = 0x29U,
        }
    }},

    .timer8_count = 1U,
    .timers8 = {{
        {
            .tcnt_address = 0x34U,
            .ocra_address = 0x33U,
            .ocrb_address = 0x32U,
            .tccra_address = 0x35U,
            .tccrb_address = 0x53U,
            .compare_a_vector_index = 14U,
            .compare_b_vector_index = 15U,
            .overflow_vector_index = 6U,
            .timer_index = 0U,
        }
    }},

    .timer16_count = 1U,
    .timers16 = {{
        {
            .tcnt_address = 0x4EU,
            .ocra_address = 0x4DU,
            .ocrb_address = 0x4CU,
            .ocrc_address = 0x4BU,
            .tifr_address = 0x58U,
            .timsk_address = 0x59U,
            .tccra_address = 0x50U,
            .tccrb_address = 0x4FU,
            .tccrc_address = 0x47U,
            .tccrd_address = 0x46U,
            .tccre_address = 0x20U,
            .tc1h_address = 0x45U,
            .ocrd_address = 0x4AU,
            .dt1_address = 0x44U,
            .compare_a_vector_index = 3U,
            .compare_b_vector_index = 4U,
            .compare_d_vector_index = 17U,
            .overflow_vector_index = 5U,
        }
    }},

    .ext_interrupt_count = 1U,
    .ext_interrupts = {{
        {
            .vector_indices = { 1U, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
        }
    }},

    .usi_count = 1U,
    .usis = {{
        {
            .usicr_address = 0x2DU,
            .usisr_address = 0x2EU,
            .usidr_address = 0x2FU,
            .usibr_address = 0x30U,
            .start_vector_index = 7U,
            .overflow_vector_index = 8U,
            .sda_pin_address = 0x3BU,
            .sda_pin_bit = 0U,
            .scl_pin_address = 0x3BU,
            .scl_pin_bit = 2U,
            .do_pin_address = 0x3BU,
            .do_pin_bit = 1U,
        }
    }},

    .eeprom_count = 1U,
    .eeproms = {{
        {
            .eecr_address = 0x3CU,
            .eedr_address = 0x3DU,
            .eearl_address = 0x3EU,
            .size = 0x100U,
        }
    }},

    .wdt_count = 1U,
    .wdts = {{
        {
            .wdtcsr_address = 0x41U,
            .vector_index = 12U,
        }
    }},

    .signature = { 0x1E, 0x92, 0x08 },

    .operating_voltage_v = 5.0,
    .vil_factor = 0.3,
    .vih_factor = 0.6,

    .port_count = 2U,
    .ports = {{
        { "PORTA", 0x39U, 0x3AU, 0x3BU,
          0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
          0x00U, 0x00U, 0xFFFFU, 255U }
,
        { "PORTB", 0x36U, 0x37U, 0x38U,
          0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
          0x00U, 0x00U, 0xFFFFU, 255U }
    }}
};

} // namespace vioavr::core::devices
