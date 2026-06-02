#include <avr/io.h>
#include <avr/interrupt.h>

ISR(TCB0_INT_vect) {
    PORTC_OUT = TCB0_CCMPL;  // use low byte first
    TCB0_INTFLAGS = TCB_CAPT_bm;
}

int main(void) {
    PORTC_DIR = 0xFF;
    PORTC_OUT = 0x01;  // heartbeat: PC0=HIGH means code runs

    // AC0: NEG=DACREF (~2.5V), POS=AINP0 (signal bank ch0), enable
    AC0_MUXCTRLA = AC_MUXNEG_DACREF_gc;
    AC0_DACREF = 128;
    AC0_CTRLA = AC_ENABLE_bm | AC_OUTEN_bm;

    // EVSYS: channel 1 = AC0_OUT, user TCB0 = channel 1
    EVSYS_CHANNEL1 = EVSYS_GENERATOR_AC0_OUT_gc;
    EVSYS_USERTCB0 = EVSYS_CHANNEL_CHANNEL1_gc;

    // TCB0: frequency mode, no prescale, event capture, capture interrupt
    TCB0_CTRLA = TCB_CLKSEL_CLKDIV1_gc;
    TCB0_CTRLB = TCB_CNTMODE_FRQ_gc;
    TCB0_EVCTRL = TCB_CAPTEI_bm;
    TCB0_INTCTRL = TCB_CAPT_bm;
    TCB0_INTFLAGS = TCB_CAPT_bm;
    TCB0_CTRLA |= TCB_ENABLE_bm;

    sei();
    for (;;) {}
}
