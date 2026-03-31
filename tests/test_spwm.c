#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

// 64-point Sine Lookup Table (scaled 0-255)
// Generated for a center of 128 and amplitude of 127
const uint8_t sine_table[64] = {
    128, 140, 153, 165, 177, 188, 199, 209,
    218, 226, 233, 240, 245, 249, 252, 254,
    255, 254, 252, 249, 245, 240, 233, 226,
    218, 209, 199, 188, 177, 165, 153, 140,
    128, 115, 102, 90, 78, 67, 56, 46,
    37, 29, 22, 15, 10, 6, 3, 1,
    0, 1, 3, 6, 10, 15, 22, 29,
    37, 46, 56, 67, 78, 90, 102, 115
};

volatile uint8_t sine_index = 0;

// Timer0 Compare Match A ISR - Updates the PWM duty cycle
ISR(TIMER0_COMPA_vect) {
    // Update Timer1 PWM Duty Cycle
    OCR1A = sine_table[sine_index];
    
    // Move to next point in sine wave
    sine_index = (sine_index + 1) & 63; 
}

int main(void) {
    // 1. Set PB1 (OC1A) as Output
    DDRB |= (1 << DDB1);

    // 2. Configure Timer1: Fast PWM, 8-bit (Mode 5)
    // Non-inverting mode on OC1A
    TCCR1A = (1 << COM1A1) | (1 << WGM10);
    // Prescaler = 1, Fast PWM (WGM12=1)
    TCCR1B = (1 << WGM12) | (1 << CS10);

    // 3. Configure Timer0: CTC Mode (Mode 2)
    // Update sine value every few cycles
    TCCR0A = (1 << WGM01);
    // Prescaler = 64
    TCCR0B = (1 << CS01) | (1 << CS00);
    // Set frequency of updates (OCR0A)
    OCR0A = 50; 
    
    // 4. Enable Timer0 Compare Match A Interrupt
    TIMSK0 |= (1 << OCIE0A);

    // 5. Enable Global Interrupts
    sei();

    while (1) {
        // Main loop does nothing; SPWM is handled entirely by hardware and ISR
    }

    return 0;
}
