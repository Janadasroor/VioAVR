# Hardware Accuracy Audit: ATmega2560

## 1. Memory Configuration

| Segment | Start | Size | Type | Status |
| --- | --- | --- | --- | --- |
| FLASH | 0x0 | 0x40000 | flash | [ ] |
| BOOT_SECTION_1 | 0x3fc00 | 0x400 | flash | [ ] |
| BOOT_SECTION_2 | 0x3f800 | 0x800 | flash | [ ] |
| BOOT_SECTION_3 | 0x3f000 | 0x1000 | flash | [ ] |
| BOOT_SECTION_4 | 0x3e000 | 0x2000 | flash | [ ] |
| SIGNATURES | 0x0 | 0x3 | signatures | [ ] |
| FUSES | 0x0 | 0x3 | fuses | [ ] |
| LOCKBITS | 0x0 | 0x1 | lockbits | [ ] |
| REGISTERS | 0x0 | 0x20 | regs | [ ] |
| MAPPED_IO | 0x20 | 0x1e0 | io | [ ] |
| IRAM | 0x200 | 0x2000 | ram | [ ] |
| XRAM | 0x2200 | 0xde00 | ram | [ ] |
| EEPROM | 0x0 | 0x1000 | eeprom | [ ] |
| OSCCAL | 0x0 | 0x1 | osccal | [ ] |

## 2. Interrupt Vector Table

| Index | Name | Caption | Status |
| --- | --- | --- | --- |
| 0 | RESET | External Pin,Power-on Reset,Brown-out Reset,Watchdog Reset,and JTAG AVR Reset. See Datasheet.      | [ ] |
| 1 | INT0 | External Interrupt Request 0 | [ ] |
| 2 | INT1 | External Interrupt Request 1 | [ ] |
| 3 | INT2 | External Interrupt Request 2 | [ ] |
| 4 | INT3 | External Interrupt Request 3 | [ ] |
| 5 | INT4 | External Interrupt Request 4 | [ ] |
| 6 | INT5 | External Interrupt Request 5 | [ ] |
| 7 | INT6 | External Interrupt Request 6 | [ ] |
| 8 | INT7 | External Interrupt Request 7 | [ ] |
| 9 | PCINT0 | Pin Change Interrupt Request 0 | [ ] |
| 10 | PCINT1 | Pin Change Interrupt Request 1 | [ ] |
| 11 | PCINT2 | Pin Change Interrupt Request 2 | [ ] |
| 12 | WDT | Watchdog Time-out Interrupt | [ ] |
| 13 | TIMER2_COMPA | Timer/Counter2 Compare Match A | [ ] |
| 14 | TIMER2_COMPB | Timer/Counter2 Compare Match B | [ ] |
| 15 | TIMER2_OVF | Timer/Counter2 Overflow | [ ] |
| 16 | TIMER1_CAPT | Timer/Counter1 Capture Event | [ ] |
| 17 | TIMER1_COMPA | Timer/Counter1 Compare Match A | [ ] |
| 18 | TIMER1_COMPB | Timer/Counter1 Compare Match B | [ ] |
| 19 | TIMER1_COMPC | Timer/Counter1 Compare Match C | [ ] |
| 20 | TIMER1_OVF | Timer/Counter1 Overflow | [ ] |
| 21 | TIMER0_COMPA | Timer/Counter0 Compare Match A | [ ] |
| 22 | TIMER0_COMPB | Timer/Counter0 Compare Match B | [ ] |
| 23 | TIMER0_OVF | Timer/Counter0 Overflow | [ ] |
| 24 | SPI_STC | SPI Serial Transfer Complete | [ ] |
| 25 | USART0_RX | USART0, Rx Complete | [ ] |
| 26 | USART0_UDRE | USART0 Data register Empty | [ ] |
| 27 | USART0_TX | USART0, Tx Complete | [ ] |
| 28 | ANALOG_COMP | Analog Comparator | [ ] |
| 29 | ADC | ADC Conversion Complete | [ ] |
| 30 | EE_READY | EEPROM Ready | [ ] |
| 31 | TIMER3_CAPT | Timer/Counter3 Capture Event | [ ] |
| 32 | TIMER3_COMPA | Timer/Counter3 Compare Match A | [ ] |
| 33 | TIMER3_COMPB | Timer/Counter3 Compare Match B | [ ] |
| 34 | TIMER3_COMPC | Timer/Counter3 Compare Match C | [ ] |
| 35 | TIMER3_OVF | Timer/Counter3 Overflow | [ ] |
| 36 | USART1_RX | USART1, Rx Complete | [ ] |
| 37 | USART1_UDRE | USART1 Data register Empty | [ ] |
| 38 | USART1_TX | USART1, Tx Complete | [ ] |
| 39 | TWI | 2-wire Serial Interface | [ ] |
| 40 | SPM_READY | Store Program Memory Read | [ ] |
| 41 | TIMER4_CAPT | Timer/Counter4 Capture Event | [ ] |
| 42 | TIMER4_COMPA | Timer/Counter4 Compare Match A | [ ] |
| 43 | TIMER4_COMPB | Timer/Counter4 Compare Match B | [ ] |
| 44 | TIMER4_COMPC | Timer/Counter4 Compare Match C | [ ] |
| 45 | TIMER4_OVF | Timer/Counter4 Overflow | [ ] |
| 46 | TIMER5_CAPT | Timer/Counter5 Capture Event | [ ] |
| 47 | TIMER5_COMPA | Timer/Counter5 Compare Match A | [ ] |
| 48 | TIMER5_COMPB | Timer/Counter5 Compare Match B | [ ] |
| 49 | TIMER5_COMPC | Timer/Counter5 Compare Match C | [ ] |
| 50 | TIMER5_OVF | Timer/Counter5 Overflow | [ ] |
| 51 | USART2_RX | USART2, Rx Complete | [ ] |
| 52 | USART2_UDRE | USART2 Data register Empty | [ ] |
| 53 | USART2_TX | USART2, Tx Complete | [ ] |
| 54 | USART3_RX | USART3, Rx Complete | [ ] |
| 55 | USART3_UDRE | USART3 Data register Empty | [ ] |
| 56 | USART3_TX | USART3, Tx Complete | [ ] |

