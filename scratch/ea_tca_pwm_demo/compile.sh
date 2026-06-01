#!/bin/bash
avr-gcc -mmcu=avr16ea28 -Os -DF_CPU=16000000UL -o fw.elf main.c 2>&1
avr-objcopy -O ihex fw.elf firmware.hex 2>&1
