/* alu.c - Test arithmetic and logic instructions */
#include <avr/io.h>

volatile uint8_t result_add;
volatile uint8_t result_sub;
volatile uint8_t result_and;
volatile uint8_t result_or;
volatile uint8_t result_xor;
volatile uint8_t result_inc;
volatile uint8_t result_dec;
volatile uint16_t result_mul;

int main(void) {
    uint8_t a = 0x0F;
    uint8_t b = 0x01;
    
    /* ADD: 0x0F + 0x01 = 0x10 */
    result_add = a + b;
    
    /* SUB: 0x10 - 0x01 = 0x0F */
    result_sub = result_add - b;
    
    /* AND: 0xFF & 0x0F = 0x0F */
    result_and = 0xFF & 0x0F;
    
    /* OR: 0xF0 | 0x0F = 0xFF */
    result_or = 0xF0 | 0x0F;
    
    /* XOR: 0xAA ^ 0x55 = 0xFF */
    result_xor = 0xAA ^ 0x55;
    
    /* INC: 0x0F + 1 = 0x10 */
    a++;
    result_inc = a;
    
    /* DEC: 0x10 - 1 = 0x0F */
    a--;
    result_dec = a;
    
    /* MUL: 0x10 * 0x02 = 0x0020 */
    result_mul = (uint16_t)result_add * 2;
    
    while (1) {}
    return 0;
}
