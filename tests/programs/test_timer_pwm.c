/* test_timer_pwm.c - Test Timer PWM modes for SimAVR vs VioAVR comparison */
#include <avr/io.h>
#include <stdint.h>

volatile uint8_t pwm_output = 0;
volatile uint8_t timer0_val = 0;
volatile uint16_t timer1_val = 0;

int main(void) {
    /* Configure Timer0: Fast PWM mode, non-inverting, no prescaler */
    TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);  // Non-inverting, Fast PWM
    TCCR0B = (1 << WGM02) | (1 << CS00);  // Fast PWM, no prescaler
    OCR0A = 128;  // 50% duty cycle
    TCNT0 = 0;

    /* Configure Timer1: CTC mode with OCR1A as TOP, no prescaler */
    TCCR1A = 0x00;
    TCCR1B = (1 << WGM12) | (1 << CS10);  // CTC mode, no prescaler
    OCR1A = 999;  // TOP value
    TCNT1 = 0;

    /* Load test values */
    uint8_t a = 0xAA;
    uint8_t b = 0x55;
    uint16_t c = 0x1234;

    /* Wait for timer to run */
    volatile uint16_t i;
    for (i = 0; i < 200; i++) {
        timer0_val = TCNT0;
        timer1_val = TCNT1;
    }

    /* Read final timer values */
    pwm_output = (TCCR0A >> 6) & 0x03;
    timer0_val = TCNT0;
    timer1_val = TCNT1;

    /* Read overflow/compare flags */
    uint8_t tifr0 = TIFR0;
    uint8_t tifr1 = TIFR1;

    while (1) {
        asm volatile("nop");
    }
    
    return 0;
}
