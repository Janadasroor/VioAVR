#include <avr/io.h>

int main(void)
{
    DDRD |= (1 << PD0) | (1 << PD1);

    PSOC0 = (1 << 0) | (1 << 2);
    PCNF0 = 0;
    PFRC0A = (1 << 2);

    OCR0RBH = (500 >> 8) & 0x0F;
    OCR0RBL = 500 & 0xFF;

    OCR0SAH = (0 >> 8) & 0xFF;
    OCR0SAL = 0 & 0xFF;

    OCR0RAH = (250 >> 8) & 0xFF;
    OCR0RAL = 250 & 0xFF;

    OCR0SBH = (260 >> 8) & 0xFF;
    OCR0SBL = 260 & 0xFF;

    PCTL0 = (1 << 0);

    for (;;) { }
}
