/* stack.c - Test push/pop and stack operations */
#include <avr/io.h>

volatile uint8_t out_popped;
volatile uint8_t out_popped2;

void callee(void) {
    uint8_t local = 0x77;
    (void)local;
}

int main(void) {
    /* Test push/pop via function call */
    uint8_t a = 0x12;
    uint8_t b = 0x34;
    
    /* Call function that uses stack */
    callee();
    
    /* Verify registers preserved */
    out_popped = a;
    out_popped2 = b;
    
    while (1) {}
    return 0;
}
