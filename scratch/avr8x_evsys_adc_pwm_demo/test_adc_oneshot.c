#include <avr/io.h>

int main(void)
{
    PORTC_DIR = 0xFF;   // All PORTC as output

    // Enable ADC
    ADC0_CTRLA = ADC_ENABLE_bm;
    ADC0_CTRLB = ADC_PRESC_DIV8_gc;
    ADC0_CTRLC = ADC_REFSEL_VDDREF_gc;
    ADC0_MUXPOS = ADC_MUXPOS_AIN0_gc;

    // Start conversion
    ADC0_COMMAND = ADC_STCONV_bm;

    // Wait for RESRDY
    while (!(ADC0_INTFLAGS & ADC_RESRDY_bm));

    // Output low byte of result to PORTC
    PORTC_OUT = ADC0_RESL;

    // Lock up
    while (1);
}
