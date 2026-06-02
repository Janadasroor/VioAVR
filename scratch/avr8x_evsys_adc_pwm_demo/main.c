#include <avr/io.h>
#include <avr/interrupt.h>

int main(void)
{
    PORTB_DIR |= PIN0_bm;
    PORTC_DIR |= PIN0_bm;

    PORTMUX_TCAROUTEA = PORTMUX_TCA0_PORTB_gc;

    TCA0_SINGLE_PER = 255;
    TCA0_SINGLE_CMP0 = 128;
    TCA0_SINGLE_CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc | TCA_SINGLE_CMP0EN_bm;
    TCA0_SINGLE_CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;

    EVSYS_CHANNEL0 = EVSYS_GENERATOR_TCA0_OVF_LUNF_gc;
    EVSYS_USERADC0 = EVSYS_CHANNEL_CHANNEL0_gc;

    ADC0_CTRLA = ADC_ENABLE_bm;
    ADC0_CTRLB = ADC_PRESC_DIV8_gc;
    ADC0_CTRLC = ADC_REFSEL_VDDREF_gc;
    ADC0_MUXPOS = ADC_MUXPOS_AIN0_gc;
    ADC0_EVCTRL = 0x01;
    ADC0_INTCTRL = ADC_RESRDY_bm;
    ADC0_COMMAND = ADC_STCONV_bm;

    sei();

    while (1) {
        PORTC_OUTTGL = PIN0_bm;
        for (volatile uint32_t i = 0; i < 60000; i++) {}
    }
}

ISR(ADC0_RESRDY_vect)
{
    uint16_t result = ADC0_RES;
    TCA0_SINGLE_CMP0 = result >> 2;
}
