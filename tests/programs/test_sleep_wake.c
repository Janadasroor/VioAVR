/* test_sleep_wake.c - Test sleep modes and wake-up for SimAVR vs VioAVR comparison */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

volatile uint8_t sleep_count = 0;
volatile uint8_t wake_reason = 0;

int main(void) {
    /* Configure Timer0: CTC mode, OCR0A=50, no prescaler */
    TCCR0A = (1 << WGM01);  // CTC mode
    TCCR0B = (1 << CS00);   // No prescaler
    OCR0A = 50;             // Compare value
    TIMSK0 |= (1 << OCIE0A); // Enable compare match A interrupt

    /* Configure sleep mode: Idle mode (SM=000) */
    SMCR = (1 << SE);  // Sleep Enable, Idle mode

    /* Enable global interrupts */
    sei();

    /* Main loop: execute SLEEP instruction */
    uint8_t counter = 0;
    for (;;) {
        counter++;
        
        /* Execute SLEEP instruction */
        asm volatile("sleep" ::);
        
        sleep_count++;
        wake_reason = counter;
        
        /* Clear SMCR.SE after wake to prevent accidental re-sleep */
        SMCR = 0;
    }
    
    return 0;
}

/* Timer0 Compare Match A ISR */
ISR(TIMER0_COMPA_vect) {
    /* Just wake up - return from ISR will continue execution */
}
