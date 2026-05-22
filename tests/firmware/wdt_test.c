#include <avr/io.h>
#include <avr/wdt.h>

int main(void) {
    uint8_t cause = MCUSR;
    MCUSR = 0;

    DDRB = 0xFF;

    if (cause & (1 << WDRF)) {
        PORTB = 0xAA;
    } else {
        PORTB = 0x55;
    }

    wdt_enable(WDTO_15MS);

    while (1);
}
