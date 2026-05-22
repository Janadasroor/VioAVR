#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BAUD 9600
#define UBRR_VAL (F_CPU/16/BAUD - 1)

#define LED_PIN PB5
#define PWM_PIN PB1

static volatile uint16_t blink_interval = 0;
static volatile uint16_t blink_counter = 0;
static volatile uint8_t rx_buf[32];
static volatile uint8_t rx_len = 0;
static volatile uint8_t rx_done = 0;

static void uart_putchar(char c) {
    while (!(UCSR0A & (1<<UDRE0)));
    UDR0 = c;
}

static void uart_puts(const char* s) {
    while (*s) uart_putchar(*s++);
}

static void uart_puthex8(uint8_t v) {
    static const char hex[] = "0123456789ABCDEF";
    uart_putchar(hex[v>>4]);
    uart_putchar(hex[v&0x0F]);
}

static void uart_putdec16(uint16_t v) {
    char buf[6];
    buf[5] = 0;
    char* p = buf + 5;
    do { *--p = '0' + (v % 10); v /= 10; } while (v);
    uart_puts(p);
}

ISR(USART_RX_vect) {
    uint8_t c = UDR0;
    if (c == '\r' || c == '\n') {
        rx_done = 1;
    } else if (rx_len < sizeof(rx_buf) - 1) {
        rx_buf[rx_len++] = c;
    }
}

ISR(TIMER0_OVF_vect) {
    if (blink_interval) {
        blink_counter++;
        if (blink_counter >= blink_interval) {
            blink_counter = 0;
            PORTB ^= (1<<LED_PIN);
        }
    }
}

static void adc_init(void) {
    ADMUX = (1<<REFS0);
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1);
}

static uint16_t adc_read(uint8_t ch) {
    ADMUX = (ADMUX & 0xF0) | (ch & 0x0F);
    ADCSRA |= (1<<ADSC);
    while (ADCSRA & (1<<ADSC));
    return ADC;
}

static void pwm_init(void) {
    DDRB |= (1<<PWM_PIN);
    TCCR1A = (1<<WGM10)|(1<<COM1A1);
    TCCR1B = (1<<CS10);
    OCR1AL = 0;
}

static void pwm_set(uint8_t v) {
    OCR1AL = v;
}

static void cmd_help(void) {
    uart_puts("Commands:\r\n");
    uart_puts("  help                    - this help\r\n");
    uart_puts("  adc <ch>               - read ADC channel (0-5)\r\n");
    uart_puts("  pwm <0-255>            - set PWM duty\r\n");
    uart_puts("  blink <ms>             - set LED blink interval (0=off)\r\n");
    uart_puts("  eeprom r <addr>        - read EEPROM byte\r\n");
    uart_puts("  eeprom w <addr> <val>  - write EEPROM byte\r\n");
    uart_puts("  gpio r <port> <pin>    - read GPIO pin\r\n");
    uart_puts("  gpio w <port> <pin> <v>- write GPIO pin\r\n");
    uart_puts("  info                   - system info\r\n");
    uart_puts("  echo <text>            - echo text\r\n");
}

static void cmd_adc(char* arg) {
    uint8_t ch = atoi(arg);
    if (ch > 5) { uart_puts("ERR: ch 0-5\r\n"); return; }
    uint16_t v = adc_read(ch);
    uart_puts("ADC");
    uart_putdec16(ch);
    uart_puts(": ");
    uart_putdec16(v);
    uart_puts(" (");
    uart_putdec16((uint32_t)v * 5000UL / 1024);
    uart_puts(" mV)\r\n");
}

static void cmd_pwm(char* arg) {
    uint8_t v = atoi(arg);
    pwm_set(v);
    uart_puts("PWM: ");
    uart_putdec16(v);
    uart_puts("\r\n");
}

static void cmd_blink(char* arg) {
    uint16_t ms = atoi(arg);
    blink_interval = ms;
    blink_counter = 0;
    if (ms == 0) PORTB &= ~(1<<LED_PIN);
    uart_puts("BLINK: ");
    uart_putdec16(ms);
    uart_puts(" ms\r\n");
}

static void cmd_eeprom(char* arg) {
    if (arg[0] == 'r' && arg[1] == ' ') {
        uint16_t addr = atoi(arg + 2);
        uint8_t v = eeprom_read_byte((uint8_t*)addr);
        uart_puts("EEPROM[");
        uart_putdec16(addr);
        uart_puts("] = ");
        uart_puthex8(v);
        uart_puts("\r\n");
    } else if (arg[0] == 'w' && arg[1] == ' ') {
        char* p = arg + 2;
        uint16_t addr = atoi(p);
        while (*p && *p != ' ') p++;
        if (*p == ' ') {
            uint8_t v = atoi(p + 1);
            eeprom_write_byte((uint8_t*)addr, v);
            uart_puts("EEPROM[");
            uart_putdec16(addr);
            uart_puts("] := ");
            uart_puthex8(v);
            uart_puts("\r\n");
        }
    } else {
        uart_puts("Usage: eeprom r <addr> | eeprom w <addr> <val>\r\n");
    }
}

