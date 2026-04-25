# Hardware Accuracy Audit: ATmega169PA

## 1. Memory Configuration

| Segment | Start | Size | Type | Status |
| --- | --- | --- | --- | --- |
| FLASH | 0x0 | 0x4000 | flash | [ ] |
| BOOT_SECTION_1 | 0x3f00 | 0x100 | flash | [ ] |
| BOOT_SECTION_2 | 0x3e00 | 0x200 | flash | [ ] |
| BOOT_SECTION_3 | 0x3c00 | 0x400 | flash | [ ] |
| BOOT_SECTION_4 | 0x3800 | 0x800 | flash | [ ] |
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
| 0 | RESET | External Pin,Power-on Reset,Brown-out Reset,Watchdog Reset,and JTAG AVR Reset. See Datasheet.      | [ ] |
| 1 | INT0 | External Interrupt Request 0 | [ ] |
| 2 | PCINT0 | Pin Change Interrupt Request 0 | [ ] |
| 3 | PCINT1 | Pin Change Interrupt Request 1 | [ ] |
| 4 | TIMER2_COMP | Timer/Counter2 Compare Match | [ ] |
| 5 | TIMER2_OVF | Timer/Counter2 Overflow | [ ] |
| 6 | TIMER1_CAPT | Timer/Counter1 Capture Event | [ ] |
| 7 | TIMER1_COMPA | Timer/Counter1 Compare Match A | [ ] |
| 8 | TIMER1_COMPB | Timer/Counter Compare Match B | [ ] |
| 9 | TIMER1_OVF | Timer/Counter1 Overflow | [ ] |
| 10 | TIMER0_COMP | Timer/Counter0 Compare Match | [ ] |
| 11 | TIMER0_OVF | Timer/Counter0 Overflow | [ ] |
| 12 | SPI_STC | SPI Serial Transfer Complete | [ ] |
| 13 | USART0_RX | USART0, Rx Complete | [ ] |
| 14 | USART0_UDRE | USART0 Data register Empty | [ ] |
| 15 | USART0_TX | USART0, Tx Complete | [ ] |
| 16 | USI_START | USI Start Condition | [ ] |
| 17 | USI_OVERFLOW | USI Overflow | [ ] |
| 18 | ANALOG_COMP | Analog Comparator | [ ] |
| 19 | ADC | ADC Conversion Complete | [ ] |
| 20 | EE_READY | EEPROM Ready | [ ] |
| 21 | SPM_READY | Store Program Memory Read | [ ] |
| 22 | LCD | LCD Start of Frame | [ ] |

## 3. Peripheral Register Map

### TC0 (TC8)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| TCCR0A | 0x44 | 0x0 | 0xff | FOC0A, WGM00, COM0A, WGM01, CS0 | [ ] |
| TCNT0 | 0x46 | 0x0 | 0xff | TCNT0 | [ ] |
| OCR0A | 0x47 | 0x0 | 0xff | OCR0A | [ ] |
| TIMSK0 | 0x6e | 0x0 | 0xff | OCIE0A, TOIE0 | [ ] |
| TIFR0 | 0x35 | 0x0 | 0xff | OCF0A, TOV0 | [ ] |
| GTCCR | 0x43 | 0x0 | 0xff | TSM, PSR10 | [ ] |

### TC1 (TC16)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| TCCR1A | 0x80 | 0x0 | 0xff | COM1A, COM1B, WGM1 | [ ] |
| TCCR1B | 0x81 | 0x0 | 0xff | ICNC1, ICES1, WGM1, CS1 | [ ] |
| TCCR1C | 0x82 | 0x0 | 0xff | FOC1A, FOC1B | [ ] |
| TCNT1 | 0x84 | 0x0 | 0xffff | TCNT1 | [ ] |
| OCR1A | 0x88 | 0x0 | 0xffff | OCR1A | [ ] |
| OCR1B | 0x8a | 0x0 | 0xffff | OCR1B | [ ] |
| ICR1 | 0x86 | 0x0 | 0xffff | ICR1 | [ ] |
| TIMSK1 | 0x6f | 0x0 | 0xff | ICIE1, OCIE1B, OCIE1A, TOIE1 | [ ] |
| TIFR1 | 0x36 | 0x0 | 0xff | ICF1, OCF1B, OCF1A, TOV1 | [ ] |