## 3. Peripheral Register Map

### AC (AC)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| ADCSRB | 0x7b | 0x0 | 0xff | ACME | [ ] |
| ACSR | 0x50 | 0x0 | 0xff | ACD, ACBG, ACO, ACI, ACIE, ACIC, ACIS | [ ] |
| DIDR1 | 0x7f | 0x0 | 0xff | AIN1D, AIN0D | [ ] |

### USART0 (USART)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| UDR0 | 0xc6 | 0x0 | 0xff |  | [ ] |
| UCSR0A | 0xc0 | 0x0 | 0xff | RXC0, TXC0, UDRE0, FE0, DOR0, UPE0, U2X0, MPCM0 | [ ] |
| UCSR0B | 0xc1 | 0x0 | 0xff | RXCIE0, TXCIE0, UDRIE0, RXEN0, TXEN0, UCSZ02, RXB80, TXB80 | [ ] |
| UCSR0C | 0xc2 | 0x0 | 0xff | UMSEL0, UPM0, USBS0, UCSZ0, UCPOL0 | [ ] |
| UBRR0 | 0xc4 | 0x0 | 0xfff |  | [ ] |

### USART1 (USART)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| UDR1 | 0xce | 0x0 | 0xff |  | [ ] |
| UCSR1A | 0xc8 | 0x0 | 0xff | RXC1, TXC1, UDRE1, FE1, DOR1, UPE1, U2X1, MPCM1 | [ ] |
| UCSR1B | 0xc9 | 0x0 | 0xff | RXCIE1, TXCIE1, UDRIE1, RXEN1, TXEN1, UCSZ12, RXB81, TXB81 | [ ] |
| UCSR1C | 0xca | 0x0 | 0xff | UMSEL1, UPM1, USBS1, UCSZ1, UCPOL1 | [ ] |
| UBRR1 | 0xcc | 0x0 | 0xfff |  | [ ] |

### USART2 (USART)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| UDR2 | 0xd6 | 0x0 | 0xff |  | [ ] |
| UCSR2A | 0xd0 | 0x0 | 0xff | RXC2, TXC2, UDRE2, FE2, DOR2, UPE2, U2X2, MPCM2 | [ ] |
| UCSR2B | 0xd1 | 0x0 | 0xff | RXCIE2, TXCIE2, UDRIE2, RXEN2, TXEN2, UCSZ22, RXB82, TXB82 | [ ] |
| UCSR2C | 0xd2 | 0x0 | 0xff | UMSEL2, UPM2, USBS2, UCSZ2, UCPOL2 | [ ] |
| UBRR2 | 0xd4 | 0x0 | 0xfff |  | [ ] |

