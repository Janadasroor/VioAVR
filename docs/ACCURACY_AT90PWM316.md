# Hardware Accuracy Audit: AT90PWM316

## 1. Memory Configuration

| Segment | Start | Size | Type | Status |
| --- | --- | --- | --- | --- |
| FLASH | 0x0 | 0x4000 | flash | [ ] |
| BOOT_SECTION_1 | 0x3e00 | 0x200 | flash | [ ] |
| BOOT_SECTION_2 | 0x3c00 | 0x400 | flash | [ ] |
| BOOT_SECTION_3 | 0x3800 | 0x800 | flash | [ ] |
| BOOT_SECTION_4 | 0x3000 | 0x1000 | flash | [ ] |
| SIGNATURES | 0x0 | 0x3 | signatures | [ ] |
| FUSES | 0x0 | 0x3 | fuses | [ ] |
| LOCKBITS | 0x0 | 0x1 | lockbits | [ ] |
| REGISTERS | 0x0 | 0x20 | regs | [ ] |
| MAPPED_IO | 0x20 | 0xe0 | io | [ ] |
| IRAM | 0x100 | 0x400 | ram | [ ] |
| EEPROM | 0x0 | 0x200 | eeprom | [ ] |
| OSCCAL | 0x0 | 0x1 | osccal | [ ] |

## 2. Interrupt Vector Table

| Index | Name | Caption | Status |
| --- | --- | --- | --- |
| 0 | RESET | External Pin, Power-on Reset, Brown-out Reset, Watchdog Reset and JTAG AVR Reset | [ ] |
| 1 | PSC2_CAPT | PSC2 Capture Event | [ ] |
| 2 | PSC2_EC | PSC2 End Cycle | [ ] |
| 3 | PSC1_CAPT | PSC1 Capture Event | [ ] |
| 4 | PSC1_EC | PSC1 End Cycle | [ ] |
| 5 | PSC0_CAPT | PSC0 Capture Event | [ ] |
| 6 | PSC0_EC | PSC0 End Cycle | [ ] |
| 7 | ANALOG_COMP_0 | Analog Comparator 0 | [ ] |
| 8 | ANALOG_COMP_1 | Analog Comparator 1 | [ ] |
| 9 | ANALOG_COMP_2 | Analog Comparator 2 | [ ] |
| 10 | INT0 | External Interrupt Request 0 | [ ] |
| 11 | TIMER1_CAPT | Timer/Counter1 Capture Event | [ ] |
| 12 | TIMER1_COMPA | Timer/Counter1 Compare Match A | [ ] |
| 13 | TIMER1_COMPB | Timer/Counter Compare Match B | [ ] |
| 14 | RESERVED15 | None | [ ] |
| 15 | TIMER1_OVF | Timer/Counter1 Overflow | [ ] |
| 16 | TIMER0_COMP_A | Timer/Counter0 Compare Match A | [ ] |
| 17 | TIMER0_OVF | Timer/Counter0 Overflow | [ ] |
| 18 | ADC | ADC Conversion Complete | [ ] |
| 19 | INT1 | External Interrupt Request 1 | [ ] |
| 20 | SPI_STC | SPI Serial Transfer Complete | [ ] |
| 21 | USART_RX | USART, Rx Complete | [ ] |
| 22 | USART_UDRE | USART Data Register Empty | [ ] |
| 23 | USART_TX | USART, Tx Complete | [ ] |
| 24 | INT2 | External Interrupt Request 2 | [ ] |
| 25 | WDT | Watchdog Timeout Interrupt | [ ] |
| 26 | EE_READY | EEPROM Ready | [ ] |
| 27 | TIMER0_COMPB | Timer Counter 0 Compare Match B | [ ] |
| 28 | INT3 | External Interrupt Request 3 | [ ] |
| 29 | RESERVED30 | None | [ ] |
| 30 | RESERVED31 | None | [ ] |
| 31 | SPM_READY | Store Program Memory Read | [ ] |

