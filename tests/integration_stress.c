#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdint.h>

#define BAUD 9600
#define MYUBRR (F_CPU/16/BAUD-1)

volatile uint32_t system_ticks = 0;
volatile uint16_t adc_result = 0;
volatile uint8_t adc_done = 0;

// UART TX Buffer
#define TX_BUFFER_SIZE 64
volatile uint8_t tx_buffer[TX_BUFFER_SIZE];
volatile uint8_t tx_write_idx = 0;
volatile uint8_t tx_read_idx = 0;

void uart_putc(char c) {
    uint8_t next = (tx_write_idx + 1) % TX_BUFFER_SIZE;
    // Wait if full
    while (next == tx_read_idx) {
        __asm__ __volatile__ ("nop");
    }
    tx_buffer[tx_write_idx] = c;
    tx_write_idx = next;
    // Enable UDRE interrupt
    UCSR0B |= (1 << UDRIE0);
}

void uart_print(const char* s) {
    while (*s) uart_putc(*s++);
}

void uart_print_u16(uint16_t v) {
    char buf[6];
    uint8_t i = 0;
    if (v == 0) buf[i++] = '0';
    else {
        while (v > 0) {
            buf[i++] = '0' + (v % 10);
            v /= 10;
        }
    }
    while (i > 0) uart_putc(buf[--i]);
}

void uart_print_u32(uint32_t v) {
    char buf[11];
    uint8_t i = 0;
    if (v == 0) buf[i++] = '0';
    else {
        while (v > 0) {
            buf[i++] = '0' + (v % 10);
            v /= 10;
        }
    }
    while (i > 0) uart_putc(buf[--i]);
}

ISR(USART_UDRE_vect) {
    if (tx_read_idx != tx_write_idx) {
        UDR0 = tx_buffer[tx_read_idx];
        tx_read_idx = (tx_read_idx + 1) % TX_BUFFER_SIZE;
    } else {
        UCSR0B &= ~(1 << UDRIE0);
    }
}

ISR(TIMER0_COMPA_vect) {
    system_ticks++;
}

ISR(ADC_vect) {
    adc_result = ADC;
    adc_done = 1;
}

int main() {
    // 1. EEPROM Test: Write then Read for predictability in runner
    uint32_t eeprom_magic = 0x56494F38; // "VIO8"
    uint32_t val = 0;
    eeprom_write_block(&eeprom_magic, (void*)0, 4);
    eeprom_read_block(&val, (void*)0, 4);

    // 2. UART Init
    UBRR0H = (uint8_t)(MYUBRR >> 8);
    UBRR0L = (uint8_t)MYUBRR;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (3 << UCSZ00);

    // 3. Timer0 Init (1ms)
    TCCR0A = (1 << WGM01); // CTC
    TCCR0B = (1 << CS01) | (1 << CS00); // 64 prescaler
    OCR0A = 249; 
    TIMSK0 |= (1 << OCIE0A);

    // 4. Timer1 Init (PWM)
    // OC1A (PB1) must be set to output for PWM to appear on the pin
    DDRB |= (1 << 1); 
    TCCR1A = (1 << COM1A1) | (1 << WGM10); // Fast PWM 8-bit
    TCCR1B = (1 << WGM12) | (1 << CS11); // 8 prescaler
    OCR1A = 127; // 50% duty cycle

    // 5. ADC Init
    ADMUX = (1 << REFS0); // AVCC
    ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1); // /64 prescaler

    sei();

    uart_print("INTEGRATION-START\n");
    uart_print("E:");
    uart_print_u32(val);
    uart_print("\n");

    uint32_t last_report = 0;
    while (1) {
        if (system_ticks - last_report >= 100) {
            last_report = system_ticks;
            ADCSRA |= (1 << ADSC);
        }

        if (adc_done) {
            adc_done = 0;
            uart_print("T:");
            uart_print_u32(system_ticks);
            uart_print(" A:");
            uart_print_u16(adc_result);
            uart_print("\n");

            if (system_ticks >= 1200) {
                uart_print("INTEGRATION-DONE-V2\n");
                while(1);
            }
        }
    }

    return 0;
}
