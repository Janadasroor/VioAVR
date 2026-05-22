#include <avr/io.h>

int main(void)
{
    DDRB |= (1 << PB1) | (1 << PB2);

    ICR1H = (799 >> 8) & 0xFF;
    ICR1L = 799 & 0xFF;

    OCR1AH = (392 >> 8) & 0xFF;
    OCR1AL = 392 & 0xFF;
    OCR1BH = (408 >> 8) & 0xFF;
    OCR1BL = 408 & 0xFF;

    TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << COM1B0) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);

    for (;;) { }
}
