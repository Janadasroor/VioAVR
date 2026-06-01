#include <avr/io.h>

int main(void) {
    PORTA.DIRSET = 0x01;
    PORTD.DIRSET = 0x40;

    IRCOM.CTRL = 0x01;
    IRCOM.TXPLCTRL = 100;

    while (1);
}
