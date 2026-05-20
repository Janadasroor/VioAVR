#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

echo "Compiling attiny85_demo.c..."
avr-gcc -Wall -Os -g -mmcu=attiny85 -DF_CPU=16000000UL -c attiny85_demo.c -o attiny85_demo.o

echo "Linking attiny85_demo.elf..."
avr-gcc -g -mmcu=attiny85 attiny85_demo.o -o attiny85_demo.elf

echo "Extracting attiny85_demo.hex..."
avr-objcopy -j .text -j .data -O ihex attiny85_demo.elf attiny85_demo.hex

echo "Success! attiny85_demo.hex generated."
ls -l attiny85_demo.hex
