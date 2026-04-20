#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define BAUD 38400
#define MYUBRR (16000000UL/16/BAUD-1)

volatile uint8_t rx_buffer[32];
volatile uint8_t rx_write_idx = 0;
volatile uint8_t rx_read_idx = 0;

volatile uint8_t tx_buffer[32];
volatile uint8_t tx_write_idx = 0;
volatile uint8_t tx_read_idx = 0;

ISR(USART_RX_vect) {
    uint8_t data = UDR0;
    uint8_t next = (rx_write_idx + 1) % 32;
    if (next != rx_read_idx) {
        rx_buffer[rx_write_idx] = data;
        rx_write_idx = next;
    }
}

ISR(USART_UDRE_vect) {
    if (tx_read_idx != tx_write_idx) {
        UDR0 = tx_buffer[tx_read_idx];
        tx_read_idx = (tx_read_idx + 1) % 32;
    } else {
        UCSR0B &= ~(1 << UDRIE0);
    }
}

void tx_send(uint8_t data) {
    uint8_t next = (tx_write_idx + 1) % 32;
    // Wait if buffer full
    while (next == tx_read_idx);
    
    tx_buffer[tx_write_idx] = data;
    tx_write_idx = next;
    
    // Enable UDRE interrupt
    UCSR0B |= (1 << UDRIE0);
}

int main(void) {
    // Init UART
    UBRR0H = (uint8_t)(MYUBRR >> 8);
    UBRR0L = (uint8_t)MYUBRR;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    UCSR0C = (3 << UCSZ00); // 8N1
    
    sei();
    
    const char* hello = "VioAVR-OK\n";
    for (const char* p = hello; *p; p++) {
        tx_send(*p);
    }
    
    while (1) {
        if (rx_read_idx != rx_write_idx) {
            uint8_t data = rx_buffer[rx_read_idx];
            rx_read_idx = (rx_read_idx + 1) % 32;
            
            // Process: increment byte and echo (A -> B)
            tx_send(data + 1);
        }
    }
}
