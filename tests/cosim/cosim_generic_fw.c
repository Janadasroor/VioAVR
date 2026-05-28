/*
 * Generic co-simulation test firmware for any AVR device.
 * Sets PORTB bit 0 (PB0) to output and drives it HIGH, then loops forever.
 */
#define F_CPU 16000000UL
#include <avr/io.h>

int main(void) {
    DDRB |= (1 << PB0);
    PORTB |= (1 << PB0);
    while (1) { }
}
