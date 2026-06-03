AVR_CC  = avr-gcc
AVR_OBJ = avr-objcopy
CFLAGS  = -Os -DF_CPU=16000000UL -mmcu=

all: master.hex slave.hex

master.elf: master.c
	$(AVR_CC) $(CFLAGS)atmega4809 -o $@ $^

slave.elf: slave.c
	$(AVR_CC) $(CFLAGS)atmega328p -o $@ $^

%.hex: %.elf
	$(AVR_OBJ) -O ihex -R .eeprom $< $@

clean:
	rm -f *.elf *.hex
