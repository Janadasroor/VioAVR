// Blinky LED firmware for VioAVR simulator test.
// Public Domain / Unlicense - written from scratch, no external dependencies.
#include <avr/io.h>
#include <util/delay.h>

int main(void) {
    DDRB |= (1 << DDB5);
    while (1) {
        PORTB |= (1 << PORTB5);
        _delay_ms(100);
        PORTB &= ~(1 << PORTB5);
        _delay_ms(100);
    }
}