## 3. Peripheral Register Map

### PORTB (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PORTB | 0x25 | 0x0 | 0xff |  | [ ] |
| DDRB | 0x24 | 0x0 | 0xff |  | [ ] |
| PINB | 0x23 | 0x0 | 0xff |  | [ ] |

### PORTC (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PORTC | 0x28 | 0x0 | 0xff |  | [ ] |
| DDRC | 0x27 | 0x0 | 0xff |  | [ ] |
| PINC | 0x26 | 0x0 | 0xff |  | [ ] |

### PORTD (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PORTD | 0x2b | 0x0 | 0xff |  | [ ] |
| DDRD | 0x2a | 0x0 | 0xff |  | [ ] |
| PIND | 0x29 | 0x0 | 0xff |  | [ ] |

### PORTE (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PORTE | 0x2e | 0x0 | 0x7 |  | [ ] |
| DDRE | 0x2d | 0x0 | 0x7 |  | [ ] |
| PINE | 0x2c | 0x0 | 0x7 |  | [ ] |

### BOOT_LOAD (BOOT_LOAD)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| SPMCSR | 0x57 | 0x0 | 0xff | SPMIE, RWWSB, RWWSRE, BLBSET, PGWRT, PGERS, SPMEN | [ ] |

### EUSART (EUSART)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| EUDR | 0xce | 0x0 | 0xff |  | [ ] |
| EUCSRA | 0xc8 | 0x0 | 0xff | UTxS, URxS | [ ] |
| EUCSRB | 0xc9 | 0x0 | 0xff | EUSART, EUSBS, EMCH, BODR | [ ] |
| EUCSRC | 0xca | 0x0 | 0xff | FEM, F1617, STP | [ ] |
| MUBRRH | 0xcd | 0x0 | 0xff | MUBRR | [ ] |
| MUBRRL | 0xcc | 0x0 | 0xff | MUBRR | [ ] |

### AC (AC)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| AC0CON | 0xad | 0x0 | 0xff | AC0EN, AC0IE, AC0IS, AC0M | [ ] |
| AC1CON | 0xae | 0x0 | 0xff | AC1EN, AC1IE, AC1IS, AC1ICE, AC1M | [ ] |
| AC2CON | 0xaf | 0x0 | 0xff | AC2EN, AC2IE, AC2IS, AC2M | [ ] |
| ACSR | 0x50 | 0x0 | 0xff | ACCKDIV, AC2IF, AC1IF, AC0IF, AC2O, AC1O, AC0O | [ ] |

### DAC (DAC)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| DAC | 0xab | 0x0 | 0xff | DAC | [ ] |
| DACON | 0xaa | 0x0 | 0xff | DAATE, DATS, DALA, DAOE, DAEN | [ ] |

### CPU (CPU)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| SREG | 0x5f | 0x0 | 0xff | I, T, H, S, V, N, Z, C | [ ] |
| SP | 0x5d | 0x0 | 0xffff |  | [ ] |
| MCUCR | 0x55 | 0x0 | 0xff | SPIPS, PUD, IVSEL, IVCE | [ ] |
| MCUSR | 0x54 | 0x0 | 0xff | WDRF, BORF, EXTRF, PORF | [ ] |
| OSCCAL | 0x66 | 0x0 | 0x7f | OSCCAL | [ ] |
| CLKPR | 0x61 | 0x0 | 0xff | CLKPCE, CLKPS | [ ] |
| SMCR | 0x53 | 0x0 | 0xff | SM, SE | [ ] |
| GPIOR3 | 0x3b | 0x0 | 0xff | GPIOR | [ ] |
| GPIOR2 | 0x3a | 0x0 | 0xff | GPIOR | [ ] |
| GPIOR1 | 0x39 | 0x0 | 0xff | GPIOR | [ ] |
| GPIOR0 | 0x3e | 0x0 | 0xff | GPIOR07, GPIOR06, GPIOR05, GPIOR04, GPIOR03, GPIOR02, GPIOR01, GPIOR00 | [ ] |
| PLLCSR | 0x49 | 0x0 | 0xff | PLLF, PLLE, PLOCK | [ ] |
| PRR | 0x64 | 0x0 | 0xff | PRPSC2, PRPSC1, PRPSC0, PRTIM1, PRTIM0, PRSPI, PRUSART0, PRADC | [ ] |

