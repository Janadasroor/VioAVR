#include <avr/io.h>
#include <stdio.h>

/**
 * ATmega328P Voltmeter Firmware
 * 
 * Target: ATmega328P
 * Logic:
 *   - Samples ADC0 (PC0)
 *   - Converts to millivolts (assuming 5V Vref)
 *   - Outputs formatted string to UART
 */

void uart_init(void) {
    // 9600 baud @ 16MHz
    UBRR0H = 0;
    UBRR0L = 103;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_putchar(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

void uart_print(const char* s) {
    while (*s) uart_putchar(*s++);
}

void adc_init(void) {
    ADMUX = (1 << REFS0); // AVCC ref
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1); // Enable, div 64
}

uint16_t adc_read(void) {
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

int main(void) {
    uart_init();
    adc_init();
    
    char buffer[16];
    
    while (1) {
        uint16_t val = adc_read();
        
        // (val * 5000) / 1024
        uint32_t mv = ((uint32_t)val * 5000) / 1024;
        
        uint16_t whole = mv / 1000;
        uint16_t frac = mv % 1000;
        
        sprintf(buffer, "V: %u.%03u\r\n", whole, frac);
        uart_print(buffer);
        
        // Delay loop
        for (volatile uint32_t i = 0; i < 10000; i++);
    }
    
    return 0;
}
