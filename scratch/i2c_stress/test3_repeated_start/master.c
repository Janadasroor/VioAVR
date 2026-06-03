#include <avr/io.h>

#define SLAVE_ADDR 0x42

int main(void)
{
    PORTA.DIRSET = PIN0_bm;
    PORTA.OUTCLR = PIN0_bm;

    TWI0.MBAUD   = 75;
    TWI0.MCTRLA  = TWI_ENABLE_bm | TWI_SMEN_bm;
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;

    for (;;) {
        TWI0.MADDR = (SLAVE_ADDR << 1) | 0;
        while (!(TWI0.MSTATUS & TWI_WIF_bm));

        if (TWI0.MSTATUS & TWI_RXACK_bm) {
            TWI0.MCTRLB = TWI_MCMD_STOP_gc;
        } else {
            TWI0.MDATA = 0xA5;
            while (!(TWI0.MSTATUS & TWI_WIF_bm));

            TWI0.MADDR = (SLAVE_ADDR << 1) | 1;
            while (!(TWI0.MSTATUS & TWI_RIF_bm));

            uint8_t rx = TWI0.MDATA;

            TWI0.MCTRLB = TWI_MCMD_STOP_gc;

            if (rx == 0xA5) {
                PORTA.OUTSET = PIN0_bm;
            }
        }

        for (volatile unsigned long d = 0; d < 50000; d++);
        PORTA.OUTCLR = PIN0_bm;
    }
}
