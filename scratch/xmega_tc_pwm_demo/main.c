#include <avr/io.h>

int main(void) {
    // TC4 drives PORTC bit 4 (WO0). The PinMux claim_pin sets DDR.
    TCC4_CTRLA = 0x03;   // enable + clk/1 (CLKSEL=1, not 0=stopped)
    TCC4_CTRLB = 0x10;   // CCAEN (WO0 on bit 4)
    TCC4_CTRLD = 0x03;   // WGMODE = 3 (SS PWM, in CTRLD for XMEGA TC)
    TCC4_PER = 399;
    TCC4_CCA = 200;
    while (1);
}
