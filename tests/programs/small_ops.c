/* small_ops.c - Test small/utility instructions (SWAP, ASR, LSR, ROR, NEG, COM) */
#include <avr/io.h>

volatile uint8_t result_swap;
volatile uint8_t result_asr;
volatile uint8_t result_lsr;
volatile uint8_t result_neg;
volatile uint8_t result_com;
volatile uint8_t result_tst;

int main(void) {
    uint8_t v;
    
    /* SWAP: 0x3C -> 0xC3 */
    v = 0x3C;
    v = (v >> 4) | (v << 4);
    result_swap = v;
    
    /* ASR: arithmetic shift right 0x80 -> 0xC0 */
    v = 0x80;
    result_asr = (uint8_t)((int8_t)v >> 1);
    
    /* LSR: logical shift right 0x80 -> 0x40 */
    v = 0x80;
    result_lsr = v >> 1;
    
    /* NEG: two's complement 0x01 -> 0xFF */
    v = 0x01;
    result_neg = (uint8_t)(-v);
    
    /* COM: one's complement 0x55 -> 0xAA */
    v = 0x55;
    result_com = ~v;
    
    /* TST: test for zero (AND reg,reg) */
    v = 0x00;
    result_tst = (v == 0) ? 0xFF : 0x00;
    
    while (1) {}
    return 0;
}
