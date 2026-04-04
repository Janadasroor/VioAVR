/* compare.c - Test comparison instructions (CP/CPC/CPI/CPSE) */
#include <avr/io.h>

volatile uint8_t cmp_equal;
volatile uint8_t cmp_less;
volatile uint8_t cmp_greater;
volatile uint8_t cpse_result;

int main(void) {
    uint8_t a = 0x10;
    uint8_t b = 0x10;
    uint8_t c = 0x20;
    
    /* Compare equal: a == b */
    if (a == b) {
        cmp_equal = 1;
    }
    
    /* Compare less: a < c */
    if (a < c) {
        cmp_less = 1;
    }
    
    /* Compare greater: c > b */
    if (c > b) {
        cmp_greater = 1;
    }
    
    /* Compare and skip if equal */
    uint8_t skip_test = 0;
    if (a == b) {
        skip_test = 0xAA;
    }
    cpse_result = skip_test;
    
    while (1) {}
    return 0;
}
