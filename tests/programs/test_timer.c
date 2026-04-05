/* test_timer.c - Test timer operations for SimAVR vs VioAVR comparison */
#include <avr/io.h>
#include <stdint.h>

volatile uint8_t t0_overflow_count = 0;
volatile uint16_t t1_capture_value = 0;

int main(void) {
    /* Configure Timer0: Normal mode, no prescaler */
    TCCR0A = 0x00;  // Normal mode
    TCCR0B = 0x01;  // No prescaler (CS00=1)
    TCNT0 = 0x00;
    TIMSK0 |= (1 << TOIE0);  // Enable overflow interrupt

    /* Configure Timer1: Normal mode, no prescaler */
    TCCR1A = 0x00;
    TCCR1B = 0x01;  // No prescaler
    TCNT1 = 0x0000;

    /* Load some values into registers for verification */
    uint8_t a = 0xAA;
    uint8_t b = 0x55;
    uint16_t c = 0x1234;
    uint32_t d = 0xCAFEBABE;

    /* Simple arithmetic to test ALU */
    uint8_t sum = a + b;
    uint16_t product = c * 2;
    uint8_t xor_val = a ^ b;

    /* Wait a bit for timer to run */
    volatile uint16_t i;
    for (i = 0; i < 100; i++) {
        uint8_t temp = TCNT0;  // Read timer value
        uint16_t t1_val = TCNT1;
    }

    /* Check timer overflow flag */
    if (TIFR0 & (1 << TOV0)) {
        t0_overflow_count++;
    }

    /* Read timer values */
    uint8_t final_t0 = TCNT0;
    uint16_t final_t1 = TCNT1;

    while (1) {
        /* Infinite loop - execution stops here */
        asm volatile("nop");
    }
    
    return 0;
}
