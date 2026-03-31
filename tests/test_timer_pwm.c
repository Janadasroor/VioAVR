#include <avr/io.h>
#include <util/delay.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

int main(void) {
    // 1. Set PB1 (OC1A) as an output
    DDRB |= (1 << DDB1);

    // 2. Configure Timer1 for Fast PWM, 8-bit mode
    // WGM10=1, WGM11=0, WGM12=1, WGM13=0 -> Mode 5: Fast PWM, 8-bit, TOP=0x00FF
    // COM1A1=1, COM1A0=0 -> Clear OC1A on Compare Match, set OC1A at BOTTOM (non-inverting)
    TCCR1A = (1 << COM1A1) | (1 << WGM10);
    TCCR1B = (1 << WGM12) | (1 << CS10); // Prescaler = 1

    uint8_t duty = 0;

    while (1) {
        // 3. Update Compare Register
        OCR1AL = duty;
        
        duty++;

        // In a real chip, we'd wait. In simulation, we can loop quickly.
        // But let's add a small delay for "realism" and to see the trace.
        for(volatile uint16_t i=0; i<100; i++);
    }

    return 0;
}
