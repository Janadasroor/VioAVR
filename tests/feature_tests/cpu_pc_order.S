#define __SFR_OFFSET 0
#include <avr/io.h>

.section .text
.global main
main:
    ; PC 0x00: Configure Watchdog (8ms timeout, reset mode)
    ldi r16, (1<<WDCE) | (1<<WDE)
    sts _SFR_MEM_ADDR(WDTCSR), r16
    ldi r16, (1<<WDE) ; WDP=0 -> 16ms approx
    sts _SFR_MEM_ADDR(WDTCSR), r16

    ; LDI some values to registers to see trace progress
    ldi r20, 0x42
    
loop:
    ; PC 0x06: RJMP loop
    ; In VioAVR (BROKEN), if watchdog triggers here:
    ; 1. advance_cycles(2) calls reset(). PC = 0.
    ; 2. execute_rjmp sets PC = 0x06 (the jump target).
    ; 3. Final PC seen in trace is 0x06.
    
    ; In SimAVR/VioAVR (FIXED):
    ; 1. execute_rjmp sets PC = 0x06.
    ; 2. advance_cycles(2) calls reset(). PC = 0.
    ; 3. Final PC seen in trace is 0x00.
    rjmp loop
