#include <avr/io.h>

int main(void) {
    PORTC_DIR = 0x01;
    TCC4_CTRLA = 0x01;
    TCC4_CTRLB = 0x10;
    TCC4_PER = 1023;
    TCC4_CCA = 512;

    ADCA_CTRLA = 0x01;
    ADCA_CTRLB = 0x00;
    ADCA_REFCTRL = 0x03;
    ADCA_PRESCALER = 0x00;

    uint16_t adc_val = 512;

    while (1) {
        ADCA_CTRLA |= 0x40;
        while (!(ADCA_INTFLAGS & 0x01));
        adc_val = ADCA_CH0RES;
        ADCA_INTFLAGS = 0x01;

        uint16_t duty = adc_val >> 2;
        if (duty > 1023) duty = 1023;
        TCC4_CCA = 1023 - duty;

        for (volatile uint32_t i = 0; i < 10000; i++);
    }
}
