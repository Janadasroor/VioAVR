#include <avr/io.h>

int main(void) {
    DDRB |= (1 << DDB5);   // PB5 as Output (LED)
    DDRD &= ~(1 << DDD2);  // PD2 as Input
    PORTD |= (1 << PORTD2); // Enable pull-up on PD2

    while (1) {
        if (PIND & (1 << PIND2)) {
            PORTB |= (1 << PORTB5);
        } else {
            PORTB &= ~(1 << PORTB5);
        }
    }
}
