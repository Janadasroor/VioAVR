#include <avr/io.h>
int main(void) {
    PORTA_DIR = PIN1_bm;  // PA1 as output
    PORTA_OUT = PIN1_bm;  // PA1 = HIGH
    for (;;) {}
}
