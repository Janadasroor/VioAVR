#include <avr/io.h>

.section .text
.global main

main:
    ldi r16, 0xAA
    ldi r17, 0x55
break_here:
    add r16, r17
    nop
    rjmp main
