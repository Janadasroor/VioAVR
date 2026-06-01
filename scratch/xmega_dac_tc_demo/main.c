#include <avr/io.h>

int main(void) {
    // --- DAC0: enable channel 0 first ---
    DACA.CTRLA = 0x01;         // CH0EN

    // Write data: low byte first, then high byte triggers conversion
    // For ~2.0V at Vref=3.3V: data = 2.0 * 4096 / 3.3 = 2482 = 0x09B2
    DACA.CH0DATAL = 0xB2;
    DACA.CH0DATAH = 0x09;

    // --- TC4: single-slope PWM on PC4 (WO0) ---
    TCC4_CTRLA = 0x01;         // enable, clk/1
    TCC4_CTRLB = 0x10;         // CCAEN (WO0 on bit 4)
    TCC4_CTRLD = 0x03;         // WGMODE = 3 (SS PWM)
    TCC4_PER = 399;             // period = 400 cycles
    TCC4_CCA = 200;             // 50% duty cycle

    while (1);
}
