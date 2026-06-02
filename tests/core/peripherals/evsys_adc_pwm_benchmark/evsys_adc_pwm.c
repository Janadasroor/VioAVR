#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

ISR(ADC0_RESRDY_vect) {
    uint16_t result = ADC0.RES;
    TCA0.SINGLE.CMP0 = result;
    ADC0.INTFLAGS = ADC_RESRDY_bm;
}

int main(void) {
    PORTA.DIR = PIN0_bm;

    TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm;
    TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP0EN_bm;
    TCA0.SINGLE.PER = 0xFFFF;
    TCA0.SINGLE.CMP0 = 0x8000;

    ADC0.CTRLB = ADC_SAMPNUM_ACC1_gc;
    ADC0.CTRLC = ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
    ADC0.CTRLA = ADC_ENABLE_bm | ADC_FREERUN_bm | ADC_RESSEL_10BIT_gc;
    ADC0.MUXPOS = ADC_MUXPOS_AIN0_gc;
    *(volatile uint8_t *)0x0608 = 0x01;

    ADC0.INTCTRL = ADC_RESRDY_bm;
    sei();

    while (1) {
        __asm__ __volatile__("" ::: "memory");
    }
}
