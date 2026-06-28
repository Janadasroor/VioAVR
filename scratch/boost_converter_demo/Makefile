MCU = atmega328p
F_CPU = 16000000UL
CC = avr-gcc
OBJCOPY = avr-objcopy
CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -Wall -Wextra

all: firmware.hex

firmware.elf: firmware.c
	$(CC) $(CFLAGS) -o $@ $<

firmware.hex: firmware.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

clean:
	rm -f firmware.elf firmware.hex
