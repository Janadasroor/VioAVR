#include <avr/io.h>

int main(void) {
    volatile uint8_t i;
    for (i = 0; i < 100; i++);
    while (1) {
        __asm__ __volatile__(
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop"
        );
    }
    return 0;
}
