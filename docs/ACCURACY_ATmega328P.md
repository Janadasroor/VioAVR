# Hardware Accuracy Audit: ATmega328P

## 1. Memory Configuration

| Segment | Start | Size | Type | Status |
| --- | --- | --- | --- | --- |
| FLASH | 0x0 | 0x8000 | flash | [ ] |
| BOOT_SECTION_1 | 0x7e00 | 0x200 | flash | [ ] |
| BOOT_SECTION_2 | 0x7c00 | 0x400 | flash | [ ] |
| BOOT_SECTION_3 | 0x7800 | 0x800 | flash | [ ] |
| BOOT_SECTION_4 | 0x7000 | 0x1000 | flash | [ ] |
| SIGNATURES | 0x0 | 0x3 | signatures | [ ] |
| FUSES | 0x0 | 0x3 | fuses | [ ] |
| LOCKBITS | 0x0 | 0x1 | lockbits | [ ] |
| REGISTERS | 0x0 | 0x20 | regs | [ ] |
| MAPPED_IO | 0x20 | 0xe0 | io | [ ] |
| IRAM | 0x100 | 0x800 | ram | [ ] |
| EEPROM | 0x0 | 0x400 | eeprom | [ ] |
| OSCCAL | 0x0 | 0x1 | osccal | [ ] |

## 2. Interrupt Vector Table

| Index | Name | Caption | Status |
| --- | --- | --- | --- |
| 0 | RESET | External Pin, Power-on Reset, Brown-out Reset and Watchdog Reset | [ ] |
| 1 | INT0 | External Interrupt Request 0 | [ ] |
| 2 | INT1 | External Interrupt Request 1 | [ ] |
| 3 | PCINT0 | Pin Change Interrupt Request 0 | [ ] |
| 4 | PCINT1 | Pin Change Interrupt Request 1 | [ ] |
| 5 | PCINT2 | Pin Change Interrupt Request 2 | [ ] |
| 6 | WDT | Watchdog Time-out Interrupt | [ ] |
| 7 | TIMER2_COMPA | Timer/Counter2 Compare Match A | [ ] |
| 8 | TIMER2_COMPB | Timer/Counter2 Compare Match B | [ ] |
| 9 | TIMER2_OVF | Timer/Counter2 Overflow | [ ] |
| 10 | TIMER1_CAPT | Timer/Counter1 Capture Event | [ ] |
| 11 | TIMER1_COMPA | Timer/Counter1 Compare Match A | [ ] |
| 12 | TIMER1_COMPB | Timer/Counter1 Compare Match B | [ ] |
| 13 | TIMER1_OVF | Timer/Counter1 Overflow | [ ] |
| 14 | TIMER0_COMPA | TimerCounter0 Compare Match A | [ ] |
| 15 | TIMER0_COMPB | TimerCounter0 Compare Match B | [ ] |
| 16 | TIMER0_OVF | Timer/Counter0 Overflow | [ ] |
| 17 | SPI_STC | SPI Serial Transfer Complete | [ ] |
| 18 | USART_RX | USART Rx Complete | [ ] |
| 19 | USART_UDRE | USART, Data Register Empty | [ ] |
| 20 | USART_TX | USART Tx Complete | [ ] |
| 21 | ADC | ADC Conversion Complete | [ ] |
| 22 | EE_READY | EEPROM Ready | [ ] |
| 23 | ANALOG_COMP | Analog Comparator | [ ] |
| 24 | TWI | Two-wire Serial Interface | [ ] |
| 25 | SPM_Ready | Store Program Memory Read | [ ] |

## 3. Peripheral Register Map

### USART0 (USART)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| UDR0 | 0xc6 | 0x0 | 0xff |  | [ ] |
| UCSR0A | 0xc0 | 0x0 | 0xff | RXC0, TXC0, UDRE0, FE0, DOR0, UPE0, U2X0, MPCM0 | [ ] |
| UCSR0B | 0xc1 | 0x0 | 0xff | RXCIE0, TXCIE0, UDRIE0, RXEN0, TXEN0, UCSZ02, RXB80, TXB80 | [ ] |
| UCSR0C | 0xc2 | 0x0 | 0xff | UMSEL0, UPM0, USBS0, UCSZ0, UCPOL0 | [ ] |
| UBRR0 | 0xc4 | 0x0 | 0xfff |  | [ ] |