static void cmd_gpio(char* arg) {
    if (arg[0] == 'r' && arg[1] == ' ') {
        uint8_t port = atoi(arg + 2);
        char* p = arg + 2;
        while (*p && *p != ' ') p++;
        if (*p == ' ') {
            uint8_t pin = atoi(p + 1);
            volatile uint8_t* pin_reg = (port == 0) ? &PINB : (port == 1) ? &PINC : &PIND;
            uint8_t v = (*pin_reg >> pin) & 1;
            uart_puts("P");
            uart_putchar('A' + port);
            uart_putdec16(pin);
            uart_puts(": ");
            uart_putchar('0' + v);
            uart_puts("\r\n");
        }
    } else if (arg[0] == 'w' && arg[1] == ' ') {
        char* p = arg + 2;
        uint8_t port = atoi(p);
        while (*p && *p != ' ') p++;
        if (*p == ' ') {
            uint8_t pin = atoi(++p);
            while (*p && *p != ' ') p++;
            if (*p == ' ') {
                uint8_t v = atoi(p + 1);
                volatile uint8_t* ddr_reg = (port == 0) ? &DDRB : (port == 1) ? &DDRC : &DDRD;
                volatile uint8_t* port_reg = (port == 0) ? &PORTB : (port == 1) ? &PORTC : &PORTD;
                *ddr_reg |= (1 << pin);
                if (v) *port_reg |= (1 << pin);
                else   *port_reg &= ~(1 << pin);
                uart_puts("GPIO OK\r\n");
            }
        }
    } else {
        uart_puts("Usage: gpio r <port> <pin> | gpio w <port> <pin> <val>\r\n");
    }
}

static void cmd_info(void) {
    uart_puts("MCU: ATmega328P\r\n");
    uart_puts("F_CPU: 16 MHz\r\n");
    uart_puts("MCUSR: 0x");
    uart_puthex8(MCUSR);
    uart_puts("\r\n");
    uart_puts("BLINK: ");
    uart_putdec16(blink_interval);
    uart_puts(" ms\r\n");
    uart_puts("PWM: ");
    uart_putdec16(OCR1AL);
    uart_puts("\r\n");
}

static void cmd_echo(char* arg) {
    uart_puts(arg);
    uart_puts("\r\n");
}

static void process_command(void) {
    rx_buf[rx_len] = 0;
    char* cmd = (char*)rx_buf;
    while (*cmd == ' ') cmd++;

    if (cmd[0] == 0) { /* empty */ }
    else if (strcmp(cmd, "help") == 0) cmd_help();
    else if (strncmp(cmd, "adc ", 4) == 0) cmd_adc(cmd + 4);
    else if (strncmp(cmd, "pwm ", 4) == 0) cmd_pwm(cmd + 4);
    else if (strncmp(cmd, "blink ", 6) == 0) cmd_blink(cmd + 6);
    else if (strncmp(cmd, "eeprom ", 7) == 0) cmd_eeprom(cmd + 7);
    else if (strncmp(cmd, "gpio ", 5) == 0) cmd_gpio(cmd + 5);
    else if (strcmp(cmd, "info") == 0) cmd_info();
    else if (strncmp(cmd, "echo ", 5) == 0) cmd_echo(cmd + 5);
    else {
        uart_puts("ERR: unknown '");
        uart_puts(cmd);
        uart_puts("'. Try 'help'\r\n");
    }

    uart_puts("OK> ");
    rx_len = 0;
    rx_done = 0;
}

int main(void) {
    DDRB |= (1<<LED_PIN);

    UBRR0H = (uint8_t)(UBRR_VAL >> 8);
    UBRR0L = (uint8_t)UBRR_VAL;
    UCSR0B = (1<<TXEN0)|(1<<RXEN0)|(1<<RXCIE0);
    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);

    TCCR0A = 0;
    TCCR0B = (1<<CS01)|(1<<CS00);
    TIMSK0 = (1<<TOIE0);

    adc_init();
    pwm_init();

    sei();

    uart_puts("\r\nAVR System Monitor v1.0\r\n");
    uart_puts("Type 'help' for commands\r\nOK> ");

    while (1) {
        if (rx_done) process_command();
    }
}
