#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

echo "Compiling adc_demo.c..."
avr-gcc -Wall -Os -g -mmcu=atmega328p -DF_CPU=16000000UL -c adc_demo.c -o adc_demo.o

echo "Linking adc_demo.elf..."
avr-gcc -g -mmcu=atmega328p adc_demo.o -o adc_demo.elf

echo "Extracting adc_demo.hex..."
avr-objcopy -j .text -j .data -O ihex adc_demo.elf adc_demo.hex

echo "Success! adc_demo.hex generated."
ls -l adc_demo.hex
