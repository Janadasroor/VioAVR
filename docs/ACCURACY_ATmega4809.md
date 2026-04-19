# Hardware Accuracy Audit: ATmega4809

## 1. Memory Configuration

| Segment | Start | Size | Type | Status |
| --- | --- | --- | --- | --- |
| IO | 0x0 | 0x1100 | io | [ ] |
| SIGNATURES | 0x1100 | 0x3 | signatures | [ ] |
| PROD_SIGNATURES | 0x1103 | 0x7d | signatures | [ ] |
| FUSES | 0x1280 | 0xa | fuses | [ ] |
| LOCKBITS | 0x128a | 0x1 | lockbits | [ ] |
| USER_SIGNATURES | 0x1300 | 0x40 | user_signatures | [ ] |
| EEPROM | 0x1400 | 0x100 | eeprom | [ ] |
| INTERNAL_SRAM | 0x2800 | 0x1800 | ram | [ ] |
| MAPPED_PROGMEM | 0x4000 | 0xc000 | other | [ ] |
| PROGMEM | 0x0 | 0xc000 | flash | [ ] |

## 2. Interrupt Vector Table

| Index | Name | Caption | Status |
| --- | --- | --- | --- |
| 1 | NMI | None | [ ] |
| 2 | VLM | None | [ ] |
| 3 | CNT | None | [ ] |
| 4 | PIT | None | [ ] |
| 5 | CCL | None | [ ] |
| 6 | PORT | None | [ ] |
| 7 | LUNF | None | [ ] |
| 7 | OVF | None | [ ] |
| 8 | HUNF | None | [ ] |
| 9 | CMP0 | None | [ ] |
| 9 | LCMP0 | None | [ ] |
| 10 | CMP1 | None | [ ] |
| 10 | LCMP1 | None | [ ] |
| 11 | CMP2 | None | [ ] |
| 11 | LCMP2 | None | [ ] |
| 12 | INT | None | [ ] |
| 13 | INT | None | [ ] |
| 14 | TWIS | None | [ ] |
| 15 | TWIM | None | [ ] |
| 16 | INT | None | [ ] |
| 17 | RXC | None | [ ] |
| 18 | DRE | None | [ ] |
| 19 | TXC | None | [ ] |
| 20 | PORT | None | [ ] |
| 21 | AC | None | [ ] |
| 22 | RESRDY | None | [ ] |
| 23 | WCOMP | None | [ ] |
| 24 | PORT | None | [ ] |
| 25 | INT | None | [ ] |
| 26 | RXC | None | [ ] |
| 27 | DRE | None | [ ] |
| 28 | TXC | None | [ ] |
| 29 | PORT | None | [ ] |
| 30 | EE | None | [ ] |
| 31 | RXC | None | [ ] |
| 32 | DRE | None | [ ] |
| 33 | TXC | None | [ ] |
| 34 | PORT | None | [ ] |
| 35 | PORT | None | [ ] |
| 36 | INT | None | [ ] |
| 37 | RXC | None | [ ] |
| 38 | DRE | None | [ ] |
| 39 | TXC | None | [ ] |

## 3. Peripheral Register Map

### AC0 (AC)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| CTRLA | 0x680 | 0x0 | 0xff | ENABLE, HYSMODE, LPMODE, INTMODE, OUTEN, RUNSTDBY | [ ] |
| MUXCTRLA | 0x682 | 0x0 | 0xff | MUXNEG, MUXPOS, INVERT | [ ] |
| DACREF | 0x684 | 0xff | 0xff | DATA | [ ] |
| INTCTRL | 0x686 | 0x0 | 0xff | CMP | [ ] |
| STATUS | 0x687 | 0x0 | 0xff | CMP, STATE | [ ] |

### ADC0 (ADC)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| CTRLA | 0x600 | 0x0 | 0xff | ENABLE, FREERUN, RESSEL, RUNSTBY | [ ] |
| CTRLB | 0x601 | 0x0 | 0xff | SAMPNUM | [ ] |
| CTRLC | 0x602 | 0x0 | 0xff | PRESC, REFSEL, SAMPCAP | [ ] |
| CTRLD | 0x603 | 0x0 | 0xff | SAMPDLY, ASDV, INITDLY | [ ] |
| CTRLE | 0x604 | 0x0 | 0xff | WINCM | [ ] |
| SAMPCTRL | 0x605 | 0x0 | 0xff | SAMPLEN | [ ] |
| MUXPOS | 0x606 | 0x0 | 0xff | MUXPOS | [ ] |
| COMMAND | 0x608 | 0x0 | 0xff | STCONV | [ ] |
| EVCTRL | 0x609 | 0x0 | 0xff | STARTEI | [ ] |
| INTCTRL | 0x60a | 0x0 | 0xff | RESRDY, WCMP | [ ] |
| INTFLAGS | 0x60b | 0x0 | 0xff | RESRDY, WCMP | [ ] |
| DBGCTRL | 0x60c | 0x0 | 0xff | DBGRUN | [ ] |
| TEMP | 0x60d | 0x0 | 0xff | TEMP | [ ] |
| RES | 0x610 | 0x0 | 0xff |  | [ ] |
| WINLT | 0x612 | 0x0 | 0xff |  | [ ] |
| WINHT | 0x614 | 0x0 | 0xff |  | [ ] |
| CALIB | 0x616 | 0x1 | 0xff | DUTYCYC | [ ] |

### BOD (BOD)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| CTRLA | 0x80 | 0x5 | 0xff | SLEEP, ACTIVE, SAMPFREQ | [ ] |
| CTRLB | 0x81 | 0x0 | 0xff | LVL | [ ] |
| VLMCTRLA | 0x88 | 0x0 | 0xff | VLMLVL | [ ] |
| INTCTRL | 0x89 | 0x0 | 0xff | VLMIE, VLMCFG | [ ] |
| INTFLAGS | 0x8a | 0x0 | 0xff | VLMIF | [ ] |
| STATUS | 0x8b | 0x0 | 0xff | VLMS | [ ] |

