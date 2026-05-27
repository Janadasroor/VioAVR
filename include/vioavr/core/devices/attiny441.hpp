#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor attiny441 {
    .name = "ATtiny441",
    .flash_words = 2048U,
    .sram_bytes = 256U,
    .sram_start = 0x0100U,
    .eeprom_bytes = 256U,
    .interrupt_vector_count = 30U,
    .interrupt_vector_size = 2U,
    .flash_page_size = 0x10U,
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
            .adcsra_address = 0x25U,
            .adcsrb_address = 0x24U,
            .admux_address = 0x29U,
            .vector_index = 13U,
            .didr0_address = 0x60U,
        }
    }},

    .ac_count = 1U,
    .acs = {{
        {
        }
    }},

    .timer8_count = 1U,
    .timers8 = {{
        {
            .tcnt_address = 0x52U,
            .ocra_address = 0x56U,
            .ocrb_address = 0x5CU,
            .tifr_address = 0x58U,
            .timsk_address = 0x59U,
            .tccra_address = 0x50U,
            .tccrb_address = 0x53U,
            .compare_a_vector_index = 9U,
            .compare_b_vector_index = 10U,
            .overflow_vector_index = 11U,
            .timer_index = 0U,
        }
    }},

    .timer16_count = 2U,
    .timers16 = {{
        {
            .tcnt_address = 0x4CU,
            .ocra_address = 0x4AU,
            .ocrb_address = 0x48U,
            .icr_address = 0x44U,
            .tifr_address = 0x2EU,
            .timsk_address = 0x2FU,
            .tccra_address = 0x4FU,
            .tccrb_address = 0x4EU,
            .tccrc_address = 0x42U,
            .capture_vector_index = 5U,
            .compare_a_vector_index = 6U,
            .compare_b_vector_index = 7U,
            .overflow_vector_index = 8U,
        },
        {
            .capture_vector_index = 16U,
            .compare_a_vector_index = 17U,
            .compare_b_vector_index = 18U,
            .overflow_vector_index = 19U,
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
        },
        {
            .udr_address = 0x80U,
            .ucsra_address = 0x86U,
            .ucsrb_address = 0x85U,
            .ucsrc_address = 0x84U,
        }
    }},

    .spi_count = 1U,
    .spis = {{
        {
            .spcr_address = 0xB2U,
            .spsr_address = 0xB1U,
            .spdr_address = 0xB0U,
        }
    }},

    .twi_count = 1U,
    .twis = {{
        {
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
            .vector_index = 4U,
        }
    }},

    .signature = { 0x1E, 0x92, 0x15 },

    .operating_voltage_v = 5.0,
    .vil_factor = 0.3,
    .vih_factor = 0.6,

    .port_count = 2U,
    .ports = {{
        { "PORTB", 0x36U, 0x37U, 0x38U,
          0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
          0x00U, 0x00U, 0xFFFFU, 255U }
,
        { "PORTA", 0x39U, 0x3AU, 0x3BU,
          0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
          0x00U, 0x00U, 0xFFFFU, 255U }
    }}
};

} // namespace vioavr::core::devices
