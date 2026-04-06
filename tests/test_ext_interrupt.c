#include <avr/io.h>
#include <avr/interrupt.h>

ISR(INT0_vect) {
    PORTB ^= (1 << PORTB5); // Toggle LED
}

int main(void) {
    DDRB |= (1 << DDB5);    // PB5 as Output
    
    // Configure INT0 (PD2)
    DDRD &= ~(1 << DDD2);   // Input
    PORTD |= (1 << PORTD2);  // Pull-up
    
    // Trigger on falling edge
    EICRA |= (1 << ISC01);
    EICRA &= ~(1 << ISC00);
    
    EIMSK |= (1 << INT0);   // Enable INT0
    sei();                  // Enable global interrupts

    while (1) {
        // Spin
    }
}
