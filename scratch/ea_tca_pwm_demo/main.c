#include <avr/io.h>

int main(void) {
    PORTA_DIR = PIN0_bm;
    TCA0_SINGLE_CTRLB = 0x13;
    TCA0_SINGLE_PER = 255;
    TCA0_SINGLE_CMP0 = 64;
    TCA0_SINGLE_CTRLA = 0x01;
    for (;;) {}
}