### USART3 (USART)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| UDR3 | 0x136 | 0x0 | 0xff |  | [ ] |
| UCSR3A | 0x130 | 0x0 | 0xff | RXC3, TXC3, UDRE3, FE3, DOR3, UPE3, U2X3, MPCM3 | [ ] |
| UCSR3B | 0x131 | 0x0 | 0xff | RXCIE3, TXCIE3, UDRIE3, RXEN3, TXEN3, UCSZ32, RXB83, TXB83 | [ ] |
| UCSR3C | 0x132 | 0x0 | 0xff | UMSEL3, UPM3, USBS3, UCSZ3, UCPOL3 | [ ] |
| UBRR3 | 0x134 | 0x0 | 0xfff |  | [ ] |

### TWI (TWI)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| TWAMR | 0xbd | 0x0 | 0xff | TWAM | [ ] |
| TWBR | 0xb8 | 0x0 | 0xff |  | [ ] |
| TWCR | 0xbc | 0x0 | 0xff | TWINT, TWEA, TWSTA, TWSTO, TWWC, TWEN, TWIE | [ ] |
| TWSR | 0xb9 | 0x0 | 0xff | TWS, TWPS | [ ] |
| TWDR | 0xbb | 0x0 | 0xff |  | [ ] |
| TWAR | 0xba | 0x0 | 0xff | TWA, TWGCE | [ ] |

### SPI (SPI)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| SPCR | 0x4c | 0x0 | 0xff | SPIE, SPE, DORD, MSTR, CPOL, CPHA, SPR | [ ] |
| SPSR | 0x4d | 0x0 | 0xff | SPIF, WCOL, SPI2X | [ ] |
| SPDR | 0x4e | 0x0 | 0xff |  | [ ] |

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

### PORTH (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PORTH | 0x102 | 0x0 | 0xff |  | [ ] |
| DDRH | 0x101 | 0x0 | 0xff |  | [ ] |
| PINH | 0x100 | 0x0 | 0xff |  | [ ] |

### PORTJ (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PORTJ | 0x105 | 0x0 | 0xff |  | [ ] |
| DDRJ | 0x104 | 0x0 | 0xff |  | [ ] |
| PINJ | 0x103 | 0x0 | 0xff |  | [ ] |

### PORTK (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PORTK | 0x108 | 0x0 | 0xff |  | [ ] |
| DDRK | 0x107 | 0x0 | 0xff |  | [ ] |
| PINK | 0x106 | 0x0 | 0xff |  | [ ] |

### PORTL (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PORTL | 0x10b | 0x0 | 0xff |  | [ ] |
| DDRL | 0x10a | 0x0 | 0xff |  | [ ] |
| PINL | 0x109 | 0x0 | 0xff |  | [ ] |

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

### WDT (WDT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| WDTCSR | 0x60 | 0x0 | 0xff | WDIF, WDIE, WDP, WDCE, WDE | [ ] |

### EEPROM (EEPROM)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| EEAR | 0x41 | 0x0 | 0xfff |  | [ ] |
| EEDR | 0x40 | 0x0 | 0xff |  | [ ] |
| EECR | 0x3f | 0x0 | 0xff | EEPM, EERIE, EEMPE, EEPE, EERE | [ ] |

### TC5 (TC16)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| TCCR5A | 0x120 | 0x0 | 0xff | COM5A, COM5B, COM5C, WGM5 | [ ] |
| TCCR5B | 0x121 | 0x0 | 0xff | ICNC5, ICES5, WGM5, CS5 | [ ] |
| TCCR5C | 0x122 | 0x0 | 0xff | FOC5A, FOC5B, FOC5C | [ ] |
| TCNT5 | 0x124 | 0x0 | 0xffff |  | [ ] |
| OCR5A | 0x128 | 0x0 | 0xffff |  | [ ] |
| OCR5B | 0x12a | 0x0 | 0xffff |  | [ ] |
| OCR5C | 0x12c | 0x0 | 0xffff |  | [ ] |
| ICR5 | 0x126 | 0x0 | 0xffff |  | [ ] |
| TIMSK5 | 0x73 | 0x0 | 0xff | ICIE5, OCIE5C, OCIE5B, OCIE5A, TOIE5 | [ ] |
| TIFR5 | 0x3a | 0x0 | 0xff | ICF5, OCF5C, OCF5B, OCF5A, TOV5 | [ ] |

