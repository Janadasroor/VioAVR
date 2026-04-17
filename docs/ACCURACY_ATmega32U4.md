# Hardware Accuracy Audit: ATmega32U4

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
| IRAM | 0x100 | 0xa00 | ram | [ ] |
| EEPROM | 0x0 | 0x400 | eeprom | [ ] |
| OSCCAL | 0x0 | 0x1 | osccal | [ ] |

## 2. Interrupt Vector Table

| Index | Name | Caption | Status |
| --- | --- | --- | --- |
| 0 | RESET | External Pin,Power-on Reset,Brown-out Reset,Watchdog Reset,and JTAG AVR Reset. See Datasheet.      | [ ] |
| 1 | INT0 | External Interrupt Request 0 | [ ] |
| 2 | INT1 | External Interrupt Request 1 | [ ] |
| 3 | INT2 | External Interrupt Request 2 | [ ] |
| 4 | INT3 | External Interrupt Request 3 | [ ] |
| 5 | Reserved1 | Reserved1 | [ ] |
| 6 | Reserved2 | Reserved2 | [ ] |
| 7 | INT6 | External Interrupt Request 6 | [ ] |
| 8 | Reserved3 | Reserved3 | [ ] |
| 9 | PCINT0 | Pin Change Interrupt Request 0 | [ ] |
| 10 | USB_GEN | USB General Interrupt Request | [ ] |
| 11 | USB_COM | USB Endpoint/Pipe Interrupt Communication Request | [ ] |
| 12 | WDT | Watchdog Time-out Interrupt | [ ] |
| 13 | Reserved4 | Reserved4 | [ ] |
| 14 | Reserved5 | Reserved5 | [ ] |
| 15 | Reserved6 | Reserved6 | [ ] |
| 16 | TIMER1_CAPT | Timer/Counter1 Capture Event | [ ] |
| 17 | TIMER1_COMPA | Timer/Counter1 Compare Match A | [ ] |
| 18 | TIMER1_COMPB | Timer/Counter1 Compare Match B | [ ] |
| 19 | TIMER1_COMPC | Timer/Counter1 Compare Match C | [ ] |
| 20 | TIMER1_OVF | Timer/Counter1 Overflow | [ ] |
| 21 | TIMER0_COMPA | Timer/Counter0 Compare Match A | [ ] |
| 22 | TIMER0_COMPB | Timer/Counter0 Compare Match B | [ ] |
| 23 | TIMER0_OVF | Timer/Counter0 Overflow | [ ] |
| 24 | SPI_STC | SPI Serial Transfer Complete | [ ] |
| 25 | USART1_RX | USART1, Rx Complete | [ ] |
| 26 | USART1_UDRE | USART1 Data register Empty | [ ] |
| 27 | USART1_TX | USART1, Tx Complete | [ ] |
| 28 | ANALOG_COMP | Analog Comparator | [ ] |
| 29 | ADC | ADC Conversion Complete | [ ] |
| 30 | EE_READY | EEPROM Ready | [ ] |
| 31 | TIMER3_CAPT | Timer/Counter3 Capture Event | [ ] |
| 32 | TIMER3_COMPA | Timer/Counter3 Compare Match A | [ ] |
| 33 | TIMER3_COMPB | Timer/Counter3 Compare Match B | [ ] |
| 34 | TIMER3_COMPC | Timer/Counter3 Compare Match C | [ ] |
| 35 | TIMER3_OVF | Timer/Counter3 Overflow | [ ] |
| 36 | TWI | 2-wire Serial Interface         | [ ] |
| 37 | SPM_READY | Store Program Memory Read | [ ] |
| 38 | TIMER4_COMPA | Timer/Counter4 Compare Match A | [ ] |
| 39 | TIMER4_COMPB | Timer/Counter4 Compare Match B | [ ] |
| 40 | TIMER4_COMPD | Timer/Counter4 Compare Match D | [ ] |
| 41 | TIMER4_OVF | Timer/Counter4 Overflow | [ ] |
| 42 | TIMER4_FPF | Timer/Counter4 Fault Protection Interrupt | [ ] |

## 3. Peripheral Register Map

### WDT (WDT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| WDTCSR | 0x60 | 0x0 | 0xff | WDIF, WDIE, WDP, WDCE, WDE | [ ] |