### CCL (CCL)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| CTRLA | 0x1c0 | 0x0 | 0xff | ENABLE, RUNSTDBY | [ ] |
| SEQCTRL0 | 0x1c1 | 0x0 | 0xff | SEQSEL0 | [ ] |
| SEQCTRL1 | 0x1c2 | 0x0 | 0xff | SEQSEL1 | [ ] |
| INTCTRL0 | 0x1c5 | 0x0 | 0xff | INTMODE0, INTMODE1, INTMODE2, INTMODE3 | [ ] |
| INTFLAGS | 0x1c7 | 0x0 | 0xff | INT | [ ] |
| LUT0CTRLA | 0x1c8 | 0x0 | 0xff | ENABLE, CLKSRC, FILTSEL, OUTEN, EDGEDET | [ ] |
| LUT0CTRLB | 0x1c9 | 0x0 | 0xff | INSEL0, INSEL1 | [ ] |
| LUT0CTRLC | 0x1ca | 0x0 | 0xff | INSEL2 | [ ] |
| TRUTH0 | 0x1cb | 0x0 | 0xff | TRUTH | [ ] |
| LUT1CTRLA | 0x1cc | 0x0 | 0xff | ENABLE, CLKSRC, FILTSEL, OUTEN, EDGEDET | [ ] |
| LUT1CTRLB | 0x1cd | 0x0 | 0xff | INSEL0, INSEL1 | [ ] |
| LUT1CTRLC | 0x1ce | 0x0 | 0xff | INSEL2 | [ ] |
| TRUTH1 | 0x1cf | 0x0 | 0xff | TRUTH | [ ] |
| LUT2CTRLA | 0x1d0 | 0x0 | 0xff | ENABLE, CLKSRC, FILTSEL, OUTEN, EDGEDET | [ ] |
| LUT2CTRLB | 0x1d1 | 0x0 | 0xff | INSEL0, INSEL1 | [ ] |
| LUT2CTRLC | 0x1d2 | 0x0 | 0xff | INSEL2 | [ ] |
| TRUTH2 | 0x1d3 | 0x0 | 0xff | TRUTH | [ ] |
| LUT3CTRLA | 0x1d4 | 0x0 | 0xff | ENABLE, CLKSRC, FILTSEL, OUTEN, EDGEDET | [ ] |
| LUT3CTRLB | 0x1d5 | 0x0 | 0xff | INSEL0, INSEL1 | [ ] |
| LUT3CTRLC | 0x1d6 | 0x0 | 0xff | INSEL2 | [ ] |
| TRUTH3 | 0x1d7 | 0x0 | 0xff | TRUTH | [ ] |

### CLKCTRL (CLKCTRL)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| MCLKCTRLA | 0x60 | 0x0 | 0xff | CLKSEL, CLKOUT | [ ] |
| MCLKCTRLB | 0x61 | 0x11 | 0xff | PEN, PDIV | [ ] |
| MCLKLOCK | 0x62 | 0x0 | 0xff | LOCKEN | [ ] |
| MCLKSTATUS | 0x63 | 0x0 | 0xff | SOSC, OSC20MS, OSC32KS, XOSC32KS, EXTS | [ ] |
| OSC20MCTRLA | 0x70 | 0x0 | 0xff | RUNSTDBY | [ ] |
| OSC20MCALIBA | 0x71 | 0x0 | 0xff | CAL20M | [ ] |
| OSC20MCALIBB | 0x72 | 0x0 | 0xff | TEMPCAL20M, LOCK | [ ] |
| OSC32KCTRLA | 0x78 | 0x0 | 0xff | RUNSTDBY | [ ] |
| XOSC32KCTRLA | 0x7c | 0x0 | 0xff | ENABLE, RUNSTDBY, SEL, CSUT | [ ] |

### CPU (CPU)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| CCP | 0x34 | 0x0 | 0xff | CCP | [ ] |
| SPL | 0x3d | 0x0 | 0xff |  | [ ] |
| SPH | 0x3e | 0x0 | 0xff |  | [ ] |
| SREG | 0x3f | 0x0 | 0xff | C, Z, N, V, S, H, T, I | [ ] |

### CPUINT (CPUINT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| CTRLA | 0x110 | 0x0 | 0xff | LVL0RR, CVT, IVSEL | [ ] |
| STATUS | 0x111 | 0x0 | 0xff | LVL0EX, LVL1EX, NMIEX | [ ] |
| LVL0PRI | 0x112 | 0x0 | 0xff | LVL0PRI | [ ] |
| LVL1VEC | 0x113 | 0x0 | 0xff | LVL1VEC | [ ] |

### CRCSCAN (CRCSCAN)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| CTRLA | 0x120 | 0x0 | 0xff | ENABLE, NMIEN, RESET | [ ] |
| CTRLB | 0x121 | 0x0 | 0xff | SRC, MODE | [ ] |
| STATUS | 0x122 | 0x2 | 0xff | BUSY, OK | [ ] |

### EVSYS (EVSYS)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| STROBE | 0x180 | 0x0 | 0xff | STROBE0 | [ ] |
| CHANNEL0 | 0x190 | 0x0 | 0xff | GENERATOR | [ ] |
| CHANNEL1 | 0x191 | 0x0 | 0xff | GENERATOR | [ ] |
| CHANNEL2 | 0x192 | 0x0 | 0xff | GENERATOR | [ ] |
| CHANNEL3 | 0x193 | 0x0 | 0xff | GENERATOR | [ ] |
| CHANNEL4 | 0x194 | 0x0 | 0xff | GENERATOR | [ ] |
| CHANNEL5 | 0x195 | 0x0 | 0xff | GENERATOR | [ ] |
| CHANNEL6 | 0x196 | 0x0 | 0xff | GENERATOR | [ ] |
| CHANNEL7 | 0x197 | 0x0 | 0xff | GENERATOR | [ ] |
| USERCCLLUT0A | 0x1a0 | 0x0 | 0xff | CHANNEL | [ ] |
| USERCCLLUT0B | 0x1a1 | 0x0 | 0xff | CHANNEL | [ ] |
| USERCCLLUT1A | 0x1a2 | 0x0 | 0xff | CHANNEL | [ ] |
| USERCCLLUT1B | 0x1a3 | 0x0 | 0xff | CHANNEL | [ ] |
| USERCCLLUT2A | 0x1a4 | 0x0 | 0xff | CHANNEL | [ ] |
| USERCCLLUT2B | 0x1a5 | 0x0 | 0xff | CHANNEL | [ ] |
| USERCCLLUT3A | 0x1a6 | 0x0 | 0xff | CHANNEL | [ ] |
| USERCCLLUT3B | 0x1a7 | 0x0 | 0xff | CHANNEL | [ ] |
| USERADC0 | 0x1a8 | 0x0 | 0xff | CHANNEL | [ ] |
| USEREVOUTA | 0x1a9 | 0x0 | 0xff | CHANNEL | [ ] |
| USEREVOUTB | 0x1aa | 0x0 | 0xff | CHANNEL | [ ] |
| USEREVOUTC | 0x1ab | 0x0 | 0xff | CHANNEL | [ ] |
| USEREVOUTD | 0x1ac | 0x0 | 0xff | CHANNEL | [ ] |
| USEREVOUTE | 0x1ad | 0x0 | 0xff | CHANNEL | [ ] |
| USEREVOUTF | 0x1ae | 0x0 | 0xff | CHANNEL | [ ] |
| USERUSART0 | 0x1af | 0x0 | 0xff | CHANNEL | [ ] |
| USERUSART1 | 0x1b0 | 0x0 | 0xff | CHANNEL | [ ] |
| USERUSART2 | 0x1b1 | 0x0 | 0xff | CHANNEL | [ ] |
| USERUSART3 | 0x1b2 | 0x0 | 0xff | CHANNEL | [ ] |
| USERTCA0 | 0x1b3 | 0x0 | 0xff | CHANNEL | [ ] |
| USERTCB0 | 0x1b4 | 0x0 | 0xff | CHANNEL | [ ] |
| USERTCB1 | 0x1b5 | 0x0 | 0xff | CHANNEL | [ ] |
| USERTCB2 | 0x1b6 | 0x0 | 0xff | CHANNEL | [ ] |
| USERTCB3 | 0x1b7 | 0x0 | 0xff | CHANNEL | [ ] |