### TC4 (TC16)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| TCCR4A | 0xa0 | 0x0 | 0xff | COM4A, COM4B, COM4C, WGM4 | [ ] |
| TCCR4B | 0xa1 | 0x0 | 0xff | ICNC4, ICES4, WGM4, CS4 | [ ] |
| TCCR4C | 0xa2 | 0x0 | 0xff | FOC4A, FOC4B, FOC4C | [ ] |
| TCNT4 | 0xa4 | 0x0 | 0xffff |  | [ ] |
| OCR4A | 0xa8 | 0x0 | 0xffff |  | [ ] |
| OCR4B | 0xaa | 0x0 | 0xffff |  | [ ] |
| OCR4C | 0xac | 0x0 | 0xffff |  | [ ] |
| ICR4 | 0xa6 | 0x0 | 0xffff |  | [ ] |
| TIMSK4 | 0x72 | 0x0 | 0xff | ICIE4, OCIE4C, OCIE4B, OCIE4A, TOIE4 | [ ] |
| TIFR4 | 0x39 | 0x0 | 0xff | ICF4, OCF4C, OCF4B, OCF4A, TOV4 | [ ] |

### TC3 (TC16)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| TCCR3A | 0x90 | 0x0 | 0xff | COM3A, COM3B, COM3C, WGM3 | [ ] |
| TCCR3B | 0x91 | 0x0 | 0xff | ICNC3, ICES3, WGM3, CS3 | [ ] |
| TCCR3C | 0x92 | 0x0 | 0xff | FOC3A, FOC3B, FOC3C | [ ] |
| TCNT3 | 0x94 | 0x0 | 0xffff |  | [ ] |
| OCR3A | 0x98 | 0x0 | 0xffff |  | [ ] |
| OCR3B | 0x9a | 0x0 | 0xffff |  | [ ] |
| OCR3C | 0x9c | 0x0 | 0xffff |  | [ ] |
| ICR3 | 0x96 | 0x0 | 0xffff |  | [ ] |
| TIMSK3 | 0x71 | 0x0 | 0xff | ICIE3, OCIE3C, OCIE3B, OCIE3A, TOIE3 | [ ] |
| TIFR3 | 0x38 | 0x0 | 0xff | ICF3, OCF3C, OCF3B, OCF3A, TOV3 | [ ] |

### TC1 (TC16)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| TCCR1A | 0x80 | 0x0 | 0xff | COM1A, COM1B, COM1C, WGM1 | [ ] |
| TCCR1B | 0x81 | 0x0 | 0xff | ICNC1, ICES1, WGM1, CS1 | [ ] |
| TCCR1C | 0x82 | 0x0 | 0xff | FOC1A, FOC1B, FOC1C | [ ] |
| TCNT1 | 0x84 | 0x0 | 0xffff |  | [ ] |
| OCR1A | 0x88 | 0x0 | 0xffff |  | [ ] |
| OCR1B | 0x8a | 0x0 | 0xffff |  | [ ] |
| OCR1C | 0x8c | 0x0 | 0xffff |  | [ ] |
| ICR1 | 0x86 | 0x0 | 0xffff |  | [ ] |
| TIMSK1 | 0x6f | 0x0 | 0xff | ICIE1, OCIE1C, OCIE1B, OCIE1A, TOIE1 | [ ] |
| TIFR1 | 0x36 | 0x0 | 0xff | ICF1, OCF1C, OCF1B, OCF1A, TOV1 | [ ] |

### JTAG (JTAG)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| OCDR | 0x51 | 0x0 | 0xff |  | [ ] |
| MCUCR | 0x55 | 0x0 | 0xff | JTD | [ ] |
| MCUSR | 0x54 | 0x0 | 0xff | JTRF | [ ] |

