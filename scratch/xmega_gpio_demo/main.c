#include <avr/io.h>

int main(void) {
    PORTA_DIR = 0x01;
    PORTA_OUT = 0x01;
    while (1) {
        PORTA_OUTTGL = 0x01;
        for (volatile uint16_t i = 0; i < 30000; i++);
    }
}
