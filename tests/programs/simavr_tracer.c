#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim_avr.h"
#include "sim_elf.h"
#include <stdbool.h>

static uint8_t smart_read(avr_t *avr, uint16_t addr) {
    if (addr == 32 + 0x3F) { // SREG
        uint8_t s = 0;
        for (int i = 0; i < 8; i++) if (avr->sreg[i]) s |= (1 << i);
        return s;
    }

    // For I/O registers, check if there's a read callback (like for Timers)
    // SimAVR's avr->data[addr] might be stale if the peripheral calculates the value on-demand.
    // However, we must be careful with UART registers to avoid side effects.
    if (addr >= 32 && addr < 32 + 64 + 160) { // IO and Extended IO
        int io_addr = addr - 32;
        // Don't call read handlers for UART status/data to avoid clearing RXC/UDRE
        bool is_uart = (addr >= 0xc0 && addr <= 0xc6); 
        if (!is_uart && avr->io[io_addr].r.c) {
            return avr->io[io_addr].r.c(avr, addr, avr->io[io_addr].r.param);
        }
    }

    return avr->data[addr];
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <elf> <cycles>\n", argv[0]);
        return 1;
    }

    const char *elf_file = argv[1];
    long cycles = atol(argv[2]);
    fprintf(stderr, "Tracer: Requested %ld cycles for %s\n", cycles, elf_file);

    avr_t *avr = avr_make_mcu_by_name("atmega328p");
    if (!avr) {
        fprintf(stderr, "Failed to create MCU atmega328p\n");
        return 1;
    }
    avr_init(avr);

    elf_firmware_t f;
    memset(&f, 0, sizeof(f));
    if (elf_read_firmware(elf_file, &f) != 0) {
        fprintf(stderr, "Failed to read ELF %s\n", elf_file);
        return 1;
    }
    printf("Firmware: %s, MMCU: %s, F_CPU: %u\n", elf_file, f.mmcu, f.frequency);
    avr_load_firmware(avr, &f);
    avr->frequency = 16000000L;
    avr_reset(avr);

    // Print Header matching cpu_trace_util.cpp
    printf("Cycle,PC,SREG,SP");
    for (int i = 0; i < 32; i++) printf(",R%d", i);
    printf(",PORTB,DDRB,PORTD,DDRD,TCNT1L,TCNT1H,TCNT0,ADCSRA,ACSR,State\n");
    fflush(stdout);

    for (long i = 0; i < cycles; i++) {
        uint8_t spl = smart_read(avr, 32 + 0x3D);
        uint8_t sph = smart_read(avr, 32 + 0x3E);
        uint16_t sp = spl | (sph << 8);
        uint8_t sreg = smart_read(avr, 32 + 0x3F);

        printf("%lu,%04x,%02x,%04x", (unsigned long)avr->cycle, avr->pc, sreg, sp);
        for (int r = 0; r < 32; r++) printf(",%02x", smart_read(avr, r));
        
        printf(",%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%d\n",
               smart_read(avr, 0x25), // PORTB
               smart_read(avr, 0x24), // DDRB
               smart_read(avr, 0x2b), // PORTD
               smart_read(avr, 0x2a), // DDRD
               smart_read(avr, 0x84), // TCNT1L
               smart_read(avr, 0x85), // TCNT1H
               smart_read(avr, 0x46), // TCNT0
               smart_read(avr, 0x7a), // ADCSRA
               smart_read(avr, 0x50), // ACSR
               avr->state);
        fflush(stdout);

        avr_run(avr);
        if (avr->state == cpu_Done || avr->state == cpu_Stopped) break;
    }
    return 0;
}
