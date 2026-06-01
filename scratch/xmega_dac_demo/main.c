#include <avr/io.h>

int main(void) {
    PORTC_DIR = 0x11;  // PC0 + PC4 output

    // DAC CH0: enable, LEFTADJ=0, write mid-scale
    DACA.CTRLA = DAC_CH0EN_bm;
    DACA.CH0DATA = 0x0800;  // 2.5V @ VREF=5V

    // TCC4: single-slope PWM, (199+1)*62.5ns = 12.5us = 80kHz, 50%
    TCC4_PER = 399;
    TCC4_CCA = 200;
    TCC4_CTRLB = 0x13;
    TCC4_CTRLA = 0x01;

    // Indicate initialization complete on PC0
    PORTC_OUT = 0x01;

    volatile unsigned int dac_val = 0x0000;
    volatile unsigned long i;

    while (1) {
        // Ramp DAC through 16 levels
        dac_val += 0x0100;
        if (dac_val > 0x0FFF) dac_val = 0x0000;

        DACA.CH0DATA = dac_val;

        // PC0: HIGH when DAC > half-scale
        PORTC_OUT = (dac_val >= 0x0800) ? 0x01 : 0x00;

        for (i = 0; i < 5000; i++);
    }
}