### FUSE (FUSE)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| WDTCFG | 0x1280 | 0x0 | 0xff | PERIOD, WINDOW | [ ] |
| BODCFG | 0x1281 | 0x0 | 0xff | SLEEP, ACTIVE, SAMPFREQ, LVL | [ ] |
| OSCCFG | 0x1282 | 0x7e | 0xff | FREQSEL, OSCLOCK | [ ] |
| SYSCFG0 | 0x1285 | 0xf6 | 0xff | EESAVE, RSTPINCFG, CRCSRC | [ ] |
| SYSCFG1 | 0x1286 | 0xff | 0xff | SUT | [ ] |
| APPEND | 0x1287 | 0x0 | 0xff |  | [ ] |
| BOOTEND | 0x1288 | 0x0 | 0xff |  | [ ] |

### GPIO (GPIO)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| GPIOR0 | 0x1c | 0x0 | 0xff |  | [ ] |
| GPIOR1 | 0x1d | 0x0 | 0xff |  | [ ] |
| GPIOR2 | 0x1e | 0x0 | 0xff |  | [ ] |
| GPIOR3 | 0x1f | 0x0 | 0xff |  | [ ] |

### LOCKBIT (LOCKBIT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| LOCKBIT | 0x128a | 0xc5 | 0xff | LB | [ ] |

### NVMCTRL (NVMCTRL)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| CTRLA | 0x1000 | 0x0 | 0xff | CMD | [ ] |
| CTRLB | 0x1001 | 0x0 | 0xff | APCWP, BOOTLOCK | [ ] |
| STATUS | 0x1002 | 0x0 | 0xff | FBUSY, EEBUSY, WRERROR | [ ] |
| INTCTRL | 0x1003 | 0x0 | 0xff | EEREADY | [ ] |
| INTFLAGS | 0x1004 | 0x0 | 0xff | EEREADY | [ ] |
| DATA | 0x1006 | 0x0 | 0xff |  | [ ] |
| ADDR | 0x1008 | 0x0 | 0xff |  | [ ] |

### PORTA (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| DIR | 0x400 | 0x0 | 0xff |  | [ ] |
| DIRSET | 0x401 | 0x0 | 0xff |  | [ ] |
| DIRCLR | 0x402 | 0x0 | 0xff |  | [ ] |
| DIRTGL | 0x403 | 0x0 | 0xff |  | [ ] |
| OUT | 0x404 | 0x0 | 0xff |  | [ ] |
| OUTSET | 0x405 | 0x0 | 0xff |  | [ ] |
| OUTCLR | 0x406 | 0x0 | 0xff |  | [ ] |
| OUTTGL | 0x407 | 0x0 | 0xff |  | [ ] |
| IN | 0x408 | 0x0 | 0xff |  | [ ] |
| INTFLAGS | 0x409 | 0x0 | 0xff | INT | [ ] |
| PORTCTRL | 0x40a | 0x0 | 0xff | SRL | [ ] |
| PIN0CTRL | 0x410 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN1CTRL | 0x411 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN2CTRL | 0x412 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN3CTRL | 0x413 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN4CTRL | 0x414 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN5CTRL | 0x415 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN6CTRL | 0x416 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN7CTRL | 0x417 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |

### PORTB (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| DIR | 0x420 | 0x0 | 0xff |  | [ ] |
| DIRSET | 0x421 | 0x0 | 0xff |  | [ ] |
| DIRCLR | 0x422 | 0x0 | 0xff |  | [ ] |
| DIRTGL | 0x423 | 0x0 | 0xff |  | [ ] |
| OUT | 0x424 | 0x0 | 0xff |  | [ ] |
| OUTSET | 0x425 | 0x0 | 0xff |  | [ ] |
| OUTCLR | 0x426 | 0x0 | 0xff |  | [ ] |
| OUTTGL | 0x427 | 0x0 | 0xff |  | [ ] |
| IN | 0x428 | 0x0 | 0xff |  | [ ] |
| INTFLAGS | 0x429 | 0x0 | 0xff | INT | [ ] |
| PORTCTRL | 0x42a | 0x0 | 0xff | SRL | [ ] |
| PIN0CTRL | 0x430 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN1CTRL | 0x431 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN2CTRL | 0x432 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN3CTRL | 0x433 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN4CTRL | 0x434 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN5CTRL | 0x435 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN6CTRL | 0x436 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN7CTRL | 0x437 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |

### PORTC (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| DIR | 0x440 | 0x0 | 0xff |  | [ ] |
| DIRSET | 0x441 | 0x0 | 0xff |  | [ ] |
| DIRCLR | 0x442 | 0x0 | 0xff |  | [ ] |
| DIRTGL | 0x443 | 0x0 | 0xff |  | [ ] |
| OUT | 0x444 | 0x0 | 0xff |  | [ ] |
| OUTSET | 0x445 | 0x0 | 0xff |  | [ ] |
| OUTCLR | 0x446 | 0x0 | 0xff |  | [ ] |
| OUTTGL | 0x447 | 0x0 | 0xff |  | [ ] |
| IN | 0x448 | 0x0 | 0xff |  | [ ] |
| INTFLAGS | 0x449 | 0x0 | 0xff | INT | [ ] |
| PORTCTRL | 0x44a | 0x0 | 0xff | SRL | [ ] |
| PIN0CTRL | 0x450 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN1CTRL | 0x451 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN2CTRL | 0x452 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN3CTRL | 0x453 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN4CTRL | 0x454 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN5CTRL | 0x455 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN6CTRL | 0x456 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN7CTRL | 0x457 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |

