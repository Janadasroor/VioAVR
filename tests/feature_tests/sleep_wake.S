#define __SFR_OFFSET 0
#include <avr/io.h>

.section .vectors
.org 0x00
    rjmp main
.org 0x1C ; Timer0 Compare A vector (0x0E << 1)
    ldi r18, 0xAA
    reti

.section .text
.global main
main:
    ; Reset stack pointer
    ldi r16, 0x08
    out SPH, r16
    ldi r16, 0xFF
    out SPL, r16

    ; Setup Compare Match
    ldi r16, 0x10 ; 16 cycles
    out OCR0A, r16
    ldi r16, 0x02 ; OCIE0A
    sts TIMSK0, r16
    sei
    
    ; Setup Sleep
    ldi r16, 0x01 ; SE (Idle)
    out SMCR, r16
    
    ; Start Timer
    ldi r16, 0x01 ; clk/1
    out TCCR0B, r16
    
    sleep ; Trigger sleep
    
    ; After wake-up
    ldi r17, 0x55
    rjmp main
