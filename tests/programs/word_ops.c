/* word_ops.c - Test 16-bit word operations (ADIW/SBIW/MOVW) */
#include <avr/io.h>

volatile uint16_t result_add_word;
volatile uint16_t result_sub_word;
volatile uint16_t result_copy;
volatile uint8_t result_mul_word;

int main(void) {
    /* ADIW: r25:r24 = 0x0100 + 1 = 0x0101 */
    uint16_t w1 = 0x0100;
    w1++;
    result_add_word = w1;
    
    /* SBIW: r25:r24 = 0x0101 - 1 = 0x0100 */
    w1--;
    result_sub_word = w1;
    
    /* MOVW: copy word */
    uint16_t w2 = w1;
    result_copy = w2;
    
    /* 16-bit multiply (uses MUL instructions) */
    uint8_t lo = 0x12;
    uint8_t hi = 0x34;
    result_mul_word = lo * hi;
    
    while (1) {}
    return 0;
}