### PORTD (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| DIR | 0x460 | 0x0 | 0xff |  | [ ] |
| DIRSET | 0x461 | 0x0 | 0xff |  | [ ] |
| DIRCLR | 0x462 | 0x0 | 0xff |  | [ ] |
| DIRTGL | 0x463 | 0x0 | 0xff |  | [ ] |
| OUT | 0x464 | 0x0 | 0xff |  | [ ] |
| OUTSET | 0x465 | 0x0 | 0xff |  | [ ] |
| OUTCLR | 0x466 | 0x0 | 0xff |  | [ ] |
| OUTTGL | 0x467 | 0x0 | 0xff |  | [ ] |
| IN | 0x468 | 0x0 | 0xff |  | [ ] |
| INTFLAGS | 0x469 | 0x0 | 0xff | INT | [ ] |
| PORTCTRL | 0x46a | 0x0 | 0xff | SRL | [ ] |
| PIN0CTRL | 0x470 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN1CTRL | 0x471 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN2CTRL | 0x472 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN3CTRL | 0x473 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN4CTRL | 0x474 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN5CTRL | 0x475 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN6CTRL | 0x476 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN7CTRL | 0x477 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |

### PORTE (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| DIR | 0x480 | 0x0 | 0xff |  | [ ] |
| DIRSET | 0x481 | 0x0 | 0xff |  | [ ] |
| DIRCLR | 0x482 | 0x0 | 0xff |  | [ ] |
| DIRTGL | 0x483 | 0x0 | 0xff |  | [ ] |
| OUT | 0x484 | 0x0 | 0xff |  | [ ] |
| OUTSET | 0x485 | 0x0 | 0xff |  | [ ] |
| OUTCLR | 0x486 | 0x0 | 0xff |  | [ ] |
| OUTTGL | 0x487 | 0x0 | 0xff |  | [ ] |
| IN | 0x488 | 0x0 | 0xff |  | [ ] |
| INTFLAGS | 0x489 | 0x0 | 0xff | INT | [ ] |
| PORTCTRL | 0x48a | 0x0 | 0xff | SRL | [ ] |
| PIN0CTRL | 0x490 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN1CTRL | 0x491 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN2CTRL | 0x492 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN3CTRL | 0x493 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN4CTRL | 0x494 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN5CTRL | 0x495 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN6CTRL | 0x496 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN7CTRL | 0x497 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |

### PORTF (PORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| DIR | 0x4a0 | 0x0 | 0xff |  | [ ] |
| DIRSET | 0x4a1 | 0x0 | 0xff |  | [ ] |
| DIRCLR | 0x4a2 | 0x0 | 0xff |  | [ ] |
| DIRTGL | 0x4a3 | 0x0 | 0xff |  | [ ] |
| OUT | 0x4a4 | 0x0 | 0xff |  | [ ] |
| OUTSET | 0x4a5 | 0x0 | 0xff |  | [ ] |
| OUTCLR | 0x4a6 | 0x0 | 0xff |  | [ ] |
| OUTTGL | 0x4a7 | 0x0 | 0xff |  | [ ] |
| IN | 0x4a8 | 0x0 | 0xff |  | [ ] |
| INTFLAGS | 0x4a9 | 0x0 | 0xff | INT | [ ] |
| PORTCTRL | 0x4aa | 0x0 | 0xff | SRL | [ ] |
| PIN0CTRL | 0x4b0 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN1CTRL | 0x4b1 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN2CTRL | 0x4b2 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN3CTRL | 0x4b3 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN4CTRL | 0x4b4 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN5CTRL | 0x4b5 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN6CTRL | 0x4b6 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |
| PIN7CTRL | 0x4b7 | 0x0 | 0xff | ISC, PULLUPEN, INVEN | [ ] |

### PORTMUX (PORTMUX)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| EVSYSROUTEA | 0x5e0 | 0x0 | 0xff | EVOUTA, EVOUTB, EVOUTC, EVOUTD, EVOUTE, EVOUTF | [ ] |
| CCLROUTEA | 0x5e1 | 0x0 | 0xff | LUT0, LUT1, LUT2, LUT3 | [ ] |
| USARTROUTEA | 0x5e2 | 0x0 | 0xff | USART0, USART1, USART2, USART3 | [ ] |
| TWISPIROUTEA | 0x5e3 | 0x0 | 0xff | SPI0, TWI0 | [ ] |
| TCAROUTEA | 0x5e4 | 0x0 | 0xff | TCA0 | [ ] |
| TCBROUTEA | 0x5e5 | 0x0 | 0xff | TCB0, TCB1, TCB2, TCB3 | [ ] |

### RSTCTRL (RSTCTRL)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| RSTFR | 0x40 | 0x0 | 0xff | PORF, BORF, EXTRF, WDRF, SWRF, UPDIRF | [ ] |
| SWRR | 0x41 | 0x0 | 0xff | SWRE | [ ] |

### RTC (RTC)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| CTRLA | 0x140 | 0x0 | 0xff | RTCEN, CORREN, PRESCALER, RUNSTDBY | [ ] |
| STATUS | 0x141 | 0x0 | 0xff | CTRLABUSY, CNTBUSY, PERBUSY, CMPBUSY | [ ] |
| INTCTRL | 0x142 | 0x0 | 0xff | OVF, CMP | [ ] |
| INTFLAGS | 0x143 | 0x0 | 0xff | OVF, CMP | [ ] |
| TEMP | 0x144 | 0x0 | 0xff |  | [ ] |
| DBGCTRL | 0x145 | 0x0 | 0xff | DBGRUN | [ ] |
| CALIB | 0x146 | 0x0 | 0xff | ERROR, SIGN | [ ] |
| CLKSEL | 0x147 | 0x0 | 0xff | CLKSEL | [ ] |
| CNT | 0x148 | 0x0 | 0xff |  | [ ] |
| PER | 0x14a | 0xffff | 0xff |  | [ ] |
| CMP | 0x14c | 0x0 | 0xff |  | [ ] |
| PITCTRLA | 0x150 | 0x0 | 0xff | PITEN, PERIOD | [ ] |
| PITSTATUS | 0x151 | 0x0 | 0xff | CTRLBUSY | [ ] |
| PITINTCTRL | 0x152 | 0x0 | 0xff | PI | [ ] |
| PITINTFLAGS | 0x153 | 0x0 | 0xff | PI | [ ] |
| PITDBGCTRL | 0x155 | 0x0 | 0xff | DBGRUN | [ ] |

