// SPI loopback test: initialize SPI master, send bytes, capture response.
// Public domain.
#include <avr/io.h>

int main(void) {
    // SPI master mode: SCK=PB5, MOSI=PB3, SS=PB2 (outputs), MISO=PB4 (input)
    DDRB = (1 << PB5) | (1 << PB3) | (1 << PB2);  // SCK, MOSI, SS as outputs
    PORTB |= (1 << PB2);  // SS high (inactive)

    // SPCR: SPE=1 (enable), MSTR=1 (master), SPR0=0,SPR1=0 (fclk/4)
    SPCR = (1 << SPE) | (1 << MSTR);

    // Transmit bytes and store received bytes to a buffer
    volatile uint8_t rxbuf[8] = {0};
    const uint8_t txbuf[8] = {0x12, 0x34, 0xAB, 0xCD, 0x55, 0xAA, 0xFF, 0x00};

    for (uint8_t i = 0; i < 8; i++) {
        SPDR = txbuf[i];           // start transmission
        while (!(SPSR & (1 << SPIF)));  // wait for complete
        rxbuf[i] = SPDR;           // read received byte
    }

    // Set PORTB bit 0 to 1 to indicate completion
    PORTB |= (1 << PB0);

    // Store results in PORTD for test verification
    PORTD = rxbuf[7];  // Last received byte

    while (1);
}
