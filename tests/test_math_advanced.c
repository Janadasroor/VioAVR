#include <avr/io.h>

int main(void) {
    DDRB = 0xFF;
    DDRC = 0xFF;

    volatile uint16_t x = 1000;
    volatile uint16_t y = 500;
    volatile uint16_t res16;

    volatile uint32_t long_a = 100000;
    volatile uint32_t long_b = 50000;
    volatile uint32_t res32;

    // 1. Addition (16-bit) -> 1500 (0x05DC)
    res16 = x + y;
    PORTB = (uint8_t)(res16 & 0xFF); // 0xDC
    PORTC = (uint8_t)(res16 >> 8);   // 0x05

    // 2. Subtraction (16-bit) -> 500 (0x01F4)
    res16 = x - y;
    PORTB = (uint8_t)(res16 & 0xFF); // 0xF4
    PORTC = (uint8_t)(res16 >> 8);   // 0x01

    // 3. Multiplication (8-bit) -> 200 (0xC8)
    volatile uint8_t m1 = 10;
    volatile uint8_t m2 = 20;
    PORTB = m1 * m2; // 0xC8

    // 4. Division (16-bit) -> 2
    res16 = x / y;
    PORTB = (uint8_t)res16; // 0x02

    // 5. 32-bit Addition -> 150000 (0x000249F0)
    res32 = long_a + long_b;
    PORTB = (uint8_t)(res32 & 0xFF);        // 0xF0
    PORTC = (uint8_t)((res32 >> 8) & 0xFF); // 0x49

    while (1) {
        // Halt
    }
}