### SIGROW (SIGROW)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| DEVICEID0 | 0x1100 | 0x0 | 0xff |  | [ ] |
| DEVICEID1 | 0x1101 | 0x0 | 0xff |  | [ ] |
| DEVICEID2 | 0x1102 | 0x0 | 0xff |  | [ ] |
| SERNUM0 | 0x1103 | 0x0 | 0xff |  | [ ] |
| SERNUM1 | 0x1104 | 0x0 | 0xff |  | [ ] |
| SERNUM2 | 0x1105 | 0x0 | 0xff |  | [ ] |
| SERNUM3 | 0x1106 | 0x0 | 0xff |  | [ ] |
| SERNUM4 | 0x1107 | 0x0 | 0xff |  | [ ] |
| SERNUM5 | 0x1108 | 0x0 | 0xff |  | [ ] |
| SERNUM6 | 0x1109 | 0x0 | 0xff |  | [ ] |
| SERNUM7 | 0x110a | 0x0 | 0xff |  | [ ] |
| SERNUM8 | 0x110b | 0x0 | 0xff |  | [ ] |
| SERNUM9 | 0x110c | 0x0 | 0xff |  | [ ] |
| OSCCAL32K | 0x1114 | 0x0 | 0xff |  | [ ] |
| OSCCAL16M0 | 0x1118 | 0x0 | 0xff |  | [ ] |
| OSCCAL16M1 | 0x1119 | 0x0 | 0xff |  | [ ] |
| OSCCAL20M0 | 0x111a | 0x0 | 0xff |  | [ ] |
| OSCCAL20M1 | 0x111b | 0x0 | 0xff |  | [ ] |
| TEMPSENSE0 | 0x1120 | 0x0 | 0xff |  | [ ] |
| TEMPSENSE1 | 0x1121 | 0x0 | 0xff |  | [ ] |
| OSC16ERR3V | 0x1122 | 0x0 | 0xff |  | [ ] |
| OSC16ERR5V | 0x1123 | 0x0 | 0xff |  | [ ] |
| OSC20ERR3V | 0x1124 | 0x0 | 0xff |  | [ ] |
| OSC20ERR5V | 0x1125 | 0x0 | 0xff |  | [ ] |
| CHECKSUM1 | 0x112f | 0x0 | 0xff |  | [ ] |

### SLPCTRL (SLPCTRL)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| CTRLA | 0x50 | 0x0 | 0xff | SEN, SMODE | [ ] |

### SPI0 (SPI)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| CTRLA | 0x8c0 | 0x0 | 0xff | ENABLE, PRESC, CLK2X, MASTER, DORD | [ ] |
| CTRLB | 0x8c1 | 0x0 | 0xff | MODE, SSD, BUFWR, BUFEN | [ ] |
| INTCTRL | 0x8c2 | 0x0 | 0xff | IE, SSIE, DREIE, TXCIE, RXCIE | [ ] |
| INTFLAGS | 0x8c3 | 0x0 | 0xff | BUFOVF, SSIF, DREIF, TXCIF, RXCIF, WRCOL, IF | [ ] |
| DATA | 0x8c4 | 0x0 | 0xff |  | [ ] |

### SYSCFG (SYSCFG)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| REVID | 0xf01 | 0x0 | 0xff |  | [ ] |
| EXTBRK | 0xf02 | 0x0 | 0xff | ENEXTBRK | [ ] |
| OCDM | 0xf18 | 0x0 | 0xff |  | [ ] |
| OCDMS | 0xf19 | 0x0 | 0xff | OCDMR | [ ] |

### TCA0 (TCA)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| CTRLA | 0xa00 | 0x0 | 0xff | ENABLE, CLKSEL | [ ] |
| CTRLB | 0xa01 | 0x0 | 0xff | LCMP0EN, LCMP1EN, LCMP2EN, HCMP0EN, HCMP1EN, HCMP2EN | [ ] |
| CTRLC | 0xa02 | 0x0 | 0xff | LCMP0OV, LCMP1OV, LCMP2OV, HCMP0OV, HCMP1OV, HCMP2OV | [ ] |
| CTRLD | 0xa03 | 0x0 | 0xff | SPLITM | [ ] |
| CTRLECLR | 0xa04 | 0x0 | 0xff | CMD | [ ] |
| CTRLESET | 0xa05 | 0x0 | 0xff | CMD | [ ] |
| CTRLFCLR | 0xa06 | 0x0 | 0xff | PERBV, CMP0BV, CMP1BV, CMP2BV | [ ] |
| CTRLFSET | 0xa07 | 0x0 | 0xff | PERBV, CMP0BV, CMP1BV, CMP2BV | [ ] |
| EVCTRL | 0xa09 | 0x0 | 0xff | CNTEI, EVACT | [ ] |
| INTCTRL | 0xa0a | 0x0 | 0xff | LUNF, HUNF, LCMP0, LCMP1, LCMP2 | [ ] |
| INTFLAGS | 0xa0b | 0x0 | 0xff | LUNF, HUNF, LCMP0, LCMP1, LCMP2 | [ ] |
| DBGCTRL | 0xa0e | 0x0 | 0xff | DBGRUN | [ ] |
| TEMP | 0xa0f | 0x0 | 0xff |  | [ ] |
| CNT | 0xa20 | 0x0 | 0xff |  | [ ] |
| PER | 0xa26 | 0xffff | 0xff |  | [ ] |
| CMP0 | 0xa28 | 0x0 | 0xff |  | [ ] |
| CMP1 | 0xa2a | 0x0 | 0xff |  | [ ] |
| CMP2 | 0xa2c | 0x0 | 0xff |  | [ ] |
| PERBUF | 0xa36 | 0xffff | 0xff |  | [ ] |
| CMP0BUF | 0xa38 | 0x0 | 0xff |  | [ ] |
| CMP1BUF | 0xa3a | 0x0 | 0xff |  | [ ] |
| CMP2BUF | 0xa3c | 0x0 | 0xff |  | [ ] |
| LCNT | 0xa20 | 0x0 | 0xff |  | [ ] |
| HCNT | 0xa21 | 0x0 | 0xff |  | [ ] |
| LPER | 0xa26 | 0x0 | 0xff |  | [ ] |
| HPER | 0xa27 | 0x0 | 0xff |  | [ ] |
| LCMP0 | 0xa28 | 0x0 | 0xff |  | [ ] |
| HCMP0 | 0xa29 | 0x0 | 0xff |  | [ ] |
| LCMP1 | 0xa2a | 0x0 | 0xff |  | [ ] |
| HCMP1 | 0xa2b | 0x0 | 0xff |  | [ ] |
| LCMP2 | 0xa2c | 0x0 | 0xff |  | [ ] |
| HCMP2 | 0xa2d | 0x0 | 0xff |  | [ ] |