### TC2 (TC8_ASYNC)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| TCCR2A | 0xb0 | 0x0 | 0xff | FOC2A, WGM20, COM2A, WGM21, CS2 | [ ] |
| TCNT2 | 0xb2 | 0x0 | 0xff | TCNT2 | [ ] |
| OCR2A | 0xb3 | 0x0 | 0xff | OCR2A | [ ] |
| TIMSK2 | 0x70 | 0x0 | 0xff | OCIE2A, TOIE2 | [ ] |
| TIFR2 | 0x37 | 0x0 | 0xff | OCF2A, TOV2 | [ ] |
| GTCCR | 0x43 | 0x0 | 0xff | PSR2 | [ ] |
| ASSR | 0xb6 | 0x0 | 0xff | EXCLK, AS2, TCN2UB, OCR2UB, TCR2UB | [ ] |

### WDT (WDT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| WDTCR | 0x60 | 0x0 | 0xff | WDCE, WDE, WDP | [ ] |

### EEPROM (EEPROM)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| EEAR | 0x41 | 0x0 | 0x1ff | EEAR | [ ] |
| EEDR | 0x40 | 0x0 | 0xff | EEDR | [ ] |
| EECR | 0x3f | 0x0 | 0xff | EERIE, EEMWE, EEWE, EERE | [ ] |

### SPI (SPI)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| SPCR | 0x4c | 0x0 | 0xff | SPIE, SPE, DORD, MSTR, CPOL, CPHA, SPR | [ ] |
| SPSR | 0x4d | 0x0 | 0xff | SPIF, WCOL, SPI2X | [ ] |
| SPDR | 0x4e | 0x0 | 0xff | SPDR | [ ] |

### PORTA (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PORTA | 0x22 | 0x0 | 0xff |  | [ ] |
| DDRA | 0x21 | 0x0 | 0xff |  | [ ] |
| PINA | 0x20 | 0x0 | 0xff |  | [ ] |

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
| PORTE | 0x2e | 0x0 | 0xff |  | [ ] |
| DDRE | 0x2d | 0x0 | 0xff |  | [ ] |
| PINE | 0x2c | 0x0 | 0xff |  | [ ] |

### PORTF (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PORTF | 0x31 | 0x0 | 0xff |  | [ ] |
| DDRF | 0x30 | 0x0 | 0xff |  | [ ] |
| PINF | 0x2f | 0x0 | 0xff |  | [ ] |

### PORTG (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PORTG | 0x34 | 0x0 | 0x3f |  | [ ] |
| DDRG | 0x33 | 0x0 | 0x3f |  | [ ] |
| PING | 0x32 | 0x0 | 0x3f |  | [ ] |

### AC (AC)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| ADCSRB | 0x7b | 0x0 | 0xff | ACME | [ ] |
| ACSR | 0x50 | 0x0 | 0xff | ACD, ACBG, ACO, ACI, ACIE, ACIC, ACIS | [ ] |
| DIDR1 | 0x7f | 0x0 | 0xff | AIN1D, AIN0D | [ ] |

### JTAG (JTAG)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| OCDR | 0x51 | 0x0 | 0xff |  | [ ] |
| MCUCR | 0x55 | 0x0 | 0xff | JTD | [ ] |
| MCUSR | 0x54 | 0x0 | 0xff | JTRF | [ ] |

### LCD (LCD)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| LCDCRA | 0xe4 | 0x0 | 0xff | LCDEN, LCDAB, LCDIF, LCDIE, LCDBD, LCDCCD, LCDBL | [ ] |
| LCDCRB | 0xe5 | 0x0 | 0xff | LCDCS, LCD2B, LCDMUX, LCDPM | [ ] |
| LCDFRR | 0xe6 | 0x0 | 0xff | LCDPS, LCDCD | [ ] |
| LCDCCR | 0xe7 | 0x0 | 0xff | LCDDC, LCDMDT, LCDCC | [ ] |
| LCDDR18 | 0xfe | 0x0 | 0x1 |  | [ ] |
| LCDDR17 | 0xfd | 0x0 | 0xff |  | [ ] |
| LCDDR16 | 0xfc | 0x0 | 0xff |  | [ ] |
| LCDDR15 | 0xfb | 0x0 | 0xff |  | [ ] |
| LCDDR13 | 0xf9 | 0x0 | 0x1 |  | [ ] |
| LCDDR12 | 0xf8 | 0x0 | 0xff |  | [ ] |
| LCDDR11 | 0xf7 | 0x0 | 0xff |  | [ ] |
| LCDDR10 | 0xf6 | 0x0 | 0xff |  | [ ] |
| LCDDR8 | 0xf4 | 0x0 | 0x1 |  | [ ] |
| LCDDR7 | 0xf3 | 0x0 | 0xff |  | [ ] |
| LCDDR6 | 0xf2 | 0x0 | 0xff |  | [ ] |
| LCDDR5 | 0xf1 | 0x0 | 0xff |  | [ ] |
| LCDDR3 | 0xef | 0x0 | 0x1 |  | [ ] |
| LCDDR2 | 0xee | 0x0 | 0xff |  | [ ] |
| LCDDR1 | 0xed | 0x0 | 0xff |  | [ ] |
| LCDDR0 | 0xec | 0x0 | 0xff |  | [ ] |

