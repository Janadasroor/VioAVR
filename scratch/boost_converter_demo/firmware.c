#include <avr/io.h>
#include <util/delay.h>

#define TARGET_ADC 512    // 2.5V target (with 5V Vref)
#define MAX_DUTY 220      // Safe limit (86% max duty cycle)
#define MIN_DUTY 10       // Safe limit (4% min duty cycle)

void pwm_init(void) {
    // Configure PB1 (OC1A) as output
    DDRB |= (1 << PB1);
    
    // Fast PWM 8-bit mode (Mode 5: WGM12=1, WGM10=1)
    // Clear OC1A on Compare Match, set OC1A at BOTTOM (non-inverting mode)
    TCCR1A = (1 << COM1A1) | (1 << WGM10);
    TCCR1B = (1 << WGM12) | (1 << CS10); // No prescaling (62.5 kHz @ 16 MHz)
    
    OCR1A = 50; // Initial duty cycle (~20%)
}

void adc_init(void) {
    // Select Vref = AVcc, Input Channel = ADC0 (PC0)
    ADMUX = (1 << REFS0);
    
    // Enable ADC, set prescaler to 128 (125 kHz ADC clock @ 16 MHz)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read(void) {
    // Start conversion
    ADCSRA |= (1 << ADSC);
    
    // Wait for completion
    while (ADCSRA & (1 << ADSC));
    
    return ADC;
}

int main(void) {
    pwm_init();
    adc_init();
    
    int16_t current_duty = 50;
    
    while (1) {
        uint16_t val = adc_read();
        
        // Simple integral control / feedback loop
        if (val < TARGET_ADC) {
            if (current_duty < MAX_DUTY) {
                current_duty++;
            }
        } else if (val > TARGET_ADC) {
            if (current_duty > MIN_DUTY) {
                current_duty--;
            }
        }
        
        OCR1A = (uint8_t)current_duty;
        
        // Loop delay to let the analog stage settle
        _delay_us(10);
    }
    
    return 0;
}
