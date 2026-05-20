#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

int main(void) {
    // 1. Set PB1 (OC1A) as an output pin
    DDRB |= (1 << PB1);

    // 2. Configure Timer1 in Fast PWM Mode with ICR1 as TOP (Mode 14)
    // TCCR1A: Clear OC1A on Compare Match, set OC1A at BOTTOM (non-inverting mode)
    // COM1A1 = 1, COM1A0 = 0, WGM11 = 1, WGM10 = 0
    TCCR1A = (1 << COM1A1) | (1 << WGM11);

    // TCCR1B: WGM13 = 1, WGM12 = 1 (Fast PWM Mode 14)
    // Prescaler = 1 (CS10 = 1) -> Clock frequency = 16 MHz
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);

    // 3. Set PWM Period (TOP value in ICR1)
    // F_PWM = F_CPU / (Prescaler * (1 + TOP))
    // For 80 kHz PWM: TOP = (16,000,000 / 80,000) - 1 = 199
    ICR1 = 199;

    // 4. Set Initial Duty Cycle (Compare value in OCR1A)
    // 50% duty cycle: 199 * 0.5 = 100
    OCR1A = 100;

    // 5. Initialize ADC
    // ADMUX: REFS0 = 1 (AVcc with external cap at AREF pin), MUX3:0 = 0000 (ADC0)
    ADMUX = (1 << REFS0);
    // ADCSRA: ADEN = 1 (Enable ADC), ADPS2 = 1 (Prescaler = 16 for high conversion speed)
    ADCSRA = (1 << ADEN) | (1 << ADPS2);

    float integral = 0.0f;
    float Kp = 0.6f;
    float Ki = 0.15f;
    float target_adc = 410.0f; // 2.0V sensing (representing 12.0V drain voltage / 6)

    // PI Closed-Loop Regulation Loop
    while (1) {
        // Start Conversion
        ADCSRA |= (1 << ADSC);
        // Wait for completion
        while (ADCSRA & (1 << ADSC));
        
        uint16_t adc_val = ADC;
        float error = (float)adc_val - target_adc;
        integral += error;
        
        // Anti-windup clamping
        if (integral > 300.0f) integral = 300.0f;
        else if (integral < -300.0f) integral = -300.0f;
        
        float control = (Kp * error) + (Ki * integral);
        float new_ocr = 100.0f - control;
        
        // Clamp to valid OCR range [10, 190] (5% to 95% duty cycle)
        if (new_ocr > 190.0f) new_ocr = 190.0f;
        else if (new_ocr < 10.0f) new_ocr = 10.0f;
        
        OCR1A = (uint16_t)new_ocr;
        
        // Small delay to allow analog node voltages to settle between steps
        _delay_us(2);
    }

    return 0;
}
