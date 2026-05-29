#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

// ATmega4809 — HC-05 BT module emulator
// USART0 on PA0(RXD)/PA1(TXD) → ext ID 0/1
// BAUD = 556 → ~115200 baud

#define BUF_MAX 80
static char rx_buf[BUF_MAX];
static uint8_t rx_len;
static char cfg_name[16] = "BT_SIM";
static char cfg_pin[8]   = "1234";
static uint8_t cfg_role  = 0;

enum { MODE_AT, MODE_DATA };
static uint8_t mode = MODE_AT;

static void tx_char(char c) {
    while (!(USART0_STATUS & USART_DREIF_bm));
    USART0_TXDATAL = c;
}
static void tx_str(const char* s) { while (*s) tx_char(*s++); }
static void tx_crlf(void) { tx_str("\r\n"); }
static void tx_ok(void)   { tx_str("OK\r\n"); }
static void tx_err(void)  { tx_str("ERROR\r\n"); }

static void set_baud(uint32_t baud) {
    if (baud < 1200) return;
    uint32_t reg = (64UL * 16000000UL) / (16UL * baud);
    if (reg > 65535) reg = 65535;
    if (reg < 1) reg = 1;
    USART0_BAUD = (uint16_t)reg;
}

static void handle_at(const char* cmd) {
    if (!cmd[0]) { tx_ok(); return; }
    if (strcmp(cmd, "AT") == 0) { tx_ok(); return; }

    if (strncmp(cmd, "AT+VERSION", 10) == 0) {
        tx_str("+VERSION:2.0-20200601\r\n"); tx_ok(); return;
    }
    if (strncmp(cmd, "AT+NAME", 7) == 0) {
        if (cmd[7] == '=') {
            uint8_t i; for (i = 0; cmd[8+i] && i < 14; i++) cfg_name[i] = cmd[8+i];
            cfg_name[i] = 0; tx_ok();
        } else if (cmd[7] == '?') {
            tx_str("+NAME:"); tx_str(cfg_name); tx_crlf(); tx_ok();
        } else tx_err();
        return;
    }
    if (strncmp(cmd, "AT+ROLE", 7) == 0) {
        if (cmd[7] == '=' && cmd[8] >= '0' && cmd[8] <= '1') {
            cfg_role = cmd[8]-'0'; tx_ok();
        } else if (cmd[7] == '?') {
            tx_str("+ROLE:"); tx_char('0'+cfg_role); tx_crlf(); tx_ok();
        } else tx_err();
        return;
    }
    if (strncmp(cmd, "AT+PSWD", 7) == 0) {
        if (cmd[7] == '=') {
            uint8_t i; for (i = 0; cmd[8+i] && i < 6; i++) cfg_pin[i] = cmd[8+i];
            cfg_pin[i] = 0; tx_ok();
        } else { tx_err(); }
        return;
    }
    if (strncmp(cmd, "AT+UART", 7) == 0) {
        if (cmd[7] == '=') {
            uint32_t b = 0; uint8_t i = 8;
            while (cmd[i] >= '0' && cmd[i] <= '9') b = b*10 + cmd[i++]-'0';
            if (b >= 1200) { set_baud(b); tx_ok(); }
            else tx_err();
        } else tx_err();
        return;
    }
    if (strcmp(cmd, "AT+STATE?") == 0) {
        tx_str("+STATE:"); tx_char('0'+ (mode==MODE_DATA)); tx_crlf(); tx_ok(); return;
    }
    if (strcmp(cmd, "AT+INIT") == 0) {
        mode = MODE_DATA; tx_ok(); return;
    }
    if (strcmp(cmd, "AT+DEFAULT") == 0) {
        strcpy(cfg_name, "BT_SIM"); strcpy(cfg_pin, "1234"); cfg_role = 0;
        set_baud(115200); tx_ok(); return;
    }
    if (strcmp(cmd, "AT+RESET") == 0) {
        tx_ok(); void (*rst)(void) = 0; rst();
    }

    tx_err();
}

ISR(USART0_RXC_vect) {
    uint8_t byte = USART0_RXDATAL;
    if (mode == MODE_AT) {
        if (byte == '\r' || byte == '\n') {
            if (rx_len > 0) {
                rx_buf[rx_len] = 0;
                tx_crlf();
                handle_at(rx_buf);
                rx_len = 0;
            }
        } else if (byte >= ' ' && rx_len < BUF_MAX-1) {
            rx_buf[rx_len++] = byte;
        }
    } else {
        // Data mode: echo back to APP
        while (!(USART0_STATUS & USART_DREIF_bm));
        USART0_TXDATAL = byte;
    }
}

ISR(USART0_DRE_vect) { /* unused */ }

int main(void) {
    USART0_BAUD = 556;                                          // 115200 baud
    USART0_CTRLA = USART_RXCIE_bm;                              // RX interrupt
    USART0_CTRLB = USART_TXEN_bm | USART_RXEN_bm;              // enable TX/RX
    USART0_CTRLC = USART_CMODE_ASYNCHRONOUS_gc
                 | USART_PMODE_DISABLED_gc
                 | USART_SBMODE_1BIT_gc
                 | USART_CHSIZE_8BIT_gc;

    sei();

    tx_str("\r\nHC-05 EMULATOR\r\nOK\r\n");
    rx_len = 0;
    mode = MODE_AT;

    for (;;) { /* interrupts handle everything */ }
}
