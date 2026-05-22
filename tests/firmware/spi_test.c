#include <avr/io.h>
#include <stdint.h>

int main(void)
{
    DDRB |= (1 << PB0);
    PORTB = 0;

    DDRB |= (1 << PB2) | (1 << PB3) | (1 << PB5);
    DDRB &= ~(1 << PB4);

    SPCR = (1 << SPE) | (1 << MSTR);

    SPDR = 0xAA;
    while (!(SPSR & (1 << SPIF)));
    (void)SPDR;

    SPDR = 0x55;
    while (!(SPSR & (1 << SPIF)));
    (void)SPDR;

    PORTB |= (1 << PB0);

    for (;;) { }
}
