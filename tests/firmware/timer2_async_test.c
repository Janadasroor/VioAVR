#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

volatile uint8_t overflow_count = 0;

ISR(TIMER2_OVF_vect)
{
    overflow_count++;
    if (overflow_count >= 244) {
        overflow_count = 0;
        PORTB ^= (1 << PB0);
    }
}

int main(void)
{
    DDRB |= (1 << PB0);
    PORTB &= ~(1 << PB0);

    ASSR = (1 << AS2);
    TCCR2A = 0;
    TCCR2B = (1 << CS22) | (1 << CS20);
    TIMSK2 = (1 << TOIE2);

    sei();

    for (;;) { }
}
