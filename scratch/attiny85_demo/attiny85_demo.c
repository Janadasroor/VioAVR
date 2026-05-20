#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

/**
 * ATtiny85 Mixed-Signal Co-Simulation Demo
 * 
 * This firmware runs cycle-accurately inside the VioAVR CPU model.
 * It reads PORTB Pin 1 (which receives the analog signal after being 
 * mapped to a digital value via VioAVR's Schmitt trigger threshold solver).
 * 
 * Based on the input state:
 * - If PB1 is High: fast oscillation (50us delay -> ~10 kHz square wave output on PB0)
 * - If PB1 is Low: slow oscillation (250us delay -> ~2 kHz square wave output on PB0)
 * 
 * This demonstrates direct cycle-accurate feedback in mixed-signal co-simulation.
 */
int main(void) {
    // 1. Configure PB0 as a digital output
    DDRB |= (1 << PB0);
    
    // 2. Configure PB1 as a digital input
    DDRB &= ~(1 << PB1);
    
    // Initialize outputs
    PORTB = 0x00;

    while (1) {
        // Toggle the PB0 output pin
        PORTB ^= (1 << PB0);

        // Read the input pin PB1 and delay dynamically to control output frequency
        if (PINB & (1 << PINB1)) {
            _delay_us(50);  // High-frequency mode
        } else {
            _delay_us(250); // Low-frequency mode
        }
    }

    return 0;
}
