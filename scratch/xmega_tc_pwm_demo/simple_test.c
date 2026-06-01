#include <avr/io.h>
int main(void) {
    PORTA_DIR = 0x01;
    PORTA_OUT = 0x01;
    while (1);
}