### TCB0 (TCB)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| CTRLA | 0xa80 | 0x0 | 0xff | ENABLE, CLKSEL, SYNCUPD, RUNSTDBY | [ ] |
| CTRLB | 0xa81 | 0x0 | 0xff | CNTMODE, CCMPEN, CCMPINIT, ASYNC | [ ] |
| EVCTRL | 0xa84 | 0x0 | 0xff | CAPTEI, EDGE, FILTER | [ ] |
| INTCTRL | 0xa85 | 0x0 | 0xff | CAPT | [ ] |
| INTFLAGS | 0xa86 | 0x0 | 0xff | CAPT | [ ] |
| STATUS | 0xa87 | 0x0 | 0xff | RUN | [ ] |
| DBGCTRL | 0xa88 | 0x0 | 0xff | DBGRUN | [ ] |
| TEMP | 0xa89 | 0x0 | 0xff |  | [ ] |
| CNT | 0xa8a | 0x0 | 0xff |  | [ ] |
| CCMP | 0xa8c | 0x0 | 0xff |  | [ ] |

### TCB1 (TCB)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| CTRLA | 0xa90 | 0x0 | 0xff | ENABLE, CLKSEL, SYNCUPD, RUNSTDBY | [ ] |
| CTRLB | 0xa91 | 0x0 | 0xff | CNTMODE, CCMPEN, CCMPINIT, ASYNC | [ ] |
| EVCTRL | 0xa94 | 0x0 | 0xff | CAPTEI, EDGE, FILTER | [ ] |
| INTCTRL | 0xa95 | 0x0 | 0xff | CAPT | [ ] |
| INTFLAGS | 0xa96 | 0x0 | 0xff | CAPT | [ ] |
| STATUS | 0xa97 | 0x0 | 0xff | RUN | [ ] |
| DBGCTRL | 0xa98 | 0x0 | 0xff | DBGRUN | [ ] |
| TEMP | 0xa99 | 0x0 | 0xff |  | [ ] |
| CNT | 0xa9a | 0x0 | 0xff |  | [ ] |
| CCMP | 0xa9c | 0x0 | 0xff |  | [ ] |

### TCB2 (TCB)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| CTRLA | 0xaa0 | 0x0 | 0xff | ENABLE, CLKSEL, SYNCUPD, RUNSTDBY | [ ] |
| CTRLB | 0xaa1 | 0x0 | 0xff | CNTMODE, CCMPEN, CCMPINIT, ASYNC | [ ] |
| EVCTRL | 0xaa4 | 0x0 | 0xff | CAPTEI, EDGE, FILTER | [ ] |
| INTCTRL | 0xaa5 | 0x0 | 0xff | CAPT | [ ] |
| INTFLAGS | 0xaa6 | 0x0 | 0xff | CAPT | [ ] |
| STATUS | 0xaa7 | 0x0 | 0xff | RUN | [ ] |
| DBGCTRL | 0xaa8 | 0x0 | 0xff | DBGRUN | [ ] |
| TEMP | 0xaa9 | 0x0 | 0xff |  | [ ] |
| CNT | 0xaaa | 0x0 | 0xff |  | [ ] |
| CCMP | 0xaac | 0x0 | 0xff |  | [ ] |

### TCB3 (TCB)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| CTRLA | 0xab0 | 0x0 | 0xff | ENABLE, CLKSEL, SYNCUPD, RUNSTDBY | [ ] |
| CTRLB | 0xab1 | 0x0 | 0xff | CNTMODE, CCMPEN, CCMPINIT, ASYNC | [ ] |
| EVCTRL | 0xab4 | 0x0 | 0xff | CAPTEI, EDGE, FILTER | [ ] |
| INTCTRL | 0xab5 | 0x0 | 0xff | CAPT | [ ] |
| INTFLAGS | 0xab6 | 0x0 | 0xff | CAPT | [ ] |
| STATUS | 0xab7 | 0x0 | 0xff | RUN | [ ] |
| DBGCTRL | 0xab8 | 0x0 | 0xff | DBGRUN | [ ] |
| TEMP | 0xab9 | 0x0 | 0xff |  | [ ] |
| CNT | 0xaba | 0x0 | 0xff |  | [ ] |
| CCMP | 0xabc | 0x0 | 0xff |  | [ ] |

### TWI0 (TWI)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| CTRLA | 0x8a0 | 0x0 | 0xff | FMPEN, SDAHOLD, SDASETUP | [ ] |
| DUALCTRL | 0x8a1 | 0x0 | 0xff | ENABLE, FMPEN, SDAHOLD | [ ] |
| DBGCTRL | 0x8a2 | 0x0 | 0xff | DBGRUN | [ ] |
| MCTRLA | 0x8a3 | 0x0 | 0xff | ENABLE, SMEN, TIMEOUT, QCEN, WIEN, RIEN | [ ] |
| MCTRLB | 0x8a4 | 0x0 | 0xff | MCMD, ACKACT, FLUSH | [ ] |
| MSTATUS | 0x8a5 | 0x0 | 0xff | BUSSTATE, BUSERR, ARBLOST, RXACK, CLKHOLD, WIF, RIF | [ ] |
| MBAUD | 0x8a6 | 0x0 | 0xff |  | [ ] |
| MADDR | 0x8a7 | 0x0 | 0xff |  | [ ] |
| MDATA | 0x8a8 | 0x0 | 0xff |  | [ ] |
| SCTRLA | 0x8a9 | 0x0 | 0xff | ENABLE, SMEN, PMEN, PIEN, APIEN, DIEN | [ ] |
| SCTRLB | 0x8aa | 0x0 | 0xff | SCMD, ACKACT | [ ] |
| SSTATUS | 0x8ab | 0x0 | 0xff | AP, DIR, BUSERR, COLL, RXACK, CLKHOLD, APIF, DIF | [ ] |
| SADDR | 0x8ac | 0x0 | 0xff |  | [ ] |
| SDATA | 0x8ad | 0x0 | 0xff |  | [ ] |
| SADDRMASK | 0x8ae | 0x0 | 0xff | ADDREN, ADDRMASK | [ ] |

### USART0 (USART)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| RXDATAL | 0x800 | 0x0 | 0xff | DATA | [ ] |
| RXDATAH | 0x801 | 0x0 | 0xff | DATA8, PERR, FERR, BUFOVF, RXCIF | [ ] |
| TXDATAL | 0x802 | 0x0 | 0xff | DATA | [ ] |
| TXDATAH | 0x803 | 0x0 | 0xff | DATA8 | [ ] |
| STATUS | 0x804 | 0x0 | 0xff | WFB, BDF, ISFIF, RXSIF, DREIF, TXCIF, RXCIF | [ ] |
| CTRLA | 0x805 | 0x0 | 0xff | RS485, ABEIE, LBME, RXSIE, DREIE, TXCIE, RXCIE | [ ] |
| CTRLB | 0x806 | 0x0 | 0xff | MPCM, RXMODE, ODME, SFDEN, TXEN, RXEN | [ ] |
| CTRLC | 0x807 | 0x3 | 0xff | CMODE, UCPHA, UDORD, CHSIZE, SBMODE, PMODE | [ ] |
| BAUD | 0x808 | 0x0 | 0xff |  | [ ] |
| CTRLD | 0x80a | 0x0 | 0xff | ABW | [ ] |
| DBGCTRL | 0x80b | 0x0 | 0xff | DBGRUN | [ ] |
| EVCTRL | 0x80c | 0x0 | 0xff | IREI | [ ] |
| TXPLCTRL | 0x80d | 0x0 | 0xff | TXPL | [ ] |
| RXPLCTRL | 0x80e | 0x0 | 0xff | RXPL | [ ] |