### PORTB (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PORTB | 0x25 | 0x0 | 0xff |  | [ ] |
| DDRB | 0x24 | 0x0 | 0xff |  | [ ] |
| PINB | 0x23 | 0x0 | 0xff |  | [ ] |

### PORTC (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PORTC | 0x28 | 0x0 | 0xc0 |  | [ ] |
| DDRC | 0x27 | 0x0 | 0xc0 |  | [ ] |
| PINC | 0x26 | 0x0 | 0xc0 |  | [ ] |

### PORTD (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PORTD | 0x2b | 0x0 | 0xff |  | [ ] |
| DDRD | 0x2a | 0x0 | 0xff |  | [ ] |
| PIND | 0x29 | 0x0 | 0xff |  | [ ] |

### PORTE (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PORTE | 0x2e | 0x0 | 0x44 |  | [ ] |
| DDRE | 0x2d | 0x0 | 0x44 |  | [ ] |
| PINE | 0x2c | 0x0 | 0x44 |  | [ ] |

### PORTF (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PORTF | 0x31 | 0x0 | 0xf3 |  | [ ] |
| DDRF | 0x30 | 0x0 | 0xf3 |  | [ ] |
| PINF | 0x2f | 0x0 | 0xf3 |  | [ ] |

### SPI (SPI)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| SPCR | 0x4c | 0x0 | 0xff | SPIE, SPE, DORD, MSTR, CPOL, CPHA, SPR | [ ] |
| SPSR | 0x4d | 0x0 | 0xff | SPIF, WCOL, SPI2X | [ ] |
| SPDR | 0x4e | 0x0 | 0xff |  | [ ] |

### USART1 (USART)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| UDR1 | 0xce | 0x0 | 0xff |  | [ ] |
| UCSR1A | 0xc8 | 0x0 | 0xff | RXC1, TXC1, UDRE1, FE1, DOR1, UPE1, U2X1, MPCM1 | [ ] |
| UCSR1B | 0xc9 | 0x0 | 0xff | RXCIE1, TXCIE1, UDRIE1, RXEN1, TXEN1, UCSZ12, RXB81, TXB81 | [ ] |
| UCSR1C | 0xca | 0x0 | 0xff | UMSEL1, UPM1, USBS1, UCSZ1, UCPOL1 | [ ] |
| UCSR1D | 0xcb | 0x0 | 0xff | CTSEN, RTSEN | [ ] |
| UBRR1 | 0xcc | 0x0 | 0xfff |  | [ ] |

### BOOT_LOAD (BOOT_LOAD)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| SPMCSR | 0x57 | 0x0 | 0xff | SPMIE, RWWSB, SIGRD, RWWSRE, BLBSET, PGWRT, PGERS, SPMEN | [ ] |

### EEPROM (EEPROM)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| EEAR | 0x41 | 0x0 | 0xfff |  | [ ] |
| EEDR | 0x40 | 0x0 | 0xff |  | [ ] |
| EECR | 0x3f | 0x0 | 0xff | EEPM, EERIE, EEMPE, EEPE, EERE | [ ] |

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
| GTCCR | 0x43 | 0x0 | 0xff | TSM, PSRASY, PSRSYNC | [ ] |

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

### TC4 (TC10)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| TCCR4A | 0xc0 | 0x0 | 0xff | COM4A, COM4B, FOC4A, FOC4B, PWM4A, PWM4B | [ ] |
| TCCR4B | 0xc1 | 0x0 | 0xff | PWM4X, PSR4, DTPS4, CS4 | [ ] |
| TCCR4C | 0xc2 | 0x0 | 0xff | COM4A1S, COM4A0S, COM4B1S, COM4B0S, COM4D, FOC4D, PWM4D | [ ] |
| TCCR4D | 0xc3 | 0x0 | 0xff | FPIE4, FPEN4, FPNC4, FPES4, FPAC4, FPF4, WGM4 | [ ] |
| TCCR4E | 0xc4 | 0x0 | 0xff | TLOCK4, ENHC4, OC4OE | [ ] |
| TCNT4 | 0xbe | 0x0 | 0xff |  | [ ] |
| TC4H | 0xbf | 0x0 | 0x7 |  | [ ] |
| OCR4A | 0xcf | 0x0 | 0xff |  | [ ] |
| OCR4B | 0xd0 | 0x0 | 0xff |  | [ ] |
| OCR4C | 0xd1 | 0x0 | 0xff |  | [ ] |
| OCR4D | 0xd2 | 0x0 | 0xff |  | [ ] |
| TIMSK4 | 0x72 | 0x0 | 0xff | OCIE4D, OCIE4A, OCIE4B, TOIE4 | [ ] |
| TIFR4 | 0x39 | 0x0 | 0xff | OCF4D, OCF4A, OCF4B, TOV4 | [ ] |
| DT4 | 0xd4 | 0x0 | 0xff | DT4L | [ ] |

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
| PCMSK0 | 0x6b | 0x0 | 0xff |  | [ ] |
| PCIFR | 0x3b | 0x0 | 0xff | PCIF0 | [ ] |
| PCICR | 0x68 | 0x0 | 0xff | PCIE0 | [ ] |

