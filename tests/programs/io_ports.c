/* io_ports.c - Test I/O port operations */
#include <avr/io.h>

int main(void) {
    /* Set DDRB: PB0-PB3 as outputs */
    DDRB = 0x0F;
    
    /* Set PORTB: PB0, PB2 high */
    PORTB = 0x05;
    
    /* Read PINB (will read back PORTB for outputs) */
    volatile uint8_t pin_val = PINB;
    
    /* Toggle using PIN register */
    PINB = 0x05;
    
    /* Set DDRC: PC5 as output */
    DDRC = 0x20;
    
    /* Set PORTC: PC5 high */
    PORTC = 0x20;
    
    /* Set DDRD: all outputs */
    DDRD = 0xFF;
    
    /* Write to PORTD */
    PORTD = 0xA5;
    
    while (1) {}
    return 0;
}
