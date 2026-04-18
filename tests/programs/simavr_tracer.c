#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim_avr.h"
#include "sim_elf.h"

static uint8_t smart_read(avr_t *avr, uint16_t addr) {
    if (addr == 32 + 0x3F) { // SREG
        uint8_t s = 0;
        for (int i = 0; i < 8; i++) if (avr->sreg[i]) s |= (1 << i);
        return s;
    }
    // For simavr, avr->data usually contains the register values
    // unless the peripheral hasn't synced it. 
    // We avoid calling read callbacks to prevent side effects (e.g. clearing RXC/UDRE).
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
    fprintf(stderr, "Tracer: MCU %s, FlashEnd=%u, RamEnd=%u\n", avr->mmcu, avr->flashend, avr->ramend);
    avr->frequency = 16000000L;
    avr_init(avr);

    for (struct avr_io_t *p = avr->io_port; p; p = p->next) {
        fprintf(stderr, "Tracer: Attached IO: %s\n", p->kind);
    }

    elf_firmware_t f;
    memset(&f, 0, sizeof(f));
    if (elf_read_firmware(elf_file, &f) != 0) {
        fprintf(stderr, "Failed to read ELF %s\n", elf_file);
        return 1;
    }
    printf("Firmware: %s, MMCU: %s, F_CPU: %lu\n", elf_file, f.mmcu, f.frequency);
    avr_load_firmware(avr, &f);
    avr->frequency = 16000000L;
    avr_reset(avr);

    // Initial print
    printf("Cycle,PC,SREG,SP");
    for (int i = 0; i < 32; i++) printf(",R%d", i);
    printf(",TCNT0,ADCSRA,ACSR,UCSR0A,UDR0,UCSR0B,State\n");
    fflush(stdout);

    if (!avr->data) {
        fprintf(stderr, "Error: avr->data is NULL\n");
        return 1;
    }

    // Debug dump
    fprintf(stderr, "Simulator Memory Map Debug (0x00-0xFF):\n");
    for (int j = 0; j < 256; j++) {
        if (j % 16 == 0) fprintf(stderr, "\n%02x: ", j);
        fprintf(stderr, "%02x ", avr->data[j]);
    }
    fprintf(stderr, "\n");

    for (long i = 0; i < cycles; i++) {
        uint8_t spl = smart_read(avr, 32 + 0x3D);
        uint8_t sph = smart_read(avr, 32 + 0x3E);
        uint16_t sp = spl | (sph << 8);
        uint8_t sreg = smart_read(avr, 32 + 0x3F);

        // SimAVR state
        if (i < 5) fprintf(stderr, "Tracer: Iteration %ld, PC=%04x\n", (long)i, avr->pc);
        printf("%lu,%04x,%02x,%04x",
               (unsigned long)avr->cycle, avr->pc, sreg, sp);
        
        for (int r = 0; r < 32; r++) printf(",%02x", smart_read(avr, r));
        
        // I/O Registers (ATmega328P Memory Mapped Offsets)
        printf(",%02x,%02x,%02x,%02x,%02x,%02x,%d\n",
               smart_read(avr, 0x46), // TCNT0
               smart_read(avr, 0x7A), // ADCSRA
               smart_read(avr, 0x50), // ACSR
               smart_read(avr, 0xC0), // UCSR0A
               smart_read(avr, 0xC6), // UDR0
               smart_read(avr, 0xC1), // UCSR0B
               avr->state);
        fflush(stdout);

        avr_run(avr);
        if (avr->state == cpu_Done || avr->state == cpu_Stopped) {
            fprintf(stderr, "Simulator stopped naturally at cycle %lu (state %d)\n", (unsigned long)avr->cycle, avr->state);
            break;
        }
    }

    return 0;
}
