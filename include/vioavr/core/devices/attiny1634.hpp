#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor attiny1634 {
    .name = "ATtiny1634",
    .flash_words = 8192U,
    .sram_bytes = 1024U,
    .sram_start = 0x0100U,
    .eeprom_bytes = 256U,
    .interrupt_vector_count = 28U,
    .interrupt_vector_size = 2U,
    .flash_page_size = 0x20U,
    .register_file_range = { 0x0000, 0x001F },
    .io_range = { 0x0020, 0x005F },
    .extended_io_range = { 0x0060, 0x00FF },

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
            .adcsra_address = 0x23U,
            .adcsrb_address = 0x22U,
            .admux_address = 0x24U,
            .vector_index = 14U,
            .didr0_address = 0x60U,
        }
    }},

    .ac_count = 1U,
    .acs = {{
        {
            .acsr_address = 0x25U,
        }
    }},

    .timer8_count = 1U,
    .timers8 = {{
        {
            .tcnt_address = 0x39U,
            .ocra_address = 0x38U,
            .ocrb_address = 0x37U,
            .tccra_address = 0x3BU,
            .tccrb_address = 0x3AU,
            .compare_a_vector_index = 10U,
            .compare_b_vector_index = 11U,
            .overflow_vector_index = 12U,
            .timer_index = 0U,
        }
    }},

    .timer16_count = 1U,
    .timers16 = {{
        {
            .tcnt_address = 0x6EU,
            .ocra_address = 0x6CU,
            .ocrb_address = 0x6AU,
            .icr_address = 0x68U,
            .tifr_address = 0x59U,
            .timsk_address = 0x5AU,
            .tccra_address = 0x72U,
            .tccrb_address = 0x71U,
            .tccrc_address = 0x70U,
            .capture_vector_index = 6U,
            .compare_a_vector_index = 7U,
            .compare_b_vector_index = 8U,
            .overflow_vector_index = 9U,
        }
    }},

    .ext_interrupt_count = 1U,
    .ext_interrupts = {{
        {
            .vector_indices = { 1U, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
        }
    }},

    .uart_count = 2U,
    .uarts = {{
        {
            .udr_address = 0x40U,
            .ucsra_address = 0x46U,
            .ucsrb_address = 0x45U,
            .ucsrc_address = 0x44U,
        },
        {
        }
    }},

    .twi_count = 1U,
    .twis = {{
        {
        }
    }},

    .usi_count = 1U,
    .usis = {{
        {
            .usicr_address = 0x4AU,
            .usisr_address = 0x4BU,
            .usidr_address = 0x4CU,
            .usibr_address = 0x4DU,
            .start_vector_index = 23U,
            .overflow_vector_index = 24U,
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
            .wdtcsr_address = 0x50U,
            .vector_index = 5U,
        }
    }},

    .signature = { 0x1E, 0x94, 0x12 },

    .operating_voltage_v = 5.0,
    .vil_factor = 0.3,
    .vih_factor = 0.6,

    .port_count = 3U,
    .ports = {{
        { "PORTB", 0x2BU, 0x2CU, 0x2DU,
          0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
          0x00U, 0x00U, 0xFFFFU, 255U }
,
        { "PORTC", 0x27U, 0x28U, 0x29U,
          0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
          0x00U, 0x00U, 0xFFFFU, 255U }
,
        { "PORTA", 0x2FU, 0x30U, 0x31U,
          0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
          0x00U, 0x00U, 0xFFFFU, 255U }
    }}
};

} // namespace vioavr::core::devices
