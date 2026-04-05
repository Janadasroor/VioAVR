# ATmega328P Audit

This note records what has been checked for `ATmega328P` and what is still missing or only partially represented in VioAVR.

## Sources Used

- Local generated descriptor: `include/vioavr/core/devices/atmega328p.hpp`
- Local device schema: `include/vioavr/core/device.hpp`
- Official AVR header from the bundled pack: `atmega/include/avr/iom328p.h`
- ATDF from the bundled Microchip pack: `atmega/atdf/ATmega328P.atdf`
- Official Microchip datasheet: `https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf`

## Confirmed Correct In Current Descriptor

These items match the local ATDF/header and the datasheet spot-check:

- Device name: `ATmega328P`
- Flash size: `0x8000` bytes = `16384` words
- SRAM size: `0x0800` bytes = `2048` bytes
- EEPROM size: `0x0400` bytes = `1024` bytes
- Interrupt vector count: `26`
- Interrupt vector size: `2` bytes
- Default data-space layout:
  - register file `0x0000..0x001F`
  - I/O `0x0020..0x005F`
  - extended I/O `0x0060..0x00FF`
  - SRAM starts at `0x0100`
- Core register addresses:
  - `SPL = 0x5D`
  - `SPH = 0x5E`
  - `SREG = 0x5F`
- Port register addresses:
  - `PORTB`: `PINB=0x23`, `DDRB=0x24`, `PORTB=0x25`
  - `PORTC`: `PINC=0x26`, `DDRC=0x27`, `PORTC=0x28`
  - `PORTD`: `PIND=0x29`, `DDRD=0x2A`, `PORTD=0x2B`
- Timer0 register addresses in descriptor are consistent with the bundled sources
- Timer1 register addresses in descriptor are consistent with the bundled sources
- USART0 register addresses in descriptor are consistent with the bundled sources
- SPI register addresses in descriptor are consistent with the bundled sources
- TWI register addresses in descriptor are consistent with the bundled sources
- EEPROM register addresses in descriptor are consistent with the bundled sources
- WDT register address in descriptor is consistent with the bundled sources
- Interrupt indices currently stored in the descriptor for `INT0`, `Timer0`, `Timer1`, `SPI`, `USART0`, `ADC`, `EEPROM`, `WDT`, and `TWI` match the ATDF interrupt table

## Confirmed Present In ATDF/Header But Not Fully Represented In `DeviceDescriptor`

These are real `ATmega328P` features from the bundled pack, but the current descriptor schema does not model them fully or at all:

- `Timer2`
  - ATDF includes `TC2`
  - vector slots exist for `TIMER2_COMPA`, `TIMER2_COMPB`, and `TIMER2_OVF`
  - header exposes:
    - `TIFR2 = 0x17` with `TOV2`, `OCF2A`, `OCF2B`
    - `TIMSK2 = 0x70` with `TOIE2`, `OCIE2A`, `OCIE2B`
    - `TCCR2A = 0xB0`
    - `TCCR2B = 0xB1`
    - `TCNT2 = 0xB2`
    - `OCR2A = 0xB3`
    - `OCR2B = 0xB4`
    - `ASSR = 0xB6`
  - ATDF pin signals for `TC2`:
    - `OC2A -> PB3`
    - `OC2B -> PD3`
    - `TOSC1 -> PB6`
    - `TOSC2 -> PB7`
  - current `DeviceDescriptor` has `timer0` and `timer1`, but no `timer2`
- Pin-change interrupts beyond a single narrow slot
  - ATDF exposes `PCINT0`, `PCINT1`, `PCINT2`
  - current `DeviceDescriptor` only has `pin_change_interrupt_0`
- Analog comparator descriptor data
  - ATDF exposes comparator module and `ANALOG_COMP` interrupt
  - current `DeviceDescriptor` has no analog comparator descriptor
- Boot/fuse/lockbit metadata
  - ATDF exposes boot sections, fuse space, and lockbits space
  - current `DeviceDescriptor` has no structured fuse/boot/lockbit model
- Pin-function metadata
  - ATDF defines signal-to-pad mappings for UART, TWI, SPI, ADC, comparator, external interrupt, timer outputs, timer clock inputs, and oscillator pins
  - current `DeviceDescriptor` only stores simple port register triplets

## Confirmed Repo-Level Gaps

These are codebase issues, not hardware-data disagreements:

- CLI composition still hard-codes some `ATmega328P` GPIO register addresses in `apps/vioavr-cli/main.cpp`
- There is no canonical machine factory shared by CLI/tests/library construction
- `PinMap` is external-ID mapping only; it is not a chip pin-mux model
- Several peripherals exist in source, but their descriptor coverage is incomplete relative to the ATDF surface
- There is no `Timer2` implementation or test surface in the current source tree search
- Current interrupt descriptor coverage is selective: `interrupt_vector_count = 26` is correct, but several real vectors do not have descriptor-backed peripheral coverage yet

## Timer2 And Vector Coverage Conclusion

`ATmega328P` vector numbering itself is not the problem. The ATDF vector table lines up with the current descriptor values for the peripherals that are already modeled.

The concrete gap is coverage:

- vectors `7`, `8`, and `9` belong to `Timer2`
- the current runtime schema has no `timer2` descriptor slot
- therefore a real `ATmega328P` vector table exists in metadata, but the simulator cannot represent the full timer-driven interrupt surface for this chip yet

That means `ATmega328P` support is currently accurate in parts, but incomplete as a full-chip target.

## Needs Datasheet-Only Confirmation

These were not treated as confirmed in this audit pass:

- register reset values
- sleep-mode exact behavior and wake timing
- timer waveform edge cases and buffering rules
- fuse semantics
- boot reset vector behavior details
- lock-bit behavior
- exact peripheral timing behavior for UART, SPI, TWI, ADC, EEPROM, and WDT

## Practical Conclusion

The current `ATmega328P` descriptor is not obviously wrong in its core addresses and vector indices. The larger issue is scope coverage:

- basic memory map and many key register addresses are correct
- several important `ATmega328P` subsystems are still absent from the descriptor model
- full-chip fidelity now depends more on behavioral completion and descriptor expansion than on fixing glaring address mistakes
