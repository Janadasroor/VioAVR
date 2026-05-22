#include <avr/io.h>
#include <stdint.h>

int main(void)
{
    DDRB |= (1 << PB0);
    PORTB = 0;

    while (EECR & (1 << EEPE));

    EEARL = 0x10;
    EEARH = 0x00;
    EEDR = 0xA5;
    EECR |= (1 << EEMPE);
    EECR |= (1 << EEPE);

    while (EECR & (1 << EEPE));

    EEARL = 0x10;
    EEARH = 0x00;
    EECR |= (1 << EERE);

    if (EEDR == 0xA5) {
        PORTB |= (1 << PB0);
    }

    for (;;) { }
}
