#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor attiny167 {
    .name = "ATtiny167",
    .flash_words = 8192U,
    .sram_bytes = 512U,
    .sram_start = 0x0100U,
    .eeprom_bytes = 512U,
    .interrupt_vector_count = 20U,
    .interrupt_vector_size = 2U,
    .flash_page_size = 0x80U,
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
            .adcsra_address = 0x7AU,
            .adcsrb_address = 0x7BU,
            .admux_address = 0x7CU,
            .vector_index = 15U,
            .didr0_address = 0x7EU,
        }
    }},

    .ac_count = 1U,
    .acs = {{
        {
            .acsr_address = 0x50U,
        }
    }},

    .timer16_count = 1U,
    .timers16 = {{
        {
            .tcnt_address = 0x84U,
            .ocra_address = 0x88U,
            .ocrb_address = 0x8AU,
            .icr_address = 0x86U,
            .tifr_address = 0x36U,
            .timsk_address = 0x6FU,
            .tccra_address = 0x80U,
            .tccrb_address = 0x81U,
            .tccrc_address = 0x82U,
            .tccrd_address = 0x83U,
            .capture_vector_index = 6U,
            .compare_a_vector_index = 7U,
            .compare_b_vector_index = 8U,
            .overflow_vector_index = 9U,
        }
    }},

    .ext_interrupt_count = 1U,
    .ext_interrupts = {{
        {
            .eicra_address = 0x69U,
            .eimsk_address = 0x3DU,
            .eifr_address = 0x3CU,
            .vector_indices = { 1U, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
        }
    }},

    .spi_count = 1U,
    .spis = {{
        {
            .spcr_address = 0x4CU,
            .spsr_address = 0x4DU,
            .spdr_address = 0x4EU,
        }
    }},

    .usi_count = 1U,
    .usis = {{
        {
            .usicr_address = 0xB8U,
            .usisr_address = 0xB9U,
            .usidr_address = 0xBAU,
            .usibr_address = 0xBBU,
            .start_vector_index = 18U,
            .overflow_vector_index = 19U,
        }
    }},

    .eeprom_count = 1U,
    .eeproms = {{
        {
            .eecr_address = 0x3FU,
            .eedr_address = 0x40U,
            .eearl_address = 0x41U,
            .size = 0x200U,
        }
    }},

    .wdt_count = 1U,
    .wdts = {{
        {
            .wdtcsr_address = 0x60U,
            .vector_index = 5U,
        }
    }},

    .signature = { 0x1E, 0x94, 0x87 },

    .operating_voltage_v = 5.0,
    .vil_factor = 0.3,
    .vih_factor = 0.6,

    .port_count = 2U,
    .ports = {{
        { "PORTA", 0x20U, 0x21U, 0x22U,
          0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
          0x00U, 0x00U, 0xFFFFU, 255U }
,
        { "PORTB", 0x23U, 0x24U, 0x25U,
          0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
          0x00U, 0x00U, 0xFFFFU, 255U }
    }}
};

} // namespace vioavr::core::devices
