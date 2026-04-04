/* smoke.c - Minimal test: verify CPU executes NOPs and halts
 * Compile with: avr-gcc -mmcu=atmega328p -nostartfiles -Os */

void __attribute__((naked,noreturn)) _start(void) {
    __asm__ volatile (
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "1: rjmp 1b\n\t"
    );
}