### TWI (TWI)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| TWAMR | 0xbd | 0x0 | 0xff | TWAM | [ ] |
| TWBR | 0xb8 | 0x0 | 0xff |  | [ ] |
| TWCR | 0xbc | 0x0 | 0xff | TWINT, TWEA, TWSTA, TWSTO, TWWC, TWEN, TWIE | [ ] |
| TWSR | 0xb9 | 0x0 | 0xff | TWS, TWPS | [ ] |
| TWDR | 0xbb | 0x0 | 0xff |  | [ ] |
| TWAR | 0xba | 0x0 | 0xff | TWA, TWGCE | [ ] |

### ADC (ADC)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| ADMUX | 0x7c | 0x0 | 0xff | REFS, ADLAR, MUX | [ ] |
| ADCSRA | 0x7a | 0x0 | 0xff | ADEN, ADSC, ADATE, ADIF, ADIE, ADPS | [ ] |
| ADC | 0x78 | 0x0 | 0xffff |  | [ ] |
| ADCSRB | 0x7b | 0x0 | 0xff | ADHSM, MUX5, ADTS | [ ] |
| DIDR0 | 0x7e | 0x0 | 0xff | ADC7D, ADC6D, ADC5D, ADC4D, ADC3D, ADC2D, ADC1D, ADC0D | [ ] |
| DIDR2 | 0x7d | 0x0 | 0xff | ADC13D, ADC12D, ADC11D, ADC10D, ADC9D, ADC8D | [ ] |

### AC (AC)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| ADCSRB | 0x7b | 0x0 | 0xff | ACME | [ ] |
| ACSR | 0x50 | 0x0 | 0xff | ACD, ACBG, ACO, ACI, ACIE, ACIC, ACIS | [ ] |
| DIDR1 | 0x7f | 0x0 | 0xff | AIN1D, AIN0D | [ ] |

### CPU (CPU)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| SREG | 0x5f | 0x0 | 0xff | I, T, H, S, V, N, Z, C | [ ] |
| SP | 0x5d | 0x0 | 0xffff |  | [ ] |
| MCUCR | 0x55 | 0x0 | 0xff | JTD, PUD, IVSEL, IVCE | [ ] |
| MCUSR | 0x54 | 0x0 | 0xff | JTRF, WDRF, BORF, EXTRF, PORF | [ ] |
| OSCCAL | 0x66 | 0x0 | 0xff | OSCCAL | [ ] |
| RCCTRL | 0x67 | 0x0 | 0xff | RCFREQ | [ ] |
| CLKPR | 0x61 | 0x0 | 0xff | CLKPCE, CLKPS | [ ] |
| SMCR | 0x53 | 0x0 | 0xff | SM, SE | [ ] |
| EIND | 0x5c | 0x0 | 0x1 |  | [ ] |
| RAMPZ | 0x5b | 0x0 | 0xff | Res, RAMPZ | [ ] |
| GPIOR2 | 0x4b | 0x0 | 0xff | GPIOR | [ ] |
| GPIOR1 | 0x4a | 0x0 | 0xff | GPIOR | [ ] |
| GPIOR0 | 0x3e | 0x0 | 0xff | GPIOR07, GPIOR06, GPIOR05, GPIOR04, GPIOR03, GPIOR02, GPIOR01, GPIOR00 | [ ] |
| PRR1 | 0x65 | 0x0 | 0xff | PRUSB, PRTIM4, PRTIM3, PRUSART1 | [ ] |
| PRR0 | 0x64 | 0x0 | 0xff | PRTWI, PRTIM2, PRTIM0, PRTIM1, PRSPI, PRUSART0, PRADC | [ ] |
| CLKSTA | 0xc7 | 0x0 | 0xff | RCON, EXTON | [ ] |
| CLKSEL1 | 0xc6 | 0x0 | 0xff | RCCKSEL, EXCKSEL | [ ] |
| CLKSEL0 | 0xc5 | 0x0 | 0xff | RCSUT, EXSUT, RCE, EXTE, CLKS | [ ] |