### USART1 (USART)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| RXDATAL | 0x820 | 0x0 | 0xff | DATA | [ ] |
| RXDATAH | 0x821 | 0x0 | 0xff | DATA8, PERR, FERR, BUFOVF, RXCIF | [ ] |
| TXDATAL | 0x822 | 0x0 | 0xff | DATA | [ ] |
| TXDATAH | 0x823 | 0x0 | 0xff | DATA8 | [ ] |
| STATUS | 0x824 | 0x0 | 0xff | WFB, BDF, ISFIF, RXSIF, DREIF, TXCIF, RXCIF | [ ] |
| CTRLA | 0x825 | 0x0 | 0xff | RS485, ABEIE, LBME, RXSIE, DREIE, TXCIE, RXCIE | [ ] |
| CTRLB | 0x826 | 0x0 | 0xff | MPCM, RXMODE, ODME, SFDEN, TXEN, RXEN | [ ] |
| CTRLC | 0x827 | 0x3 | 0xff | CMODE, UCPHA, UDORD, CHSIZE, SBMODE, PMODE | [ ] |
| BAUD | 0x828 | 0x0 | 0xff |  | [ ] |
| CTRLD | 0x82a | 0x0 | 0xff | ABW | [ ] |
| DBGCTRL | 0x82b | 0x0 | 0xff | DBGRUN | [ ] |
| EVCTRL | 0x82c | 0x0 | 0xff | IREI | [ ] |
| TXPLCTRL | 0x82d | 0x0 | 0xff | TXPL | [ ] |
| RXPLCTRL | 0x82e | 0x0 | 0xff | RXPL | [ ] |

### USART2 (USART)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| RXDATAL | 0x840 | 0x0 | 0xff | DATA | [ ] |
| RXDATAH | 0x841 | 0x0 | 0xff | DATA8, PERR, FERR, BUFOVF, RXCIF | [ ] |
| TXDATAL | 0x842 | 0x0 | 0xff | DATA | [ ] |
| TXDATAH | 0x843 | 0x0 | 0xff | DATA8 | [ ] |
| STATUS | 0x844 | 0x0 | 0xff | WFB, BDF, ISFIF, RXSIF, DREIF, TXCIF, RXCIF | [ ] |
| CTRLA | 0x845 | 0x0 | 0xff | RS485, ABEIE, LBME, RXSIE, DREIE, TXCIE, RXCIE | [ ] |
| CTRLB | 0x846 | 0x0 | 0xff | MPCM, RXMODE, ODME, SFDEN, TXEN, RXEN | [ ] |
| CTRLC | 0x847 | 0x3 | 0xff | CMODE, UCPHA, UDORD, CHSIZE, SBMODE, PMODE | [ ] |
| BAUD | 0x848 | 0x0 | 0xff |  | [ ] |
| CTRLD | 0x84a | 0x0 | 0xff | ABW | [ ] |
| DBGCTRL | 0x84b | 0x0 | 0xff | DBGRUN | [ ] |
| EVCTRL | 0x84c | 0x0 | 0xff | IREI | [ ] |
| TXPLCTRL | 0x84d | 0x0 | 0xff | TXPL | [ ] |
| RXPLCTRL | 0x84e | 0x0 | 0xff | RXPL | [ ] |

### USART3 (USART)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| RXDATAL | 0x860 | 0x0 | 0xff | DATA | [ ] |
| RXDATAH | 0x861 | 0x0 | 0xff | DATA8, PERR, FERR, BUFOVF, RXCIF | [ ] |
| TXDATAL | 0x862 | 0x0 | 0xff | DATA | [ ] |
| TXDATAH | 0x863 | 0x0 | 0xff | DATA8 | [ ] |
| STATUS | 0x864 | 0x0 | 0xff | WFB, BDF, ISFIF, RXSIF, DREIF, TXCIF, RXCIF | [ ] |
| CTRLA | 0x865 | 0x0 | 0xff | RS485, ABEIE, LBME, RXSIE, DREIE, TXCIE, RXCIE | [ ] |
| CTRLB | 0x866 | 0x0 | 0xff | MPCM, RXMODE, ODME, SFDEN, TXEN, RXEN | [ ] |
| CTRLC | 0x867 | 0x3 | 0xff | CMODE, UCPHA, UDORD, CHSIZE, SBMODE, PMODE | [ ] |
| BAUD | 0x868 | 0x0 | 0xff |  | [ ] |
| CTRLD | 0x86a | 0x0 | 0xff | ABW | [ ] |
| DBGCTRL | 0x86b | 0x0 | 0xff | DBGRUN | [ ] |
| EVCTRL | 0x86c | 0x0 | 0xff | IREI | [ ] |
| TXPLCTRL | 0x86d | 0x0 | 0xff | TXPL | [ ] |
| RXPLCTRL | 0x86e | 0x0 | 0xff | RXPL | [ ] |

