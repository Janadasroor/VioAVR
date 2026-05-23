#include <avr/io.h>

volatile uint8_t v_a;
volatile uint8_t v_b;
volatile uint8_t v_c;

int main(void) {
    DDRB = 0xFF;
    PORTB = 0x00;

    v_a = 0x10;
    v_b = 0x20;
    v_c = 0x00;

    // ADD
    v_c = v_a + v_b;
    PORTB = v_c;

    // SUB
    v_c = v_c - 0x10;
    PORTB = v_c;

    // AND
    v_c = v_c & 0xAA;
    PORTB = v_c;

    // OR
    v_c = v_c | 0x05;
    PORTB = v_c;

    // XOR
    v_c = v_c ^ 0x25;
    PORTB = v_c;

    // INC
    v_c = 0x01;
    v_c = v_c + 1;
    PORTB = v_c;

    // DEC
    v_c = v_c - 1;
    PORTB = v_c;

    // COM (one's complement)
    v_c = 0xFF;
    v_c = ~v_c;
    PORTB = v_c;

    // LSR (shift right)
    v_c = 0x10;
    v_c = v_c >> 1;
    PORTB = v_c;

    // CPI / BREQ
    if (v_c == 0x08) {
        PORTB = 0xAA;
    }

    // CP / BRNE
    if (v_c != 99) {
        PORTB = 0xBB;
    }

    // NEG (two's complement)
    v_c = 0x01;
    v_c = -v_c;
    PORTB = v_c;

    // Write final pattern
    PORTB = 0x55;

    while (1) {}
    return 0;
}
