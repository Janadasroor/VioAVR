#include <avr/io.h>

int main(void) {
    PORTC_DIR = 0x30; // PC4 (DH0), PC5 (DL0) as outputs

    TCC0_CTRLA = 0; // stop
    TCC0_PER = 399;
    TCC0_CCA = 50;  // 12.5% duty, ~6.25us LOW / ~6.25us HIGH @ 32MHz

    TCC0_CTRLB = TC0_CCAEN_bm;  // enable WO0 output via CCA match
    TCC0_CTRLD = TC_WGMODE_SINGLESLOPE_gc; // single-slope PWM

    // AWEX with dead-time insertion
    AWEXC_CTRL = 0;
    AWEXC_DTBOTH = 20; // 20 cycle dead-time (~0.625us @ 32MHz)
    AWEXC_CTRL = AWEX_DTICCAEN_bm; // enable dead-time on channel 0 (WO0)

    TCC0_CTRLA = 0x01; // enable clock, prescaler=1

    while (1);
}
