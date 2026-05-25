#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

int main(void) {
    PORTB.DIR = PIN0_bm;
    PORTC.DIR = PIN0_bm;
    PORTD.DIR = PIN0_bm;

    while (1) {
        PORTB.OUTSET = PIN0_bm;
        PORTC.OUTSET = PIN0_bm;
        PORTD.OUTSET = PIN0_bm;
        _delay_us(50);
        PORTB.OUTCLR = PIN0_bm;
        PORTC.OUTCLR = PIN0_bm;
        PORTD.OUTCLR = PIN0_bm;
        _delay_us(50);
    }
}