### USERROW (USERROW)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| USERROW0 | 0x1300 | 0x0 | 0xff |  | [ ] |
| USERROW1 | 0x1301 | 0x0 | 0xff |  | [ ] |
| USERROW2 | 0x1302 | 0x0 | 0xff |  | [ ] |
| USERROW3 | 0x1303 | 0x0 | 0xff |  | [ ] |
| USERROW4 | 0x1304 | 0x0 | 0xff |  | [ ] |
| USERROW5 | 0x1305 | 0x0 | 0xff |  | [ ] |
| USERROW6 | 0x1306 | 0x0 | 0xff |  | [ ] |
| USERROW7 | 0x1307 | 0x0 | 0xff |  | [ ] |
| USERROW8 | 0x1308 | 0x0 | 0xff |  | [ ] |
| USERROW9 | 0x1309 | 0x0 | 0xff |  | [ ] |
| USERROW10 | 0x130a | 0x0 | 0xff |  | [ ] |
| USERROW11 | 0x130b | 0x0 | 0xff |  | [ ] |
| USERROW12 | 0x130c | 0x0 | 0xff |  | [ ] |
| USERROW13 | 0x130d | 0x0 | 0xff |  | [ ] |
| USERROW14 | 0x130e | 0x0 | 0xff |  | [ ] |
| USERROW15 | 0x130f | 0x0 | 0xff |  | [ ] |
| USERROW16 | 0x1310 | 0x0 | 0xff |  | [ ] |
| USERROW17 | 0x1311 | 0x0 | 0xff |  | [ ] |
| USERROW18 | 0x1312 | 0x0 | 0xff |  | [ ] |
| USERROW19 | 0x1313 | 0x0 | 0xff |  | [ ] |
| USERROW20 | 0x1314 | 0x0 | 0xff |  | [ ] |
| USERROW21 | 0x1315 | 0x0 | 0xff |  | [ ] |
| USERROW22 | 0x1316 | 0x0 | 0xff |  | [ ] |
| USERROW23 | 0x1317 | 0x0 | 0xff |  | [ ] |
| USERROW24 | 0x1318 | 0x0 | 0xff |  | [ ] |
| USERROW25 | 0x1319 | 0x0 | 0xff |  | [ ] |
| USERROW26 | 0x131a | 0x0 | 0xff |  | [ ] |
| USERROW27 | 0x131b | 0x0 | 0xff |  | [ ] |
| USERROW28 | 0x131c | 0x0 | 0xff |  | [ ] |
| USERROW29 | 0x131d | 0x0 | 0xff |  | [ ] |
| USERROW30 | 0x131e | 0x0 | 0xff |  | [ ] |
| USERROW31 | 0x131f | 0x0 | 0xff |  | [ ] |
| USERROW32 | 0x1320 | 0x0 | 0xff |  | [ ] |
| USERROW33 | 0x1321 | 0x0 | 0xff |  | [ ] |
| USERROW34 | 0x1322 | 0x0 | 0xff |  | [ ] |
| USERROW35 | 0x1323 | 0x0 | 0xff |  | [ ] |
| USERROW36 | 0x1324 | 0x0 | 0xff |  | [ ] |
| USERROW37 | 0x1325 | 0x0 | 0xff |  | [ ] |
| USERROW38 | 0x1326 | 0x0 | 0xff |  | [ ] |
| USERROW39 | 0x1327 | 0x0 | 0xff |  | [ ] |
| USERROW40 | 0x1328 | 0x0 | 0xff |  | [ ] |
| USERROW41 | 0x1329 | 0x0 | 0xff |  | [ ] |
| USERROW42 | 0x132a | 0x0 | 0xff |  | [ ] |
| USERROW43 | 0x132b | 0x0 | 0xff |  | [ ] |
| USERROW44 | 0x132c | 0x0 | 0xff |  | [ ] |
| USERROW45 | 0x132d | 0x0 | 0xff |  | [ ] |
| USERROW46 | 0x132e | 0x0 | 0xff |  | [ ] |
| USERROW47 | 0x132f | 0x0 | 0xff |  | [ ] |
| USERROW48 | 0x1330 | 0x0 | 0xff |  | [ ] |
| USERROW49 | 0x1331 | 0x0 | 0xff |  | [ ] |
| USERROW50 | 0x1332 | 0x0 | 0xff |  | [ ] |
| USERROW51 | 0x1333 | 0x0 | 0xff |  | [ ] |
| USERROW52 | 0x1334 | 0x0 | 0xff |  | [ ] |
| USERROW53 | 0x1335 | 0x0 | 0xff |  | [ ] |
| USERROW54 | 0x1336 | 0x0 | 0xff |  | [ ] |
| USERROW55 | 0x1337 | 0x0 | 0xff |  | [ ] |
| USERROW56 | 0x1338 | 0x0 | 0xff |  | [ ] |
| USERROW57 | 0x1339 | 0x0 | 0xff |  | [ ] |
| USERROW58 | 0x133a | 0x0 | 0xff |  | [ ] |
| USERROW59 | 0x133b | 0x0 | 0xff |  | [ ] |
| USERROW60 | 0x133c | 0x0 | 0xff |  | [ ] |
| USERROW61 | 0x133d | 0x0 | 0xff |  | [ ] |
| USERROW62 | 0x133e | 0x0 | 0xff |  | [ ] |
| USERROW63 | 0x133f | 0x0 | 0xff |  | [ ] |

### VPORTA (VPORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| DIR | 0x0 | 0x0 | 0xff |  | [ ] |
| OUT | 0x1 | 0x0 | 0xff |  | [ ] |
| IN | 0x2 | 0x0 | 0xff |  | [ ] |
| INTFLAGS | 0x3 | 0x0 | 0xff | INT | [ ] |

### VPORTB (VPORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| DIR | 0x4 | 0x0 | 0xff |  | [ ] |
| OUT | 0x5 | 0x0 | 0xff |  | [ ] |
| IN | 0x6 | 0x0 | 0xff |  | [ ] |
| INTFLAGS | 0x7 | 0x0 | 0xff | INT | [ ] |

### VPORTC (VPORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| DIR | 0x8 | 0x0 | 0xff |  | [ ] |
| OUT | 0x9 | 0x0 | 0xff |  | [ ] |
| IN | 0xa | 0x0 | 0xff |  | [ ] |
| INTFLAGS | 0xb | 0x0 | 0xff | INT | [ ] |

### VPORTD (VPORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| DIR | 0xc | 0x0 | 0xff |  | [ ] |
| OUT | 0xd | 0x0 | 0xff |  | [ ] |
| IN | 0xe | 0x0 | 0xff |  | [ ] |
| INTFLAGS | 0xf | 0x0 | 0xff | INT | [ ] |

### VPORTE (VPORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| DIR | 0x10 | 0x0 | 0xff |  | [ ] |
| OUT | 0x11 | 0x0 | 0xff |  | [ ] |
| IN | 0x12 | 0x0 | 0xff |  | [ ] |
| INTFLAGS | 0x13 | 0x0 | 0xff | INT | [ ] |

### VPORTF (VPORT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| DIR | 0x14 | 0x0 | 0xff |  | [ ] |
| OUT | 0x15 | 0x0 | 0xff |  | [ ] |
| IN | 0x16 | 0x0 | 0xff |  | [ ] |
| INTFLAGS | 0x17 | 0x0 | 0xff | INT | [ ] |

### VREF (VREF)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| CTRLA | 0xa0 | 0x0 | 0xff | AC0REFSEL, ADC0REFSEL | [ ] |
| CTRLB | 0xa1 | 0x0 | 0xff | AC0REFEN, ADC0REFEN | [ ] |

### WDT (WDT)

| Register | Offset | Reset | Mask | Bits | Status |
| --- | --- | --- | --- | --- | --- |
| CTRLA | 0x100 | 0x0 | 0xff | PERIOD, WINDOW | [ ] |
| STATUS | 0x101 | 0x0 | 0xff | SYNCBUSY, LOCK | [ ] |

