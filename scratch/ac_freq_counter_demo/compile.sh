#!/bin/bash
avr-gcc -mmcu=atmega4809 -Os -DF_CPU=16000000UL -o fw.elf main.c 2>&1
avr-objcopy -j .text -j .data -O ihex fw.elf firmware.hex
rm -f fw.elf
avr-objdump -d firmware.hex 2>/dev/null | head -80 || true
echo "---"
ls -la firmware.hex
