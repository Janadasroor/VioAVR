#include <avr/io.h>

/**
 * ADC Basic Test
 * Tests ADC enable, start conversion, and wait.
 */
int main() {
    // DDRB for output
    DDRB = 0xFF;

    // Enable ADC, prescaler /2
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    
    // Choose ADC0
    ADMUX = 0;
    
    uint16_t i = 0;
    while(i < 5) {
        // Start conversion
        ADCSRA |= (1 << ADSC);
        
        // Busy wait
        while(ADCSRA & (1 << ADSC));
        
        // Result
        uint8_t low = ADCL;
        uint8_t high = ADCH;
        
        PORTB = high;
        i++;
    }
    
    // Stop
    while(1);
}
