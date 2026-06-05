#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

// Segment mapping: PORTB bits 0-6 = a-g
//   a=PB0, b=PB1, c=PB2, d=PB3, e=PB4, f=PB5, g=PB6
const uint8_t digits[10] = {
    0b0111111, // 0: a b c d e f
    0b0000110, // 1:   b c
    0b1011011, // 2: a b   d e g
    0b1001111, // 3: a b c d   g
    0b1100110, // 4:   b c   f g
    0b1101101, // 5: a   c d f g
    0b1111101, // 6: a   c d e f g
    0b0000111, // 7: a b c
    0b1111111, // 8: a b c d e f g
    0b1101111, // 9: a b c d f g
};

int main(void) {
    DDRB = 0x7F; // PB0-PB6 as outputs (segments a-g)
    DDRD = 0x00; // no columns
    
    uint8_t digit = 0;
    while (1) {
        PORTB = digits[digit];
        _delay_us(500);
        digit = (digit + 1) % 10;
    }
}
