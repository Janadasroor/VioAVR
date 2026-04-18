#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint8_t count = 0;

ISR(TIMER1_COMPA_vect) {
    // Toggle PB0 to trace the interrupt entry timing
    PORTB ^= (1 << PB0);
    count++;
}

int main() {
    // PB0 as output
    DDRB |= (1 << PB0);
    
    // Timer1 CTC mode 4, OCR1A = TOP
    TCCR1B = (1 << WGM12);
    
    // Set TOP to 100 cycles bit-for-bit
    // At 16MHz, /1 prescaler, this should fire every 101 cycles (TCNT 0..100)
    // Actually, CTC fires when TCNT == OCR. If OCR=100, it stays at 100 for 1 cycle then resets to 0.
    OCR1A = 100;
    
    // Enable OCR1A interrupt
    TIMSK1 = (1 << OCIE1A);
    
    // Global Interrupt Enable
    sei();
    
    // Start Timer (Prescaler /8 to keep the trace manageable, or /1 for extreme precision)
    // Let's use /1 to see EVERY cycle clearly.
    TCCR1B |= (1 << CS10);
    
    while(1) {
        if (count >= 10) {
            // Stop after 10 interrupts
            TCCR1B = 0;
            break;
        }
    }
    
    // Infinite loop or crash to stop tracer
    while(1) {
        asm volatile("nop");
    }
}
