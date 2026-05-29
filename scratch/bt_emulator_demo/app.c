#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// ATmega4809 — Application firmware
// USART0 on PA0(RXD)/PA1(TXD) → ext ID 0/1
// Communicates with BT module emulator

#define BUF_MAX 64
static char rx_buf[BUF_MAX];
static uint8_t rx_idx;

static void tx_char(char c) {
    while (!(USART0_STATUS & USART_DREIF_bm));
    USART0_TXDATAL = c;
}
static void tx_str(const char* s) { while (*s) tx_char(*s++); }

ISR(USART0_RXC_vect) {
    uint8_t byte = USART0_RXDATAL;
    if (rx_idx < BUF_MAX-1) rx_buf[rx_idx++] = byte;
}

int main(void) {
    // USART0: 115200 8N1, RX interrupt
    USART0_BAUD  = 556;
    USART0_CTRLA = USART_RXCIE_bm;
    USART0_CTRLB = USART_TXEN_bm | USART_RXEN_bm;
    USART0_CTRLC = USART_CMODE_ASYNCHRONOUS_gc
                 | USART_PMODE_DISABLED_gc
                 | USART_SBMODE_1BIT_gc
                 | USART_CHSIZE_8BIT_gc;

    sei();

    // Set PA0 as output for activity indicator
    PORTA_DIR = PIN0_bm;
    PORTA_OUT &= ~PIN0_bm;
    uint8_t led = 0;

    _delay_ms(1);               // wait for BT to boot

    // Configure BT module
    tx_str("AT\r\n");
    _delay_ms(2);

    tx_str("AT+NAME=BT_SIM\r\n");
    _delay_ms(2);

    tx_str("AT+PSWD=0000\r\n");
    _delay_ms(2);

    tx_str("AT+ROLE=0\r\n");
    _delay_ms(2);

    // Enter data mode
    tx_str("AT+INIT\r\n");
    _delay_ms(2);

    uint8_t counter = 0;

    for (;;) {
        tx_char('H'); tx_char('e'); tx_char('l'); tx_char('l'); tx_char('o');
        tx_char(' ');
        tx_char('0' + (counter / 10));
        tx_char('0' + (counter % 10));
        tx_char('\r'); tx_char('\n');
        counter = (counter + 1) % 100;

        // Toggle activity LED
        if (led) PORTA_OUT |= PIN0_bm;
        else PORTA_OUT &= ~PIN0_bm;
        led ^= 1;

        _delay_ms(10);          // ~100 msgs per second
    }
}
