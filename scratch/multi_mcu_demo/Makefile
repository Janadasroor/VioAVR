# Multi-MCU Demo — Firmware Build
# 2026-06-03

AVR_CC  = avr-gcc
AVR_OBJ = avr-objcopy
CFLAGS  = -Os -DF_CPU=16000000UL -mmcu=
LDFLAGS =

.PHONY: all clean

all: master.hex slave.hex

master.elf: master.c
	$(AVR_CC) $(CFLAGS)atmega4809 $(LDFLAGS) -o $@ $^

slave.elf: slave.c
	$(AVR_CC) $(CFLAGS)atmega328p $(LDFLAGS) -o $@ $^

%.hex: %.elf
	$(AVR_OBJ) -O ihex -R .eeprom $< $@

clean:
	rm -f *.elf *.hex
