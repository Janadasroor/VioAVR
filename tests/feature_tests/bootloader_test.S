#include <avr/io.h>

.section .vectors, "ax"
    jmp main

.section .bootloader, "ax"

.global main
main:
    ; --- Part 1: SPM (Self-Programming Memory) ---
    ; We'll simulate a bootloader-like operation: Fill, Erase, Write.
    ; Target address: 0x3000 (RWW section on ATmega328P)
    
    ldi r30, 0x00
    ldi r31, 0x30
    
    ; Fill Page Buffer (just one word for brevity)
    ldi r24, 0xEF
    ldi r25, 0xBE
    mov r0, r24
    mov r1, r25
    
    ; Use bit numbers to be safe and compatible
    ldi r24, (1 << SPMEN)
    out _SFR_IO_ADDR(SPMCSR), r24
    spm ; Fill buffer at Z

    ; Page Erase
    ldi r24, (1 << PGERS) | (1 << SPMEN)
    out _SFR_IO_ADDR(SPMCSR), r24
    spm ; Erase page at Z
    
    ; AT THIS POINT: RWW Section should be busy!
    ; We can check this by reading from flash.
    lpm r20, Z ; Should return 0xFF due to RWW protection
    
    ; Page Write
    ldi r24, (1 << PGWRT) | (1 << SPMEN)
    out _SFR_IO_ADDR(SPMCSR), r24
    spm ; Write page at Z

    ; Wait for busy bit (simulated wait)
wait_spm:
    in r24, _SFR_IO_ADDR(SPMCSR)
    sbrc r24, SPMEN
    rjmp wait_spm

    ; --- Part 2: Watchdog Timer ---
    ; Start timed sequence to enable WDT
    ldi r24, (1 << WDCE) | (1 << WDE)
    sts WDTCSR, r24
    ldi r24, (1 << WDE) | (1 << WDP0) ; 32ms
    sts WDTCSR, r24
    
    wdr ; Reset watchdog
    
    ; --- Part 3: EEPROM ---
    ; Enable EERIE and trigger a write
    ldi r24, 0x00
    out _SFR_IO_ADDR(EEARL), r24
    out _SFR_IO_ADDR(EEARH), r24
    ldi r24, 0xAA
    out _SFR_IO_ADDR(EEDR), r24
    
    ldi r24, (1 << EEMPE)
    out _SFR_IO_ADDR(EECR), r24
    ldi r24, (1 << EEPE) | (1 << EERIE)
    out _SFR_IO_ADDR(EECR), r24
    
    ; Loop forever
loop:
    rjmp loop
