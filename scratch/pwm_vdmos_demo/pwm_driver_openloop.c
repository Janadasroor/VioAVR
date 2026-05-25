#define F_CPU 16000000UL
#include <avr/io.h>

int main(void) {
    DDRB |= (1 << PB1);
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
    ICR1 = 199;
    OCR1A = 100;
    while (1) { }
    return 0;
}