### PLL (PLL)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| PLLCSR | 0x49 | 0x0 | 0xff | PINDIV, PLLE, PLOCK | [ ] |
| PLLFRQ | 0x52 | 0x0 | 0xff | PINMUX, PLLUSB, PLLTM, PDIV | [ ] |

### USB_DEVICE (USB_DEVICE)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| UEINT | 0xf4 | 0x0 | 0x7f |  | [ ] |
| UEBCHX | 0xf3 | 0x0 | 0x7 |  | [ ] |
| UEBCLX | 0xf2 | 0x0 | 0xff |  | [ ] |
| UEDATX | 0xf1 | 0x0 | 0xff | DAT | [ ] |
| UEIENX | 0xf0 | 0x0 | 0xff | FLERRE, NAKINE, NAKOUTE, RXSTPE, RXOUTE, STALLEDE, TXINE | [ ] |
| UESTA1X | 0xef | 0x0 | 0xff | CTRLDIR, CURRBK | [ ] |
| UESTA0X | 0xee | 0x0 | 0xff | CFGOK, OVERFI, UNDERFI, DTSEQ, NBUSYBK | [ ] |
| UECFG1X | 0xed | 0x0 | 0xff | EPSIZE, EPBK, ALLOC | [ ] |
| UECFG0X | 0xec | 0x0 | 0xff | EPTYPE, EPDIR | [ ] |
| UECONX | 0xeb | 0x0 | 0xff | STALLRQ, STALLRQC, RSTDT, EPEN | [ ] |
| UERST | 0xea | 0x0 | 0xff | EPRST | [ ] |
| UENUM | 0xe9 | 0x0 | 0x7 |  | [ ] |
| UEINTX | 0xe8 | 0x0 | 0xff | FIFOCON, NAKINI, RWAL, NAKOUTI, RXSTPI, RXOUTI, STALLEDI, TXINI | [ ] |
| UDMFN | 0xe6 | 0x0 | 0xff | FNCERR | [ ] |
| UDFNUM | 0xe4 | 0x0 | 0x7ff |  | [ ] |
| UDADDR | 0xe3 | 0x0 | 0xff | ADDEN, UADD | [ ] |
| UDIEN | 0xe2 | 0x0 | 0xff | UPRSME, EORSME, WAKEUPE, EORSTE, SOFE, SUSPE | [ ] |
| UDINT | 0xe1 | 0x0 | 0xff | UPRSMI, EORSMI, WAKEUPI, EORSTI, SOFI, SUSPI | [ ] |
| UDCON | 0xe0 | 0x0 | 0xff | LSM, RSTCPU, RMWKUP, DETACH | [ ] |
| USBCON | 0xd8 | 0x0 | 0xff | USBE, FRZCLK, OTGPADE, VBUSTE | [ ] |
| USBINT | 0xda | 0x0 | 0xff | VBUSTI | [ ] |
| USBSTA | 0xd9 | 0x0 | 0xff | SPEED, VBUS | [ ] |
| UHWCON | 0xd7 | 0x0 | 0xff | UVREGE | [ ] |

### FUSE (FUSE)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| EXTENDED | 0x2 | 0xfb | 0xff | BODLEVEL, HWBE | [ ] |
| HIGH | 0x1 | 0x99 | 0xff | OCDEN, JTAGEN, SPIEN, WDTON, EESAVE, BOOTSZ, BOOTRST | [ ] |
| LOW | 0x0 | 0x52 | 0xff | CKDIV8, CKOUT, SUT_CKSEL | [ ] |

### LOCKBIT (LOCKBIT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| LOCKBIT | 0x0 | 0xff | 0xff | LB, BLB0, BLB1 | [ ] |

