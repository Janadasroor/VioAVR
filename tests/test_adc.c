#include <avr/io.h>

int main(void) {
    // Set up ADC:
    // ADMUX: REFS0=1 (AVCC with external capacitor at AREF pin), MUX=0 (Channel 0)
    ADMUX = (1 << REFS0);
    
    // ADCSRA: ADEN=1 (Enable), ADPS2:0=111 (Prescaler 128)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    // DDRB = 0xFF (Output for visualization)
    DDRB = 0xFF;

    while (1) {
        // Start conversion
        ADCSRA |= (1 << ADSC);

        // Wait for conversion to finish
        while (ADCSRA & (1 << ADSC));

        // Output ADCH to PORTB (high 8 bits of result if ADLAR=0, wait, ADLAR=0 by default)
        // Actually, with REFS0=1, we get 10-bit result.
        // Let's just output ADCL to PORTB.
        PORTB = ADCL;
        
        // Short delay (not strictly needed in sim but good practice)
        for(volatile int i=0; i<100; i++);
    }
}
