#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor attiny20 {
    .name = "ATtiny20",
    .flash_words = 1024U,
    .sram_bytes = 128U,
    .sram_start = 0x0040U,
    .eeprom_bytes = 0U,
    .interrupt_vector_count = 17U,
    .interrupt_vector_size = 2U,
    .flash_page_size = 0x80U,
    .register_file_range = { 0x0000, 0x001F },
    .io_range = { 0x0020, 0x005F },
    .extended_io_range = { 0x0060, 0x005F },

    .spl_address = 0x5DU,
    .sph_address = 0x5EU,
    .sreg_address = 0x5FU,
    .sigrd_mask = 0x20U,
    .spmen_mask = 0x01U,
    .cpu_frequency_hz = 16'000'000U,

    .adc_count = 1U,
    .adcs = {{
        {
            .adcsra_address = 0x12U,
            .adcsrb_address = 0x11U,
            .admux_address = 0x10U,
            .didr0_address = 0x0DU,
        }
    }},

    .ac_count = 1U,
    .acs = {{
        {
            .acsr_address = 0x13U,
        }
    }},

    .timer8_count = 1U,
    .timers8 = {{
        {
            .tcnt_address = 0x17U,
            .ocra_address = 0x16U,
            .ocrb_address = 0x15U,
            .tccra_address = 0x19U,
            .tccrb_address = 0x18U,
            .timer_index = 0U,
        }
    }},

    .timer16_count = 1U,
    .timers16 = {{
        {
            .tcnt_address = 0x20U,
            .ocra_address = 0x1EU,
            .ocrb_address = 0x1CU,
            .icr_address = 0x1AU,
            .tifr_address = 0x25U,
            .timsk_address = 0x26U,
            .tccra_address = 0x24U,
            .tccrb_address = 0x23U,
            .tccrc_address = 0x22U,
        }
    }},

    .ext_interrupt_count = 1U,
    .ext_interrupts = {{
        {
            .vector_indices = { 1U, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
        }
    }},

    .spi_count = 1U,
    .spis = {{
        {
            .spcr_address = 0x30U,
            .spsr_address = 0x2FU,
            .spdr_address = 0x2EU,
        }
    }},

    .twi_count = 1U,
    .twis = {{
        {
        }
    }},

    .wdt_count = 1U,
    .wdts = {{
        {
            .wdtcsr_address = 0x31U,
            .vector_index = 4U,
        }
    }},

    .signature = { 0x1E, 0x91, 0x0F },

    .operating_voltage_v = 5.0,
    .vil_factor = 0.3,
    .vih_factor = 0.6,

    .port_count = 2U,
    .ports = {{
        { "PORTB", 0x24U, 0x25U, 0x26U,
          0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
          0x00U, 0x00U, 0xFFFFU, 255U }
,
        { "PORTA", 0x20U, 0x21U, 0x22U,
          0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
          0x00U, 0x00U, 0xFFFFU, 255U }
    }}
};

} // namespace vioavr::core::devices
