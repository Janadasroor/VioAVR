#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

echo "Compiling pwm_driver.c..."
avr-gcc -Wall -Os -g -mmcu=atmega328p -DF_CPU=16000000UL -c pwm_driver.c -o pwm_driver.o

echo "Linking pwm_driver.elf..."
avr-gcc -g -mmcu=atmega328p pwm_driver.o -o pwm_driver.elf

echo "Extracting pwm_driver.hex..."
avr-objcopy -j .text -j .data -O ihex pwm_driver.elf pwm_driver.hex

echo "Success! pwm_driver.hex generated."
ls -l pwm_driver.hex
