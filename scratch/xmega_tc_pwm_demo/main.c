#include <avr/io.h>

int main(void) {
    PORTC_DIR = 0x01;
    TCC4_CTRLA = 0x01;
    TCC4_CTRLB = 0x10;
    TCC4_PER = 399;
    TCC4_CCA = 200;
    while (1);
}
