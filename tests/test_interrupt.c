#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint8_t count = 0;

ISR(TIMER0_OVF_vect) {
    count++;
    PORTB = count;
}

int main(void) {
    // Set up Timer0: Normal mode, prescaler 1
    TCCR0B |= (1 << CS00);
    // Enable Timer0 overflow interrupt
    TIMSK0 |= (1 << TOIE0);
    
    // Enable global interrupts
    sei();

    while (1) {
        // Just spin
    }
}