### TWI (TWI)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| TWAMR | 0xbd | 0x0 | 0xff | TWAM | [ ] |
| TWBR | 0xb8 | 0x0 | 0xff |  | [ ] |
| TWCR | 0xbc | 0x0 | 0xff | TWINT, TWEA, TWSTA, TWSTO, TWWC, TWEN, TWIE | [ ] |
| TWSR | 0xb9 | 0x0 | 0xff | TWS, TWPS | [ ] |
| TWDR | 0xbb | 0x0 | 0xff |  | [ ] |
| TWAR | 0xba | 0x0 | 0xff | TWA, TWGCE | [ ] |

### TC1 (TC16)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| TIMSK1 | 0x6f | 0x0 | 0xff | ICIE1, OCIE1B, OCIE1A, TOIE1 | [ ] |
| TIFR1 | 0x36 | 0x0 | 0xff | ICF1, OCF1B, OCF1A, TOV1 | [ ] |
| TCCR1A | 0x80 | 0x0 | 0xff | COM1A, COM1B, WGM1 | [ ] |
| TCCR1B | 0x81 | 0x0 | 0xff | ICNC1, ICES1, WGM1, CS1 | [ ] |
| TCCR1C | 0x82 | 0x0 | 0xff | FOC1A, FOC1B | [ ] |
| TCNT1 | 0x84 | 0x0 | 0xffff |  | [ ] |
| OCR1A | 0x88 | 0x0 | 0xffff |  | [ ] |
| OCR1B | 0x8a | 0x0 | 0xffff |  | [ ] |
| ICR1 | 0x86 | 0x0 | 0xffff |  | [ ] |
| GTCCR | 0x43 | 0x0 | 0xff | TSM, PSRSYNC | [ ] |

### TC2 (TC8_ASYNC)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| TIMSK2 | 0x70 | 0x0 | 0xff | OCIE2B, OCIE2A, TOIE2 | [ ] |
| TIFR2 | 0x37 | 0x0 | 0xff | OCF2B, OCF2A, TOV2 | [ ] |
| TCCR2A | 0xb0 | 0x0 | 0xff | COM2A, COM2B, WGM2 | [ ] |
| TCCR2B | 0xb1 | 0x0 | 0xff | FOC2A, FOC2B, WGM22, CS2 | [ ] |
| TCNT2 | 0xb2 | 0x0 | 0xff |  | [ ] |
| OCR2B | 0xb4 | 0x0 | 0xff |  | [ ] |
| OCR2A | 0xb3 | 0x0 | 0xff |  | [ ] |
| ASSR | 0xb6 | 0x0 | 0xff | EXCLK, AS2, TCN2UB, OCR2AUB, OCR2BUB, TCR2AUB, TCR2BUB | [ ] |
| GTCCR | 0x43 | 0x0 | 0xff | TSM, PSRASY | [ ] |

### ADC (ADC)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| ADMUX | 0x7c | 0x0 | 0xff | REFS, ADLAR, MUX | [ ] |
| ADC | 0x78 | 0x0 | 0xffff |  | [ ] |
| ADCSRA | 0x7a | 0x0 | 0xff | ADEN, ADSC, ADATE, ADIF, ADIE, ADPS | [ ] |
| ADCSRB | 0x7b | 0x0 | 0xff | ACME, ADTS | [ ] |
| DIDR0 | 0x7e | 0x0 | 0xff | ADC5D, ADC4D, ADC3D, ADC2D, ADC1D, ADC0D | [ ] |

### AC (AC)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| ACSR | 0x50 | 0x0 | 0xff | ACD, ACBG, ACO, ACI, ACIE, ACIC, ACIS | [ ] |
| DIDR1 | 0x7f | 0x0 | 0xff | AIN1D, AIN0D | [ ] |

### PORTB (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PORTB | 0x25 | 0x0 | 0xff |  | [ ] |
| DDRB | 0x24 | 0x0 | 0xff |  | [ ] |
| PINB | 0x23 | 0x0 | 0xff |  | [ ] |

### PORTC (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PORTC | 0x28 | 0x0 | 0x7f |  | [ ] |
| DDRC | 0x27 | 0x0 | 0x7f |  | [ ] |
| PINC | 0x26 | 0x0 | 0x7f |  | [ ] |

### PORTD (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PORTD | 0x2b | 0x0 | 0xff |  | [ ] |
| DDRD | 0x2a | 0x0 | 0xff |  | [ ] |
| PIND | 0x29 | 0x0 | 0xff |  | [ ] |

