#include <avr/io.h>
#include <util/delay.h>

int main(void) {
    // Configure PB0 as output (PORTB bit 0)
    DDRB |= (1 << PB0);
    
    // Configure PB1 as input (PORTB bit 1)
    DDRB &= ~(1 << PB1);
    
    // Enable pull-up on PB1
    PORTB |= (1 << PB1);

    while (1) {
        // Read input on PB1 (PINB bit 1)
        if (PINB & (1 << PB1)) {
            // PB1 is HIGH -> toggle PB0 fast (~10 kHz, 50us delay per phase)
            PORTB ^= (1 << PB0);
            _delay_us(50);
        } else {
            // PB1 is LOW -> toggle PB0 slow (~2 kHz, 250us delay per phase)
            PORTB ^= (1 << PB0);
            _delay_us(250);
        }
    }
    return 0;
}
