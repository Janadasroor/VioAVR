# Makefile for AVR test programs
# Usage: make -j8

MCU      = atmega328p
CC       = avr-gcc
OBJCOPY  = avr-objcopy
OBJDUMP  = avr-objdump

CFLAGS   = -mmcu=$(MCU) -Os -std=c99 -Wall -Wextra
CFLAGS_BARE = -mmcu=$(MCU) -nostartfiles -Os -std=c99 -Wall -Wextra

# Programs compiled without C runtime (bare metal)
BARE_PROGS = smoke

# All programs
ALL_PROGS = $(patsubst %.c,%,$(wildcard *.c))

# Output files
ELFS = $(ALL_PROGS:=.elf)
BINS = $(ALL_PROGS:=.bin)
HEXS = $(ALL_PROGS:=.hex)
LSTS = $(ALL_PROGS:=.lst)

.PHONY: all clean hex

all: $(ELFS) $(LSTS)

hex: $(HEXS)

# Compile: bare metal programs (no CRT)
$(BARE_PROGS:=.elf): %.elf: %.c
	@echo "  CC(b)   $<"
	@$(CC) $(CFLAGS_BARE) -o $@ $<

# Compile: normal programs (with CRT)
$(filter-out $(BARE_PROGS:=.elf),$(ELFS)): %.elf: %.c
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) -o $@ $<

# Generate HEX
%.hex: %.elf
	@echo "  HEX     $@"
	@$(OBJCOPY) -O ihex $< $@

# Generate BIN
%.bin: %.elf
	@echo "  BIN     $@"
	@$(OBJCOPY) -O binary $< $@

# Generate listing
%.lst: %.elf
	@echo "  LST     $@"
	@$(OBJDUMP) -d $< > $@

clean:
	@echo "  CLEAN"
	@rm -f *.elf *.hex *.bin *.lst *.o
