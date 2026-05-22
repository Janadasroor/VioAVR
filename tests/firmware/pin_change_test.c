#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

volatile uint8_t pc_count = 0;

ISR(PCINT1_vect)
{
    pc_count++;
    PORTB ^= (1 << PB0);
}

int main(void)
{
    DDRB |= (1 << PB0);
    PORTB &= ~(1 << PB0);

    PCICR = (1 << PCIE1);
    PCMSK1 = (1 << PCINT8);

    sei();

    for (;;) { }
}
