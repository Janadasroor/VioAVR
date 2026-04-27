#include <avr/io.h>

/**
 * AT90PWM316 ADC to PWM Test
 * 
 * Target: AT90PWM316
 * Logic: Read ADC0 (PB0), output 8-bit PWM on OC0A (PB7)
 */

void adc_init(void) {
    // Reference: AVCC (REFS1:0 = 01)
    // ADMUX: [REFS1 REFS0 ADLAR - MUX3 MUX2 MUX1 MUX0]
    ADMUX = (1 << 6); 
    
    // ADCSRA: [ADEN ADSC ADATE ADIF ADIE ADPS2 ADPS1 ADPS0]
    // Enable ADC, Prescaler 128 (16MHz / 128 = 125kHz)
    ADCSRA = (1 << 7) | (1 << 2) | (1 << 1) | (1 << 0);
}

uint16_t adc_read(uint8_t channel) {
    // Select channel (MUX3:0)
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
    
    // Start conversion (ADSC)
    ADCSRA |= (1 << 6);
    
    // Wait for completion
    while (ADCSRA & (1 << 6));
    
    // Read low then high
    return ADC;
}

void pwm_init(void) {
    // PB7 as output (OC0A)
    DDRB |= (1 << 7);
    
    // Timer0 Control Register A
    // COM0A1:0 = 10 (Non-inverting PWM on OC0A)
    // WGM01:0 = 11 (Fast PWM)
    TCCR0A = (1 << 7) | (1 << 1) | (1 << 0);
    
    // Timer0 Control Register B
    // CS01:0 = 011 (Prescaler 64)
    TCCR0B = (1 << 1) | (1 << 0);
}

int main(void) {
    adc_init();
    pwm_init();
    
    while (1) {
        uint16_t val = adc_read(0);
        // Convert 10-bit ADC to 8-bit PWM duty cycle
        OCR0A = (uint8_t)(val >> 2);
    }
    
    return 0;
}
