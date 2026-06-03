#include <avr/io.h>

#define SLAVE_ADDR 0x42

static const uint8_t tx_data[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
};

int main(void)
{
    TWI0.MBAUD   = 75;
    TWI0.MCTRLA  = TWI_ENABLE_bm;
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;

    for (;;) {
        TWI0.MADDR = (SLAVE_ADDR << 1) | 0;
        while (!(TWI0.MSTATUS & TWI_WIF_bm));

        if (TWI0.MSTATUS & TWI_RXACK_bm) {
            TWI0.MCTRLB = TWI_MCMD_STOP_gc;
        } else {
            for (int i = 0; i < 16; i++) {
                TWI0.MDATA = tx_data[i];
                while (!(TWI0.MSTATUS & TWI_WIF_bm));
                if (TWI0.MSTATUS & TWI_RXACK_bm) break;
            }
            TWI0.MCTRLB = TWI_MCMD_STOP_gc;
        }

        for (volatile unsigned long d = 0; d < 50000; d++);
    }
}