### TC0 (TC8)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| TIMSK0 | 0x6e | 0x0 | 0xff | OCIE0B, OCIE0A, TOIE0 | [ ] |
| TIFR0 | 0x35 | 0x0 | 0xff | OCF0B, OCF0A, TOV0 | [ ] |
| TCCR0A | 0x44 | 0x0 | 0xff | COM0A, COM0B, WGM0 | [ ] |
| TCCR0B | 0x45 | 0x0 | 0xff | FOC0A, FOC0B, WGM02, CS0 | [ ] |
| TCNT0 | 0x46 | 0x0 | 0xff | TCNT0 | [ ] |
| OCR0A | 0x47 | 0x0 | 0xff | OCR0A | [ ] |
| OCR0B | 0x48 | 0x0 | 0xff | OCR0B | [ ] |
| GTCCR | 0x43 | 0x0 | 0xff | TSM, ICPSEL1, PSR10 | [ ] |

### TC1 (TC16)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| TIMSK1 | 0x6f | 0x0 | 0xff | ICIE1, OCIE1B, OCIE1A, TOIE1 | [ ] |
| TIFR1 | 0x36 | 0x0 | 0xff | ICF1, OCF1B, OCF1A, TOV1 | [ ] |
| TCCR1A | 0x80 | 0x0 | 0xff | COM1A, COM1B, WGM1 | [ ] |
| TCCR1B | 0x81 | 0x0 | 0xff | ICNC1, ICES1, WGM1, CS1 | [ ] |
| TCCR1C | 0x82 | 0x0 | 0xff | FOC1A, FOC1B | [ ] |
| TCNT1 | 0x84 | 0x0 | 0xffff | TCNT1 | [ ] |
| OCR1A | 0x88 | 0x0 | 0xffff | OCR1A | [ ] |
| OCR1B | 0x8a | 0x0 | 0xffff | OCR1B | [ ] |
| ICR1 | 0x86 | 0x0 | 0xffff |  | [ ] |
| GTCCR | 0x43 | 0x0 | 0xff | TSM, PSRSYNC | [ ] |

### ADC (ADC)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| ADMUX | 0x7c | 0x0 | 0xff | REFS, ADLAR, MUX | [ ] |
| ADCSRA | 0x7a | 0x0 | 0xff | ADEN, ADSC, ADATE, ADIF, ADIE, ADPS | [ ] |
| ADC | 0x78 | 0x0 | 0xffff |  | [ ] |
| ADCSRB | 0x7b | 0x0 | 0x8f | ADHSM, ADTS3, ADTS2, ADTS1, ADTS0 | [ ] |
| DIDR0 | 0x7e | 0x0 | 0xff | ADC7D, ADC6D, ADC5D, ADC4D, ADC3D, ADC2D, ADC1D, ADC0D | [ ] |
| DIDR1 | 0x7f | 0x0 | 0xff | ACMP0D, AMP0PD, AMP0ND, ADC10D, ADC9D, ADC8D | [ ] |
| AMP0CSR | 0x76 | 0x0 | 0xff | AMP0EN, AMP0IS, AMP0G, AMP0TS | [ ] |
| AMP1CSR | 0x77 | 0x0 | 0xff | AMP1EN, AMP1IS, AMP1G, AMP1TS | [ ] |

