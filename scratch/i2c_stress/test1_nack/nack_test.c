#include <avr/io.h>

#define F_CPU 16000000UL

int main(void)
{
    PORTA.DIRSET = PIN0_bm;
    PORTA.OUTCLR = PIN0_bm;

    TWI0.MBAUD   = 75;
    TWI0.MCTRLA  = TWI_ENABLE_bm;
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;

    for (;;) {
        TWI0.MADDR = 0x21; // addr 0x10 + W (no slave at this addr)
        while (!(TWI0.MSTATUS & TWI_WIF_bm));

        if (TWI0.MSTATUS & TWI_RXACK_bm) {
            TWI0.MCTRLB = TWI_MCMD_STOP_gc;
            PORTA.OUTTGL = PIN0_bm;
        }

        for (volatile unsigned long i = 0; i < 10000; i++);
    }
}
