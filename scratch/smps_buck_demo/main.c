#include <avr/io.h>
#include <avr/interrupt.h>

ISR(ADC0_RESRDY_vect)
{
    int adc = ADC0_RES;
    int error = (int)675 - adc;
    int corr = error / 8;
    int duty = 42 + corr;
    if (duty < 0) duty = 0;
    if (duty > 159) duty = 159;
    TCA0_SINGLE_CMP0 = (unsigned int)duty;
    ADC0_COMMAND = ADC_STCONV_bm;
}

int main(void)
{
    PORTB_DIR |= PIN0_bm;
    PORTC_DIR |= PIN0_bm;

    PORTMUX_TCAROUTEA = PORTMUX_TCA0_PORTB_gc;

    TCA0_SINGLE_PER = 159;
    TCA0_SINGLE_CMP0 = 42;
    TCA0_SINGLE_CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc | TCA_SINGLE_CMP0EN_bm;
    TCA0_SINGLE_CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;

    ADC0_CTRLA = ADC_ENABLE_bm;
    ADC0_CTRLB = ADC_PRESC_DIV8_gc;
    ADC0_CTRLC = ADC_REFSEL_VDDREF_gc;
    ADC0_MUXPOS = ADC_MUXPOS_AIN0_gc;
    ADC0_INTCTRL = ADC_RESRDY_bm;
    ADC0_COMMAND = ADC_STCONV_bm;

    sei();

    for (;;) {}
}
