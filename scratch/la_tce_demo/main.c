#include <stdint.h>

#define PORTA_DIR   (*(volatile uint8_t*)0x401)
#define PORTA_OUT   (*(volatile uint8_t*)0x400)

#define TCE_CTRLA   (*(volatile uint8_t*)0xA00)
#define TCE_CTRLB   (*(volatile uint8_t*)0xA01)

#define PER_LOW     (*(volatile uint8_t*)0xA26)
#define PER_HIGH    (*(volatile uint8_t*)0xA27)
#define CMP0_LOW    (*(volatile uint8_t*)0xA28)
#define CMP0_HIGH   (*(volatile uint8_t*)0xA29)

int main(void) {
    PORTA_DIR |= 0x01;

    CMP0_LOW = 80;
    CMP0_HIGH = 0;

    PER_LOW = 160;
    PER_HIGH = 0;

    TCE_CTRLB = 0x10;

    TCE_CTRLA = 0x01;

    while (1) { }
}