### USART (USART)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| UDR | 0xc6 | 0x0 | 0xff |  | [ ] |
| UCSRA | 0xc0 | 0x0 | 0xff | RXC, TXC, UDRE, FE, DOR, UPE, U2X, MPCM | [ ] |
| UCSRB | 0xc1 | 0x0 | 0xff | RXCIE, TXCIE, UDRIE, RXEN, TXEN, UCSZ2, RXB8, TXB8 | [ ] |
| UCSRC | 0xc2 | 0x0 | 0xff | UMSEL0, UPM, USBS, UCSZ, UCPOL | [ ] |
| UBRRH | 0xc5 | 0x0 | 0xff | UBRR | [ ] |
| UBRRL | 0xc4 | 0x0 | 0xff | UBRR | [ ] |

### SPI (SPI)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| SPCR | 0x4c | 0x0 | 0xff | SPIE, SPE, DORD, MSTR, CPOL, CPHA, SPR | [ ] |
| SPSR | 0x4d | 0x0 | 0xff | SPIF, WCOL, SPI2X | [ ] |
| SPDR | 0x4e | 0x0 | 0xff | SPD | [ ] |

### WDT (WDT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| WDTCSR | 0x60 | 0x0 | 0xff | WDIF, WDIE, WDP, WDCE, WDE | [ ] |

### EXINT (EXINT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| EICRA | 0x69 | 0x0 | 0xff | ISC3, ISC2, ISC1, ISC0 | [ ] |
| EIMSK | 0x3d | 0x0 | 0xff | INT | [ ] |
| EIFR | 0x3c | 0x0 | 0xff | INTF | [ ] |

### EEPROM (EEPROM)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| EEAR | 0x41 | 0x0 | 0xfff | EEAR | [ ] |
| EEDR | 0x40 | 0x0 | 0xff | EEDR | [ ] |
| EECR | 0x3f | 0x0 | 0xff | EERIE, EEMWE, EEWE, EERE | [ ] |

### PSC0 (PSC)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PICR0 | 0xde | 0x0 | 0x8fff | PCST0, PICR0 | [ ] |
| PFRC0B | 0xdd | 0x0 | 0xff | PCAE0B, PISEL0B, PELEV0B, PFLTE0B, PRFM0B | [ ] |
| PFRC0A | 0xdc | 0x0 | 0xff | PCAE0A, PISEL0A, PELEV0A, PFLTE0A, PRFM0A | [ ] |
| PCTL0 | 0xdb | 0x0 | 0xff | PPRE0, PBFM0, PAOC0B, PAOC0A, PARUN0, PCCYC0, PRUN0 | [ ] |
| PCNF0 | 0xda | 0x0 | 0xff | PFIFTY0, PALOCK0, PLOCK0, PMODE0, POP0, PCLKSEL0 | [ ] |
| OCR0RB | 0xd8 | 0x0 | 0xffff |  | [ ] |
| OCR0SB | 0xd6 | 0x0 | 0xfff |  | [ ] |
| OCR0RA | 0xd4 | 0x0 | 0xfff |  | [ ] |
| OCR0SA | 0xd2 | 0x0 | 0xfff |  | [ ] |
| PSOC0 | 0xd0 | 0x0 | 0xff | PSYNC0, POEN0B, POEN0A | [ ] |
| PIM0 | 0xa1 | 0x0 | 0xff | PSEIE0, PEVE0B, PEVE0A, PEOPE0 | [ ] |
| PIFR0 | 0xa0 | 0x0 | 0xff | POAC0B, POAC0A, PSEI0, PEV0B, PEV0A, PRN0, PEOP0 | [ ] |

