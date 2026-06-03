AVR_CC  = avr-gcc
AVR_OBJ = avr-objcopy
CFLAGS  = -Os -DF_CPU=16000000UL -mmcu=atmega4809

all: nack_test.hex

nack_test.elf: nack_test.c
	$(AVR_CC) $(CFLAGS) -o $@ $^

%.hex: %.elf
	$(AVR_OBJ) -O ihex -R .eeprom $< $@

clean:
	rm -f *.elf *.hex
