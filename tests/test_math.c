#include <avr/io.h>

int main(void) {
    volatile uint8_t a = 10;
    volatile uint8_t b = 20;
    volatile uint8_t c;

    while (1) {
        c = a + b;
        PORTB = c;
        a++;
        b--;
    }
}
