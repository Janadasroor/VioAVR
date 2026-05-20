#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

int main(void) {
    // 1. Set PB0 and PB1 as output pins
    DDRB |= (1 << PB0) | (1 << PB1);
    
    // 2. Set all PORTD pins (PD0 - PD7) as output pins
    DDRD = 0xFF;

    while (1) {
        // Cycle PB0
        PORTB = (1 << PB0);
        PORTD = 0x00;
        _delay_us(100);

        // Cycle PB1
        PORTB = (1 << PB1);
        PORTD = 0x00;
        _delay_us(100);

        // Turn off PORTB
        PORTB = 0x00;

        // Cycle through PORTD0 to PORTD7
        for (int i = 0; i < 8; i++) {
            PORTD = (1 << i);
            _delay_us(100);
        }
    }

    return 0;
}
