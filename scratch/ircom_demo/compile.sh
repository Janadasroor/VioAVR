#!/bin/bash
avr-gcc -mmcu=atxmega8e5 -Os -DF_CPU=16000000UL -o main.elf main.c 2>&1
avr-objcopy -O ihex main.elf firmware.hex 2>&1
rm -f main.elf
