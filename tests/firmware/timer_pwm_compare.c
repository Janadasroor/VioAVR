#include <avr/io.h>

/**
 * Timer0 Fast PWM Mode 3
 * Tests OCR0A compare match and overflow
 */
int main() {
    // OC0A (PB3) as output
    DDRB |= (1 << PB3);
    
    // Set Fast PWM, non-inverting mode
    TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);
    
    // Duty cycle 25% (63 / 255)
    OCR0A = 63;
    
    // Prescaler /1 (no scaling) to get high frequency for trace
    TCCR0B = (1 << CS00);
    
    uint16_t i = 0;
    while(1) {
        i++;
        if (i > 1000) {
            // Change duty cycle to 75%
            OCR0A = 191;
            i = 0;
        }
    }
}