### EXINT (EXINT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| EICRA | 0x69 | 0x0 | 0xff | ISC01, ISC00 | [ ] |
| EIMSK | 0x3d | 0x0 | 0xff | PCIE, INT0 | [ ] |
| EIFR | 0x3c | 0x0 | 0xff | PCIF, INTF0 | [ ] |
| PCMSK1 | 0x6c | 0x0 | 0xff | PCINT | [ ] |
| PCMSK0 | 0x6b | 0x0 | 0xff | PCINT | [ ] |

### CPU (CPU)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| SREG | 0x5f | 0x0 | 0xff | I, T, H, S, V, N, Z, C | [ ] |
| SP | 0x5d | 0x0 | 0x7ff |  | [ ] |
| MCUCR | 0x55 | 0x0 | 0xff | BODS, BODSE, PUD, IVSEL, IVCE | [ ] |
| MCUSR | 0x54 | 0x0 | 0xff | JTRF, WDRF, BORF, EXTRF, PORF | [ ] |
| OSCCAL | 0x66 | 0x0 | 0xff | OSCCAL | [ ] |
| CLKPR | 0x61 | 0x0 | 0xff | CLKPCE, CLKPS | [ ] |
| PRR | 0x64 | 0x0 | 0xff | PRLCD, PRTIM1, PRSPI, PRUSART0, PRADC | [ ] |
| SMCR | 0x53 | 0x0 | 0xff | SM, SE | [ ] |
| GPIOR2 | 0x4b | 0x0 | 0xff | GPIOR2 | [ ] |
| GPIOR1 | 0x4a | 0x0 | 0xff | GPIOR1 | [ ] |
| GPIOR0 | 0x3e | 0x0 | 0xff | GPIOR0 | [ ] |

### USI (USI)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| USIDR | 0xba | 0x0 | 0xff | USIDR | [ ] |
| USISR | 0xb9 | 0x0 | 0xff | USISIF, USIOIF, USIPF, USIDC, USICNT | [ ] |
| USICR | 0xb8 | 0x0 | 0xff | USISIE, USIOIE, USIWM, USICS, USICLK, USITC | [ ] |

### ADC (ADC)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| ADMUX | 0x7c | 0x0 | 0xff | REFS, ADLAR, MUX | [ ] |
| ADCSRA | 0x7a | 0x0 | 0xff | ADEN, ADSC, ADATE, ADIF, ADIE, ADPS | [ ] |
| ADC | 0x78 | 0x0 | 0xffff | ADC | [ ] |
| ADCSRB | 0x7b | 0x0 | 0xff | ADTS | [ ] |
| DIDR0 | 0x7e | 0x0 | 0xff | ADC7D, ADC6D, ADC5D, ADC4D, ADC3D, ADC2D, ADC1D, ADC0D | [ ] |

### BOOT_LOAD (BOOT_LOAD)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| SPMCSR | 0x57 | 0x0 | 0xff | SPMIE, RWWSB, RWWSRE, BLBSET, PGWRT, PGERS, SPMEN | [ ] |

### USART0 (USART)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| UDR0 | 0xc6 | 0x0 | 0xff | UDR0 | [ ] |
| UCSR0A | 0xc0 | 0x0 | 0xff | RXC0, TXC0, UDRE0, FE0, DOR0, UPE0, U2X0, MPCM0 | [ ] |
| UCSR0B | 0xc1 | 0x0 | 0xff | RXCIE0, TXCIE0, UDRIE0, RXEN0, TXEN0, UCSZ02, RXB80, TXB80 | [ ] |
| UCSR0C | 0xc2 | 0x0 | 0xff | UMSEL0, UPM0, USBS0, UCSZ0, UCPOL0 | [ ] |
| UBRR0 | 0xc4 | 0x0 | 0xfff | UBRR0 | [ ] |

### FUSE (FUSE)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| EXTENDED | 0x2 | 0xff | 0xff | BODLEVEL, RSTDISBL | [ ] |
| HIGH | 0x1 | 0x99 | 0xff | OCDEN, JTAGEN, SPIEN, WDTON, EESAVE, BOOTSZ, BOOTRST | [ ] |
| LOW | 0x0 | 0x62 | 0xff | CKDIV8, CKOUT, SUT_CKSEL | [ ] |

### LOCKBIT (LOCKBIT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| LOCKBIT | 0x0 | 0xff | 0xff | LB, BLB0, BLB1 | [ ] |

