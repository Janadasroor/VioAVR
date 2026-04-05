#define __SFR_OFFSET 0
#include <avr/io.h>

.section .vectors
.org 0x00
    rjmp main

.section .text
.global main
main:
    ; Setup ADC
    ldi r16, (1 << ADEN) | (1 << ADATE) ; Enable ADC, Enable Auto-Trigger
    sts ADCSRA, r16
    ldi r16, 0x03 ; Timer0 Overflow trigger source (ADTS = 011?) 
    ; Wait, ATmega328P ADTS for Timer0 Overflow is 0x03
    sts ADCSRB, r16
    
    ; Setup Timer0
    ldi r16, 0x01 ; clk/1
    out TCCR0B, r16
    
loop:
    lds r17, ADCSRA
    rjmp loop
