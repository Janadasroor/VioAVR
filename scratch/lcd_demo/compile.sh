#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

echo "Compiling lcd_demo.c..."
avr-gcc -Wall -Os -g -mmcu=atmega3290p -DF_CPU=16000000UL -c lcd_demo.c -o lcd_demo.o

echo "Linking lcd_demo.elf..."
avr-gcc -g -mmcu=atmega3290p lcd_demo.o -o lcd_demo.elf

echo "Extracting lcd_demo.hex..."
avr-objcopy -j .text -j .data -O ihex lcd_demo.elf lcd_demo.hex

echo "Success! lcd_demo.hex generated."
ls -l lcd_demo.hex
