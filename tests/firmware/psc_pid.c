#include <avr/io.h>
#include <avr/interrupt.h>

/**
 * AT90PWM316 PSC PID Controller
 * 
 * Target: AT90PWM316
 * Logic: 
 *   - PSC0 runs at 100kHz (160 cycles @ 16MHz)
 *   - ADC0 (Feedback) is sampled
 *   - PID (Kp=1.5) calculates duty cycle
 *   - Output on PSCOUT0A (Pulse width controlled by OCR0RA)
 */

#define PERIOD 160
#define SETPOINT 512

volatile int16_t last_error = 0;
volatile int16_t integral = 0;

void psc_init(void) {
    // PSC0 Output A on PB7
    // OCR0SA (Start) = 0, OCR0RA (End) = 80 (50% initially)
    // Register mapping is Big-Endian (High then Low)
    
    OCR0SAH = 0;
    OCR0SAL = 0;
    
    OCR0RAH = 0;
    OCR0RAL = 80;
    
    // OCR0RB defines the period (160)
    OCR0RBH = 0;
    OCR0RBL = 160;
    
    // PSOC0: Enable Output A
    PSOC0 = (1 << 0);
    
    // PCTL0: PRUN=1
    PCTL0 = (1 << 0);
}

void adc_init(void) {
    ADMUX = (1 << 6); // AVCC ref, ADC0
    ADCSRA = (1 << 7) | (1 << 2) | (1 << 1); // Enable, Prescaler 64
}

uint16_t adc_read(void) {
    ADCSRA |= (1 << 6);
    while (ADCSRA & (1 << 6));
    return ADC;
}

int main(void) {
    psc_init();
    adc_init();
    
    // Kp = 1.5 -> represented as 3/2 in fixed point
    const int16_t kp_num = 3;
    const int16_t kp_den = 2;
    
    while (1) {
        uint16_t feedback = adc_read();
        int16_t error = SETPOINT - (int16_t)feedback;
        
        // P-Term: (error * 3) / 2
        int16_t output = (error * kp_num) / kp_den;
        
        // Clamp output to valid PSC range (10 to 150)
        // Offset output by a nominal duty cycle (e.g. 80)
        int16_t duty = 80 + output;
        
        if (duty < 10) duty = 10;
        if (duty > 150) duty = 150;
        
        // Update PSC0RA (Big-Endian)
        OCR0RAH = (uint8_t)(duty >> 8);
        OCR0RAL = (uint8_t)(duty & 0xFF);
        
        // Wait a bit to simulate control frequency
        for (volatile uint16_t i = 0; i < 100; i++);
    }
    
    return 0;
}
