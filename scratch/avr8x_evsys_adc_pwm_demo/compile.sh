#!/bin/bash
avr-gcc -mmcu=atmega4809 -Os -DF_CPU=16000000UL -o fw.elf main.c
avr-objcopy -O ihex fw.elf firmware.hex
avr-objdump -d fw.elf > firmware.lst
echo "firmware.hex size: $(wc -c < firmware.hex) bytes"
