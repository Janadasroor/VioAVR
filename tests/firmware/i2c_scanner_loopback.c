#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

static void usart_tx(char c)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

static void usart_print(const char* s)
{
    while (*s) usart_tx(*s++);
}

int main(void)
{
    UBRR0L = 103;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

    TWBR = 72;
    TWSR = 0;
    TWCR = (1 << TWEN);

    sei();

    usart_print("TWI scanner\r\n");

    for (;;) {
        for (uint8_t addr = 1; addr < 128; addr++) {
            TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
            while (!(TWCR & (1 << TWINT)));
            TWDR = (addr << 1) | 0;
            TWCR = (1 << TWINT) | (1 << TWEN);
            while (!(TWCR & (1 << TWINT)));
            if ((TWSR & 0xF8) == 0x18) {
                usart_print(" OK\r\n");
            }
            TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
        }
    }
}