### TC0 (TC8)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| OCR0B | 0x48 | 0x0 | 0xff |  | [ ] |
| OCR0A | 0x47 | 0x0 | 0xff |  | [ ] |
| TCNT0 | 0x46 | 0x0 | 0xff |  | [ ] |
| TCCR0B | 0x45 | 0x0 | 0xff | FOC0A, FOC0B, WGM02, CS0 | [ ] |
| TCCR0A | 0x44 | 0x0 | 0xff | COM0A, COM0B, WGM0 | [ ] |
| TIMSK0 | 0x6e | 0x0 | 0xff | OCIE0B, OCIE0A, TOIE0 | [ ] |
| TIFR0 | 0x35 | 0x0 | 0xff | OCF0B, OCF0A, TOV0 | [ ] |
| GTCCR | 0x43 | 0x0 | 0xff | TSM, PSRSYNC | [ ] |

### EXINT (EXINT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| EICRA | 0x69 | 0x0 | 0xff | ISC1, ISC0 | [ ] |
| EIMSK | 0x3d | 0x0 | 0xff | INT | [ ] |
| EIFR | 0x3c | 0x0 | 0xff | INTF | [ ] |
| PCICR | 0x68 | 0x0 | 0xff | PCIE | [ ] |
| PCMSK2 | 0x6d | 0x0 | 0xff | PCINT | [ ] |
| PCMSK1 | 0x6c | 0x0 | 0xff | PCINT | [ ] |
| PCMSK0 | 0x6b | 0x0 | 0xff | PCINT | [ ] |
| PCIFR | 0x3b | 0x0 | 0xff | PCIF | [ ] |

### SPI (SPI)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| SPDR | 0x4e | 0x0 | 0xff |  | [ ] |
| SPSR | 0x4d | 0x0 | 0xff | SPIF, WCOL, SPI2X | [ ] |
| SPCR | 0x4c | 0x0 | 0xff | SPIE, SPE, DORD, MSTR, CPOL, CPHA, SPR | [ ] |

### WDT (WDT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| WDTCSR | 0x60 | 0x0 | 0xff | WDIF, WDIE, WDP, WDCE, WDE | [ ] |

### EEPROM (EEPROM)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| EEAR | 0x41 | 0x0 | 0x3ff |  | [ ] |
| EEDR | 0x40 | 0x0 | 0xff |  | [ ] |
| EECR | 0x3f | 0x0 | 0xff | EEPM, EERIE, EEMPE, EEPE, EERE | [ ] |

### CPU (CPU)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PRR | 0x64 | 0x0 | 0xff | PRTWI, PRTIM2, PRTIM0, PRTIM1, PRSPI, PRUSART0, PRADC | [ ] |
| OSCCAL | 0x66 | 0x0 | 0xff | OSCCAL | [ ] |
| CLKPR | 0x61 | 0x0 | 0xff | CLKPCE, CLKPS | [ ] |
| SREG | 0x5f | 0x0 | 0xff | I, T, H, S, V, N, Z, C | [ ] |
| SP | 0x5d | 0x0 | 0xfff |  | [ ] |
| SPMCSR | 0x57 | 0x0 | 0xff | SPMIE, RWWSB, SIGRD, RWWSRE, BLBSET, PGWRT, PGERS, SPMEN | [ ] |
| MCUCR | 0x55 | 0x0 | 0xff | BODS, BODSE, PUD, IVSEL, IVCE | [ ] |
| MCUSR | 0x54 | 0x0 | 0xff | WDRF, BORF, EXTRF, PORF | [ ] |
| SMCR | 0x53 | 0x0 | 0xff | SM, SE | [ ] |
| GPIOR2 | 0x4b | 0x0 | 0xff |  | [ ] |
| GPIOR1 | 0x4a | 0x0 | 0xff |  | [ ] |
| GPIOR0 | 0x3e | 0x0 | 0xff |  | [ ] |

### FUSE (FUSE)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| EXTENDED | 0x2 | 0xff | 0xff | BODLEVEL | [ ] |
| HIGH | 0x1 | 0xd9 | 0xff | RSTDISBL, DWEN, SPIEN, WDTON, EESAVE, BOOTSZ, BOOTRST | [ ] |
| LOW | 0x0 | 0x62 | 0xff | CKDIV8, CKOUT, SUT_CKSEL | [ ] |

### LOCKBIT (LOCKBIT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| LOCKBIT | 0x0 | 0xff | 0xff | LB, BLB0, BLB1 | [ ] |

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
