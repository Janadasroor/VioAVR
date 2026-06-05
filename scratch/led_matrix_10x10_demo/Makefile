MCU = atmega328p
F_CPU = 16000000UL
TARGET = firmware
CFLAGS = -mmcu=$(MCU) -Os -DF_CPU=$(F_CPU) -Wall

all: $(TARGET).hex

$(TARGET).elf: firmware.c
	avr-gcc $(CFLAGS) -o $@ $^

$(TARGET).hex: $(TARGET).elf
	avr-objcopy -O ihex -R .eeprom $< $@
	avr-size --mcu=$(MCU) --format=avr $@

clean:
	rm -f $(TARGET).elf $(TARGET).hex

.PHONY: all clean
