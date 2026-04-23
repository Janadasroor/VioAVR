#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>

int main(void) {
    // Simple test: set PORTB as output
    DDRB = 0xFF;
    PORTB = 0x55;
    
    // Infinite loop
    while(1) {
        // Toggle PORTB
        PORTB ^= 0xFF;
    }
    
    return 0;
}