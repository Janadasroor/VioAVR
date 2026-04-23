#include <avr/io.h>
#include <avr/interrupt.h>

/* ATmega4809 USART0 Stress Test */

#ifndef USART0
#define USART0 (*(USART_t *) 0x0800)
#endif

#ifndef USART_DREIF_bm
#define USART_DREIF_bm 0x20
#endif

#ifndef USART_RXEN_bm
#define USART_RXEN_bm 0x80
#endif

#ifndef USART_TXEN_bm
#define USART_TXEN_bm 0x40
#endif

#ifndef USART_RXCIE_bm
#define USART_RXCIE_bm 0x80
#endif

ISR(USART0_RXC_vect) {
    uint8_t data = USART0.RXDATAL;
    /* Wait for empty transmit buffer */
    while (!(USART0.STATUS & USART_DREIF_bm));
    /* Echo incremented */
    USART0.TXDATAL = data + 1;
}

int main(void) {
    /* Set baud rate: 38400 @ 16MHz */
    /* BAUD = (64 * 16,000,000) / (16 * 38,400) = 6666.666 -> 6667 */
    USART0.BAUD = 6667;
    
    /* Enable receiver, transmitter and RX complete interrupt */
    USART0.CTRLB = USART_RXEN_bm | USART_TXEN_bm;
    USART0.CTRLA = USART_RXCIE_bm;

    sei();

    /* Send startup message */
    const char* hello = "VioAVR-OK\n";
    for (const char* p = hello; *p; p++) {
        while (!(USART0.STATUS & USART_DREIF_bm));
        USART0.TXDATAL = *p;
    }

    while (1) {
        /* Everything else is interrupt-driven */
    }
    
    return 0;
}
