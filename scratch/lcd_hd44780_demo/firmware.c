#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define E_BIT  2   // PB1 = bit 1
#define RS_BIT 1   // PB0 = bit 0

static void short_delay(void) {
    volatile uint8_t i = 10;
    while (--i) { __asm__ __volatile__("nop"); }
}

static void e_pulse(uint8_t port_val) {
    // E HIGH
    PORTB = port_val | E_BIT;
    short_delay();
    // E LOW
    PORTB = port_val;
    short_delay();
}

static void lcd_nibble(uint8_t nibble) {
    e_pulse(nibble);
}

static void lcd_byte(uint8_t rs, uint8_t byte) {
    uint8_t port_val = byte & 0xF0;
    if (rs) port_val |= RS_BIT;
    e_pulse(port_val);

    port_val = (byte << 4) & 0xF0;
    if (rs) port_val |= RS_BIT;
    e_pulse(port_val);
}

int main(void) {
    DDRB = 0xF3;
    PORTB = 0;

    _delay_us(10);

    lcd_nibble(0x30);
    lcd_nibble(0x30);
    lcd_nibble(0x30);
    lcd_nibble(0x20);

    lcd_byte(0, 0x28);
    lcd_byte(0, 0x0C);
    lcd_byte(0, 0x06);
    lcd_byte(0, 0x01);

    lcd_byte(1, 'H');
    lcd_byte(1, 'e');
    lcd_byte(1, 'l');
    lcd_byte(1, 'l');
    lcd_byte(1, 'o');

    PORTB = 0;
    while (1);
}
