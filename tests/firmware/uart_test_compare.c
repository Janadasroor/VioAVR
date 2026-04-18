#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

void uart_init() {
    // 9600 baud @ 16MHz
    // UBRR = (16000000 / (16 * 9600)) - 1 = 103.16 -> 103
    UBRR0H = 0;
    UBRR0L = 103;
    
    // Enable TX
    UCSR0B = (1 << TXEN0);
    // 8N1
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_transmit(uint8_t data) {
    // Wait for empty transmit buffer
    while (!(UCSR0A & (1 << UDRE0)));
    // Put data into buffer, sends the data
    UDR0 = data;
}

int main() {
    uart_init();
    
    _delay_ms(1);
    
    const char *msg = "Hello";
    for (int i = 0; i < 5; i++) {
        uart_transmit(msg[i]);
    }
    
    while(1);
}
