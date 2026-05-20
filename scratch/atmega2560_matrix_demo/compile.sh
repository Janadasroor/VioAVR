#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

echo "Compiling led_matrix.c for ATmega2560..."
avr-gcc -Wall -Os -g -mmcu=atmega2560 -DF_CPU=16000000UL -c led_matrix.c -o led_matrix.o

echo "Linking led_matrix.elf..."
avr-gcc -g -mmcu=atmega2560 led_matrix.o -o led_matrix.elf

echo "Extracting led_matrix.hex..."
avr-objcopy -j .text -j .data -O ihex led_matrix.elf led_matrix.hex

echo "Success! led_matrix.hex generated for ATmega2560."
ls -lh led_matrix.hex
