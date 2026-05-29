/*
 * avr_adc_bridge demo firmware for atmega3209
 *
 * Reads ADC channel 0 and drives PORTA bit 0 based on threshold.
 * ADC result > 512 (~2.5V with Vref=5V) -> PA0 HIGH
 * ADC result < 512 -> PA0 LOW
 *
 * Intended for co-simulation with avr_adc_bridge.injecting voltage
 * into analog signal bank channel 0.
 */

#include <avr/io.h>

/* Simple delay loop (~50 cycles per iteration with -Os) */
static void delay(unsigned int count)
{
    /* Use register-only loop to avoid stack frame issues */
    __asm__ volatile (
        "1: subi %A0, 1"  "\n\t"
        "   sbci %B0, 0"  "\n\t"
        "   brne 1b"
        : "+r" (count)
        :
        : "cc"
    );
}

int main(void)
{
    /* Set PORTA bit 0 as output */
    PORTA_DIR |= PIN0_bm;

    /* Enable ADC, Vref = VDD, prescaler = 64 */
    ADC0_CTRLA = ADC_ENABLE_bm;
    ADC0_CTRLB = ADC_PRESC_DIV64_gc;
    /* Channel 0 = AIN0 (PA0) by default, MUXPOS = 0 */

    while (1) {
        /* Start conversion */
        ADC0_COMMAND = ADC_STCONV_bm;

        /* Wait for conversion complete */
        while (!(ADC0_INTFLAGS & ADC_RESRDY_bm)) {}

        /* Read result */
        uint16_t result = ADC0_RES;

        /* Drive PA0 based on threshold */
        if (result > 512) {
            PORTA_OUT |= PIN0_bm;   /* HIGH */
        } else {
            PORTA_OUT &= ~PIN0_bm;  /* LOW */
        }

        /* Small delay to let ngspice time steps accumulate */
        delay(100);
    }
}
