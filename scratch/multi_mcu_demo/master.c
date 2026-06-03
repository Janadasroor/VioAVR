/*
 * Multi-MCU Co-Simulation: Master Firmware
 *
 * ATmega4809 reads ADC (AIN0 via avr_adc_bridge), transmits 8-bit value
 * over TWI/I2C bus to slave at address 0x42.
 *
 * TWI0 on default pins: SDA=PA2 (ext ID 2), SCL=PA3 (ext ID 3)
 *
 * Build: avr-gcc -mmcu=atmega4809 -Os -DF_CPU=16000000UL -o master.elf master.c
 *
 * 2026-06-03
 */

#include <avr/io.h>
#include <util/delay.h>

#define SLAVE_ADDR 0x42

int main(void)
{
    /* --- ADC0: single-shot, 8-bit, VDD reference, AIN0 --- */
    ADC0.CTRLA  = ADC_ENABLE_bm;              /* Enable ADC, free-running off */
    ADC0.CTRLB  = ADC_RESSEL_10BIT_gc;        /* 10-bit resolution */
    ADC0.CTRLC  = ADC_REFSEL_VDDREF_gc;       /* VDD as reference (5V in cosim) */
    ADC0.MUXPOS = 0;                          /* AIN0 = signal bank channel 0 */

    /* --- TWI0: master at ~100 kHz --- */
    /* MBAUD = F_CPU / (2 * F_SCL) - 5  = 16000000/(2*100000) - 5 = 75 */
    TWI0.MBAUD  = 75;
    TWI0.MCTRLA = TWI_ENABLE_bm;              /* Enable master */
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;      /* Force IDLE (model doesn't auto-transition) */

    uint8_t value = 0;

    for (;;) {
        /* --- Trigger ADC conversion --- */
        ADC0.COMMAND = ADC_STCONV_bm;          /* Start conversion */
        while (!(ADC0.INTFLAGS & ADC_RESRDY_bm));

        value = (uint8_t)(ADC0.RES >> 2);     /* 10-bit RES, take top 8 bits */

        ADC0.INTFLAGS = ADC_RESRDY_bm;        /* Clear flag */

        /* --- I2C master write: 1 data byte to slave --- */
        TWI0.MADDR = (SLAVE_ADDR << 1) | 0;       /* SLA+W = 0x84 */
        while (!(TWI0.MSTATUS & TWI_WIF_bm));      /* Wait for address sent */

        if (TWI0.MSTATUS & TWI_RXACK_bm) {
            /* Slave NACKed — send STOP and retry next cycle */
            TWI0.MCTRLB = TWI_MCMD_STOP_gc;
        } else {
            /* Slave ACKed — send data */
            TWI0.MDATA = value;
            while (!(TWI0.MSTATUS & TWI_WIF_bm));  /* Wait for data sent */
            TWI0.MCTRLB = TWI_MCMD_STOP_gc;        /* Send STOP */
        }

        _delay_ms(2);   /* ~500 Hz update rate (sim window is 5ms) */
    }
}
