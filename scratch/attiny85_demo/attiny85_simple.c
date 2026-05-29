#define F_CPU 16000000UL
#include <avr/io.h>

int main(void) {
    DDRB |= (1 << PB0);
    PORTB |= (1 << PB0);
    while (1) { }
}
