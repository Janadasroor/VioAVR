#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

volatile uint8_t int0_count = 0;

ISR(INT0_vect)
{
    int0_count++;
    PORTB ^= (1 << PB0);
}

int main(void)
{
    DDRB |= (1 << PB0);
    PORTB &= ~(1 << PB0);

    EICRA = (1 << ISC01);
    EIMSK = (1 << INT0);

    sei();

    for (;;) { }
}
