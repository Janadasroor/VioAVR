/* branches.c - Test conditional and unconditional branches */
#include <avr/io.h>

volatile uint8_t taken_breq;
volatile uint8_t taken_brne;
volatile uint8_t taken_brcs;
volatile uint8_t taken_brcc;
volatile uint8_t taken_brmi;
volatile uint8_t taken_brpl;
volatile uint8_t result_rjmp;
volatile uint8_t result_loop;

int main(void) {
    uint8_t z = 0;
    uint8_t nz = 1;
    uint8_t val;
    
    /* BREQ - should be taken when Z=1 */
    z = z - 0;  /* sets Z flag */
    if (z == 0) {
        taken_breq = 1;
    } else {
        taken_breq = 0;
    }
    
    /* BRNE - should be taken when Z=0 */
    nz = nz + 1;  /* clears Z flag */
    if (nz != 0) {
        taken_brne = 1;
    } else {
        taken_brne = 0;
    }
    
    /* RJMP - relative jump */
    val = 0xAA;
    result_rjmp = val;
    
    /* Loop counter */
    result_loop = 0;
    for (uint8_t i = 0; i < 5; i++) {
        result_loop++;
    }
    
    while (1) {}
    return 0;
}
