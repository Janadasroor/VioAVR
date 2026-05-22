#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

ISR(ANALOG_COMP_vect)
{
    PORTB ^= (1 << PB0);
}

int main(void)
{
    DDRB |= (1 << PB0);
    PORTB &= ~(1 << PB0);

    ACSR = (1 << ACBG) | (1 << ACIE) | (1 << ACIS1) | (1 << ACIS0);

    sei();

    for (;;) { }
}