### PSC1 (PSC)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PICR1 | 0xee | 0x0 | 0x8fff | PCST1, PICR1 | [ ] |
| PFRC1B | 0xed | 0x0 | 0xff | PCAE1B, PISEL1B, PELEV1B, PFLTE1B, PRFM1B | [ ] |
| PFRC1A | 0xec | 0x0 | 0xff | PCAE1A, PISEL1A, PELEV1A, PFLTE1A, PRFM1A | [ ] |
| PCTL1 | 0xeb | 0x0 | 0xff | PPRE1, PBFM1, PAOC1B, PAOC1A, PARUN1, PCCYC1, PRUN1 | [ ] |
| PCNF1 | 0xea | 0x0 | 0xff | PFIFTY1, PALOCK1, PLOCK1, PMODE1, POP1, PCLKSEL1 | [ ] |
| OCR1RB | 0xe8 | 0x0 | 0xffff |  | [ ] |
| OCR1SB | 0xe6 | 0x0 | 0xfff |  | [ ] |
| OCR1RA | 0xe4 | 0x0 | 0xfff |  | [ ] |
| OCR1SA | 0xe2 | 0x0 | 0xfff |  | [ ] |
| PSOC1 | 0xe0 | 0x0 | 0xff | PSYNC1_, POEN1B, POEN1A | [ ] |
| PIM1 | 0xa3 | 0x0 | 0xff | PSEIE1, PEVE1B, PEVE1A, PEOPE1 | [ ] |
| PIFR1 | 0xa2 | 0x0 | 0xff | POAC1B, POAC1A, PSEI1, PEV1B, PEV1A, PRN1, PEOP1 | [ ] |

### PSC2 (PSC)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PICR2 | 0xfe | 0x0 | 0x8fff | PCST2, PICR2 | [ ] |
| PFRC2B | 0xfd | 0x0 | 0xff | PCAE2B, PISEL2B, PELEV2B, PFLTE2B, PRFM2B | [ ] |
| PFRC2A | 0xfc | 0x0 | 0xff | PCAE2A, PISEL2A, PELEV2A, PFLTE2A, PRFM2A | [ ] |
| PCTL2 | 0xfb | 0x0 | 0xff | PPRE2, PBFM2, PAOC2B, PAOC2A, PARUN2, PCCYC2, PRUN2 | [ ] |
| PCNF2 | 0xfa | 0x0 | 0xff | PFIFTY2, PALOCK2, PLOCK2, PMODE2, POP2, PCLKSEL2, POME2 | [ ] |
| OCR2RB | 0xf8 | 0x0 | 0xffff |  | [ ] |
| OCR2SB | 0xf6 | 0x0 | 0xfff |  | [ ] |
| OCR2RA | 0xf4 | 0x0 | 0xfff |  | [ ] |
| OCR2SA | 0xf2 | 0x0 | 0xfff |  | [ ] |
| POM2 | 0xf1 | 0x0 | 0xff | POMV2B, POMV2A | [ ] |
| PSOC2 | 0xf0 | 0x0 | 0xff | POS2, PSYNC2_, POEN2D, POEN2B, POEN2C, POEN2A | [ ] |
| PIM2 | 0xa5 | 0x0 | 0xff | PSEIE2, PEVE2B, PEVE2A, PEOPE2 | [ ] |
| PIFR2 | 0xa4 | 0x0 | 0xff | POAC2B, POAC2A, PSEI2, PEV2B, PEV2A, PRN2, PEOP2 | [ ] |

### FUSE (FUSE)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| EXTENDED | 0x2 | 0xf9 | 0xff | PSC2RB, PSC1RB, PSC0RB, PSCRV, BOOTSZ, BOOTRST | [ ] |
| HIGH | 0x1 | 0xdf | 0xff | RSTDISBL, DWEN, SPIEN, WDTON, EESAVE, BODLEVEL | [ ] |
| LOW | 0x0 | 0x62 | 0xff | CKDIV8, CKOUT, SUT_CKSEL | [ ] |

### LOCKBIT (LOCKBIT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| LOCKBIT | 0x0 | 0xff | 0xff | LB, BLB0, BLB1 | [ ] |

