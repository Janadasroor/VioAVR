#include <avr/io.h>

int main(void) {
    DDRB = 0xFF;
    PORTB = 0x00;

    volatile uint16_t a = 1, b = 1, c = 1;
    volatile uint32_t accum = 0;

    while (1) {
        c = a + b;
        a = b;
        b = c;
        accum += (uint32_t)c;

        if (c > 10000) {
            PORTB ^= 0x01;
            a = 1;
            b = 1;
        }

        for (volatile uint8_t i = 0; i < 5; i++) {
            accum ^= (uint32_t)(i << 1);
        }
    }
}
