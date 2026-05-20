#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

echo "Compiling atmega168_demo.c..."
avr-gcc -Wall -Os -g -mmcu=atmega168 -DF_CPU=16000000UL -c atmega168_demo.c -o atmega168_demo.o

echo "Linking atmega168_demo.elf..."
avr-gcc -g -mmcu=atmega168 atmega168_demo.o -o atmega168_demo.elf

echo "Extracting atmega168_demo.hex..."
avr-objcopy -j .text -j .data -O ihex atmega168_demo.elf atmega168_demo.hex

echo "Success! atmega168_demo.hex generated."
ls -l atmega168_demo.hex