### EXINT (EXINT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| EICRA | 0x69 | 0x0 | 0xff | ISC3, ISC2, ISC1, ISC0 | [ ] |
| EICRB | 0x6a | 0x0 | 0xff | ISC7, ISC6, ISC5, ISC4 | [ ] |
| EIMSK | 0x3d | 0x0 | 0xff | INT | [ ] |
| EIFR | 0x3c | 0x0 | 0xff | INTF | [ ] |
| PCMSK2 | 0x6d | 0x0 | 0xff | PCINT | [ ] |
| PCMSK1 | 0x6c | 0x0 | 0xff | PCINT | [ ] |
| PCMSK0 | 0x6b | 0x0 | 0xff | PCINT | [ ] |
| PCIFR | 0x3b | 0x0 | 0xff | PCIF | [ ] |
| PCICR | 0x68 | 0x0 | 0xff | PCIE | [ ] |

### CPU (CPU)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| SREG | 0x5f | 0x0 | 0xff | I, T, H, S, V, N, Z, C | [ ] |
| SP | 0x5d | 0x0 | 0xffff |  | [ ] |
| MCUCR | 0x55 | 0x0 | 0xff | JTD, PUD, IVSEL, IVCE | [ ] |
| MCUSR | 0x54 | 0x0 | 0xff | JTRF, WDRF, BORF, EXTRF, PORF | [ ] |
| XMCRA | 0x74 | 0x0 | 0xff | SRE, SRL, SRW1, SRW0 | [ ] |
| XMCRB | 0x75 | 0x0 | 0xff | XMBK, XMM | [ ] |
| OSCCAL | 0x66 | 0x0 | 0xff | OSCCAL | [ ] |
| CLKPR | 0x61 | 0x0 | 0xff | CLKPCE, CLKPS | [ ] |
| SMCR | 0x53 | 0x0 | 0xff | SM, SE | [ ] |
| EIND | 0x5c | 0x0 | 0x1 |  | [ ] |
| RAMPZ | 0x5b | 0x0 | 0x3 |  | [ ] |
| GPIOR2 | 0x4b | 0x0 | 0xff | GPIOR | [ ] |
| GPIOR1 | 0x4a | 0x0 | 0xff | GPIOR | [ ] |
| GPIOR0 | 0x3e | 0x0 | 0xff | GPIOR07, GPIOR06, GPIOR05, GPIOR04, GPIOR03, GPIOR02, GPIOR01, GPIOR00 | [ ] |
| PRR1 | 0x65 | 0x0 | 0xff | PRTIM5, PRTIM4, PRTIM3, PRUSART3, PRUSART2, PRUSART1 | [ ] |
| PRR0 | 0x64 | 0x0 | 0xff | PRTWI, PRTIM2, PRTIM0, PRTIM1, PRSPI, PRUSART0, PRADC | [ ] |

### ADC (ADC)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| ADMUX | 0x7c | 0x0 | 0xff | REFS, ADLAR, MUX | [ ] |
| ADC | 0x78 | 0x0 | 0xffff |  | [ ] |
| ADCSRA | 0x7a | 0x0 | 0xff | ADEN, ADSC, ADATE, ADIF, ADIE, ADPS | [ ] |
| ADCSRB | 0x7b | 0x0 | 0xff | ACME, MUX5, ADTS | [ ] |
| DIDR2 | 0x7d | 0x0 | 0xff | ADC15D, ADC14D, ADC13D, ADC12D, ADC11D, ADC10D, ADC9D, ADC8D | [ ] |
| DIDR0 | 0x7e | 0x0 | 0xff | ADC7D, ADC6D, ADC5D, ADC4D, ADC3D, ADC2D, ADC1D, ADC0D | [ ] |

### BOOT_LOAD (BOOT_LOAD)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| SPMCSR | 0x57 | 0x0 | 0xff | SPMIE, RWWSB, SIGRD, RWWSRE, BLBSET, PGWRT, PGERS, SPMEN | [ ] |

### FUSE (FUSE)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| EXTENDED | 0x2 | 0xff | 0xff | BODLEVEL | [ ] |
| HIGH | 0x1 | 0x99 | 0xff | OCDEN, JTAGEN, SPIEN, WDTON, EESAVE, BOOTSZ, BOOTRST | [ ] |
| LOW | 0x0 | 0x62 | 0xff | CKDIV8, CKOUT, SUT_CKSEL | [ ] |

### LOCKBIT (LOCKBIT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| LOCKBIT | 0x0 | 0xff | 0xff | LB, BLB0, BLB1 | [ ] |

