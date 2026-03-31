/*
 * Header file for ATmegaS64M1
 *
 * Copyright (c) 2026 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#ifndef _AVR_ATMEGAS64M1_H_INCLUDED
#define _AVR_ATMEGAS64M1_H_INCLUDED

#ifndef _AVR_IO_H_
#  error "Include <avr/io.h> instead of this file."
#endif

#ifndef _AVR_IOXXX_H_
#  define _AVR_IOXXX_H_ "ioms64m1.h"
#else
#  error "Attempt to include more than one <avr/ioXXX.h> file."
#endif
/* Registers and associated bit numbers */

#define PINB    _SFR_IO8(0x03)
#define PINB7   7
#define PINB6   6
#define PINB5   5
#define PINB4   4
#define PINB3   3
#define PINB2   2
#define PINB1   1
#define PINB0   0

#define DDRB    _SFR_IO8(0x04)
#define DDRB7   7
// Inserted "DDB7" from "DDRB7" due to compatibility
#define DDB7    7
#define DDRB6   6
// Inserted "DDB6" from "DDRB6" due to compatibility
#define DDB6    6
#define DDRB5   5
// Inserted "DDB5" from "DDRB5" due to compatibility
#define DDB5    5
#define DDRB4   4
// Inserted "DDB4" from "DDRB4" due to compatibility
#define DDB4    4
#define DDRB3   3
// Inserted "DDB3" from "DDRB3" due to compatibility
#define DDB3    3
#define DDRB2   2
// Inserted "DDB2" from "DDRB2" due to compatibility
#define DDB2    2
#define DDRB1   1
// Inserted "DDB1" from "DDRB1" due to compatibility
#define DDB1    1
#define DDRB0   0
// Inserted "DDB0" from "DDRB0" due to compatibility
#define DDB0    0

#define PORTB   _SFR_IO8(0x05)
#define PORTB7  7
#define PORTB6  6
#define PORTB5  5
#define PORTB4  4
#define PORTB3  3
#define PORTB2  2
#define PORTB1  1
#define PORTB0  0

#define PINC    _SFR_IO8(0x06)
#define PINC7   7
#define PINC6   6
#define PINC5   5
#define PINC4   4
#define PINC3   3
#define PINC2   2
#define PINC1   1
#define PINC0   0

#define DDRC    _SFR_IO8(0x07)
#define DDRC7   7
// Inserted "DDC7" from "DDRC7" due to compatibility
#define DDC7    7
#define DDRC6   6
// Inserted "DDC6" from "DDRC6" due to compatibility
#define DDC6    6
#define DDRC5   5
// Inserted "DDC5" from "DDRC5" due to compatibility
#define DDC5    5
#define DDRC4   4
// Inserted "DDC4" from "DDRC4" due to compatibility
#define DDC4    4
#define DDRC3   3
// Inserted "DDC3" from "DDRC3" due to compatibility
#define DDC3    3
#define DDRC2   2
// Inserted "DDC2" from "DDRC2" due to compatibility
#define DDC2    2
#define DDRC1   1
// Inserted "DDC1" from "DDRC1" due to compatibility
#define DDC1    1
#define DDRC0   0
// Inserted "DDC0" from "DDRC0" due to compatibility
#define DDC0    0

#define PORTC   _SFR_IO8(0x08)
#define PORTC7  7
#define PORTC6  6
#define PORTC5  5
#define PORTC4  4
#define PORTC3  3
#define PORTC2  2
#define PORTC1  1
#define PORTC0  0

#define PIND    _SFR_IO8(0x09)
#define PIND7   7
#define PIND6   6
#define PIND5   5
#define PIND4   4
#define PIND3   3
#define PIND2   2
#define PIND1   1
#define PIND0   0

#define DDRD    _SFR_IO8(0x0A)
#define DDRD7   7
// Inserted "DDD7" from "DDRD7" due to compatibility
#define DDD7    7
#define DDRD6   6
// Inserted "DDD6" from "DDRD6" due to compatibility
#define DDD6    6
#define DDRD5   5
// Inserted "DDD5" from "DDRD5" due to compatibility
#define DDD5    5
#define DDRD4   4
// Inserted "DDD4" from "DDRD4" due to compatibility
#define DDD4    4
#define DDRD3   3
// Inserted "DDD3" from "DDRD3" due to compatibility
#define DDD3    3
#define DDRD2   2
// Inserted "DDD2" from "DDRD2" due to compatibility
#define DDD2    2
#define DDRD1   1
// Inserted "DDD1" from "DDRD1" due to compatibility
#define DDD1    1
#define DDRD0   0
// Inserted "DDD0" from "DDRD0" due to compatibility
#define DDD0    0

#define PORTD   _SFR_IO8(0x0B)
#define PORTD7  7
#define PORTD6  6
#define PORTD5  5
#define PORTD4  4
#define PORTD3  3
#define PORTD2  2
#define PORTD1  1
#define PORTD0  0

#define PINE    _SFR_IO8(0x0C)
#define PINE2   2
#define PINE1   1
#define PINE0   0

#define DDRE    _SFR_IO8(0x0D)
#define DDRE2   2
// Inserted "DDE2" from "DDRE2" due to compatibility
#define DDE2    2
#define DDRE1   1
// Inserted "DDE1" from "DDRE1" due to compatibility
#define DDE1    1
#define DDRE0   0
// Inserted "DDE0" from "DDRE0" due to compatibility
#define DDE0    0

#define PORTE   _SFR_IO8(0x0E)
#define PORTE2  2
#define PORTE1  1
#define PORTE0  0

/* Reserved [0x0F..0x14] */

#define TIFR0   _SFR_IO8(0x15)
#define TOV0    0
#define OCF0A   1
#define OCF0B   2

#define TIFR1   _SFR_IO8(0x16)
#define TOV1    0
#define OCF1A   1
#define OCF1B   2
#define ICF1    5

/* Reserved [0x17..0x18] */

#define GPIOR1  _SFR_IO8(0x19)
#define GPIOR10 0
#define GPIOR11 1
#define GPIOR12 2
#define GPIOR13 3
#define GPIOR14 4
#define GPIOR15 5
#define GPIOR16 6
#define GPIOR17 7

#define GPIOR2  _SFR_IO8(0x1A)
#define GPIOR20 0
#define GPIOR21 1
#define GPIOR22 2
#define GPIOR23 3
#define GPIOR24 4
#define GPIOR25 5
#define GPIOR26 6
#define GPIOR27 7

#define PCIFR   _SFR_IO8(0x1B)
#define PCIF0   0
#define PCIF1   1
#define PCIF2   2
#define PCIF3   3

#define EIFR    _SFR_IO8(0x1C)
#define INTF0   0
#define INTF1   1
#define INTF2   2
#define INTF3   3

#define EIMSK   _SFR_IO8(0x1D)
#define INT0    0
#define INT1    1
#define INT2    2
#define INT3    3

#define GPIOR0  _SFR_IO8(0x1E)
#define GPIOR00 0
#define GPIOR01 1
#define GPIOR02 2
#define GPIOR03 3
#define GPIOR04 4
#define GPIOR05 5
#define GPIOR06 6
#define GPIOR07 7

#define EECR    _SFR_IO8(0x1F)
#define EERE    0
#define EEWE    1
#define EEMWE   2
#define EERIE   3
#define EEPM0   4
#define EEPM1   5

#define EEDR    _SFR_IO8(0x20)

/* Combine EEARL and EEARH */
#define EEAR    _SFR_IO16(0x21)

#define EEARL   _SFR_IO8(0x21)
#define EEARH   _SFR_IO8(0x22)

#define GTCCR   _SFR_IO8(0x23)
#define PSRSYNC 0
#define ICPSEL1 6
#define TSM     7

#define TCCR0A  _SFR_IO8(0x24)
#define WGM00   0
#define WGM01   1
#define COM0B0  4
#define COM0B1  5
#define COM0A0  6
#define COM0A1  7

#define TCCR0B  _SFR_IO8(0x25)
#define CS00    0
#define CS01    1
#define CS02    2
#define WGM02   3
#define FOC0B   6
#define FOC0A   7

#define TCNT0   _SFR_IO8(0x26)

#define OCR0A   _SFR_IO8(0x27)

#define OCR0B   _SFR_IO8(0x28)

#define PLLCSR  _SFR_IO8(0x29)
#define PLOCK   0
#define PLLE    1
#define PLLF    2

/* Reserved [0x2A..0x2B] */

#define SPCR    _SFR_IO8(0x2C)
#define SPR0    0
#define SPR1    1
#define CPHA    2
#define CPOL    3
#define MSTR    4
#define DORD    5
#define SPE     6
#define SPIE    7

#define SPSR    _SFR_IO8(0x2D)
#define SPI2X   0
#define WCOL    6
#define SPIF    7

#define SPDR    _SFR_IO8(0x2E)

/* Reserved [0x2F] */

#define ACSR    _SFR_IO8(0x30)
#define AC0O    0
#define AC1O    1
#define AC2O    2
#define AC3O    3
#define AC0IF   4
#define AC1IF   5
#define AC2IF   6
#define AC3IF   7

/* Reserved [0x31..0x32] */

#define SMCR    _SFR_IO8(0x33)
#define SE      0
#define SM0     1
#define SM1     2
#define SM2     3

#define MCUSR   _SFR_IO8(0x34)
#define PORF    0
#define EXTRF   1
#define BORF    2
#define WDRF    3

#define MCUCR   _SFR_IO8(0x35)
#define IVCE    0
#define IVSEL   1
#define PUD     4
#define SPIPS   7

/* Reserved [0x36] */

#define SPMCSR  _SFR_IO8(0x37)
#define SPMEN   0
#define PGERS   1
#define PGWRT   2
#define BLBSET  3
#define RWWSRE  4
#define SIGRD   5
#define RWWSB   6
#define SPMIE   7

/* Reserved [0x38..0x3C] */

/* SP [0x3D..0x3E] */

/* SREG [0x3F] */

#define WDTCSR  _SFR_MEM8(0x60)
#define WDE     3
#define WDCE    4
#define WDP0    0
#define WDP1    1
#define WDP2    2
#define WDP3    5
#define WDIE    6
#define WDIF    7

#define CLKPR   _SFR_MEM8(0x61)
#define CLKPS0  0
#define CLKPS1  1
#define CLKPS2  2
#define CLKPS3  3
#define CLKPCE  7

/* Reserved [0x62..0x63] */

#define PRR     _SFR_MEM8(0x64)
#define PRADC   0
#define PRLIN   1
#define PRSPI   2
#define PRTIM0  3
#define PRTIM1  4
#define PRPSC   5
#define PRCAN   6

#define __AVR_HAVE_PRR	((1<<PRADC)|(1<<PRLIN)|(1<<PRSPI)|(1<<PRTIM0)|(1<<PRTIM1)|(1<<PRPSC)|(1<<PRCAN))
#define __AVR_HAVE_PRR_PRADC
#define __AVR_HAVE_PRR_PRLIN
#define __AVR_HAVE_PRR_PRSPI
#define __AVR_HAVE_PRR_PRTIM0
#define __AVR_HAVE_PRR_PRTIM1
#define __AVR_HAVE_PRR_PRPSC
#define __AVR_HAVE_PRR_PRCAN

/* Reserved [0x65] */

#define OSCCAL  _SFR_MEM8(0x66)
#define OSCCAL0 0
#define OSCCAL1 1
#define OSCCAL2 2
#define OSCCAL3 3
#define OSCCAL4 4
#define OSCCAL5 5
#define OSCCAL6 6
#define OSCCAL7 7

/* Reserved [0x67] */

#define PCICR   _SFR_MEM8(0x68)
#define PCIE0   0
#define PCIE1   1
#define PCIE2   2
#define PCIE3   3

#define EICRA   _SFR_MEM8(0x69)
#define ISC00   0
#define ISC01   1
#define ISC10   2
#define ISC11   3
#define ISC20   4
#define ISC21   5
#define ISC30   6
#define ISC31   7

#define PCMSK0  _SFR_MEM8(0x6A)
#define PCINT0  0
#define PCINT1  1
#define PCINT2  2
#define PCINT3  3
#define PCINT4  4
#define PCINT5  5
#define PCINT6  6
#define PCINT7  7

#define PCMSK1  _SFR_MEM8(0x6B)
#define PCINT8  0
#define PCINT9  1
#define PCINT10 2
#define PCINT11 3
#define PCINT12 4
#define PCINT13 5
#define PCINT14 6
#define PCINT15 7

#define PCMSK2  _SFR_MEM8(0x6C)
#define PCINT16 0
#define PCINT17 1
#define PCINT18 2
#define PCINT19 3
#define PCINT20 4
#define PCINT21 5
#define PCINT22 6
#define PCINT23 7

#define PCMSK3  _SFR_MEM8(0x6D)
#define PCINT24 0
#define PCINT25 1
#define PCINT26 2

#define TIMSK0  _SFR_MEM8(0x6E)
#define TOIE0   0
#define OCIE0A  1
#define OCIE0B  2

#define TIMSK1  _SFR_MEM8(0x6F)
#define TOIE1   0
#define OCIE1A  1
#define OCIE1B  2
#define ICIE1   5

/* Reserved [0x70..0x74] */

#define AMP0CSR _SFR_MEM8(0x75)
#define AMP0TS0 0
#define AMP0TS1 1
#define AMP0TS2 2
#define AMPCMP0 3
#define AMP0G0  4
#define AMP0G1  5
#define AMP0IS  6
#define AMP0EN  7

#define AMP1CSR _SFR_MEM8(0x76)
#define AMP1TS0 0
#define AMP1TS1 1
#define AMP1TS2 2
#define AMPCMP1 3
#define AMP1G0  4
#define AMP1G1  5
#define AMP1IS  6
#define AMP1EN  7

#define AMP2CSR _SFR_MEM8(0x77)
#define AMP2TS0 0
#define AMP2TS1 1
#define AMP2TS2 2
#define AMPCMP2 3
#define AMP2G0  4
#define AMP2G1  5
#define AMP2IS  6
#define AMP2EN  7

/* Combine ADCL and ADCH */
#ifndef __ASSEMBLER__
#define ADC     _SFR_MEM16(0x78)
#endif
#define ADCW    _SFR_MEM16(0x78)

#define ADCL    _SFR_MEM8(0x78)
#define ADCH    _SFR_MEM8(0x79)

#define ADCSRA  _SFR_MEM8(0x7A)
#define ADPS0   0
#define ADPS1   1
#define ADPS2   2
#define ADIE    3
#define ADIF    4
#define ADATE   5
#define ADSC    6
#define ADEN    7

#define ADCSRB  _SFR_MEM8(0x7B)
#define ADTS0   0
#define ADTS1   1
#define ADTS2   2
#define ADTS3   3
#define AREFEN  5
#define ISRCEN  6
#define ADHSM   7

#define ADMUX   _SFR_MEM8(0x7C)
#define MUX0    0
#define MUX1    1
#define MUX2    2
#define MUX3    3
#define MUX4    4
#define ADLAR   5
#define REFS0   6
#define REFS1   7

/* Reserved [0x7D] */

#define DIDR0   _SFR_MEM8(0x7E)
#define ADC0D   0
#define ADC1D   1
#define ADC2D   2
#define ADC3D   3
#define ADC4D   4
#define ADC5D   5
#define ADC6D   6
#define ADC7D   7

#define DIDR1   _SFR_MEM8(0x7F)
#define ADC8D   0
#define ADC9D   1
#define ADC10D  2
#define AMP0ND  3
#define AMP0PD  4
#define ACMP0D  5
#define AMP2PD  6

#define TCCR1A  _SFR_MEM8(0x80)
#define WGM10   0
#define WGM11   1
#define COM1B0  4
#define COM1B1  5
#define COM1A0  6
#define COM1A1  7

#define TCCR1B  _SFR_MEM8(0x81)
#define CS10    0
#define CS11    1
#define CS12    2
#define WGM12   3
#define WGM13   4
#define ICES1   6
#define ICNC1   7

#define TCCR1C  _SFR_MEM8(0x82)
#define FOC1B   6
#define FOC1A   7

/* Reserved [0x83] */

/* Combine TCNT1L and TCNT1H */
#define TCNT1   _SFR_MEM16(0x84)

#define TCNT1L  _SFR_MEM8(0x84)
#define TCNT1H  _SFR_MEM8(0x85)

/* Combine ICR1L and ICR1H */
#define ICR1    _SFR_MEM16(0x86)

#define ICR1L   _SFR_MEM8(0x86)
#define ICR1H   _SFR_MEM8(0x87)

/* Combine OCR1AL and OCR1AH */
#define OCR1A   _SFR_MEM16(0x88)

#define OCR1AL  _SFR_MEM8(0x88)
#define OCR1AH  _SFR_MEM8(0x89)

/* Combine OCR1BL and OCR1BH */
#define OCR1B   _SFR_MEM16(0x8A)

#define OCR1BL  _SFR_MEM8(0x8A)
#define OCR1BH  _SFR_MEM8(0x8B)

/* Reserved [0x8C..0x8F] */

#define DACON   _SFR_MEM8(0x90)
#define DAEN    0
#define DAOE    1
#define DALA    2
#define DATS0   4
#define DATS1   5
#define DATS2   6
#define DAATE   7

/* Combine DACL and DACH */
#define DAC     _SFR_MEM16(0x91)

#define DACL    _SFR_MEM8(0x91)
#define DAC0    0
#define DAC1    1
#define DAC2    2
#define DAC3    3
#define DAC4    4
#define DAC5    5
#define DAC6    6
#define DAC7    7
#define DACH    _SFR_MEM8(0x92)
#define DAC8    0
#define DAC9    1
#define DAC10   2
#define DAC11   3
#define DAC12   4
#define DAC13   5
#define DAC14   6
#define DAC15   7

/* Reserved [0x93] */

#define AC0CON  _SFR_MEM8(0x94)
#define AC0M0   0
#define AC0M1   1
#define AC0M2   2
#define ACCKSEL 3
#define AC0IS0  4
#define AC0IS1  5
#define AC0IE   6
#define AC0EN   7

#define AC1CON  _SFR_MEM8(0x95)
#define AC1M0   0
#define AC1M1   1
#define AC1M2   2
#define AC1ICE  3
#define AC1IS0  4
#define AC1IS1  5
#define AC1IE   6
#define AC1EN   7

#define AC2CON  _SFR_MEM8(0x96)
#define AC2M0   0
#define AC2M1   1
#define AC2M2   2
#define AC2IS0  4
#define AC2IS1  5
#define AC2IE   6
#define AC2EN   7

#define AC3CON  _SFR_MEM8(0x97)
#define AC3M0   0
#define AC3M1   1
#define AC3M2   2
#define AC3IS0  4
#define AC3IS1  5
#define AC3IE   6
#define AC3EN   7

/* Reserved [0x98..0x9F] */

/* Combine POCR0SAL and POCR0SAH */
#define POCR0SA _SFR_MEM16(0xA0)

#define POCR0SAL _SFR_MEM8(0xA0)
#define POCR0SAH _SFR_MEM8(0xA1)

/* Combine POCR0RAL and POCR0RAH */
#define POCR0RA _SFR_MEM16(0xA2)

#define POCR0RAL _SFR_MEM8(0xA2)
#define POCR0RAH _SFR_MEM8(0xA3)

/* Combine POCR0SBL and POCR0SBH */
#define POCR0SB _SFR_MEM16(0xA4)

#define POCR0SBL _SFR_MEM8(0xA4)
#define POCR0SBH _SFR_MEM8(0xA5)

/* Combine POCR1SAL and POCR1SAH */
#define POCR1SA _SFR_MEM16(0xA6)

#define POCR1SAL _SFR_MEM8(0xA6)
#define POCR1SAH _SFR_MEM8(0xA7)

/* Combine POCR1RAL and POCR1RAH */
#define POCR1RA _SFR_MEM16(0xA8)

#define POCR1RAL _SFR_MEM8(0xA8)
#define POCR1RAH _SFR_MEM8(0xA9)

/* Combine POCR1SBL and POCR1SBH */
#define POCR1SB _SFR_MEM16(0xAA)

#define POCR1SBL _SFR_MEM8(0xAA)
#define POCR1SBH _SFR_MEM8(0xAB)

/* Combine POCR2SAL and POCR2SAH */
#define POCR2SA _SFR_MEM16(0xAC)

#define POCR2SAL _SFR_MEM8(0xAC)
#define POCR2SAH _SFR_MEM8(0xAD)

/* Combine POCR2RAL and POCR2RAH */
#define POCR2RA _SFR_MEM16(0xAE)

#define POCR2RAL _SFR_MEM8(0xAE)
#define POCR2RAH _SFR_MEM8(0xAF)

/* Combine POCR2SBL and POCR2SBH */
#define POCR2SB _SFR_MEM16(0xB0)

#define POCR2SBL _SFR_MEM8(0xB0)
#define POCR2SBH _SFR_MEM8(0xB1)

/* Combine POCR_RBL and POCR_RBH */
#define POCR_RB _SFR_MEM16(0xB2)

#define POCR_RBL _SFR_MEM8(0xB2)
#define POCR_RBH _SFR_MEM8(0xB3)

#define PSYNC   _SFR_MEM8(0xB4)
#define PSYNC00 0
#define PSYNC01 1
#define PSYNC10 2
#define PSYNC11 3
#define PSYNC20 4
#define PSYNC21 5

#define PCNF    _SFR_MEM8(0xB5)
#define POPA    2
#define POPB    3
#define PMODE   4
#define PULOCK  5

#define POC     _SFR_MEM8(0xB6)
#define POEN0A  0
#define POEN0B  1
#define POEN1A  2
#define POEN1B  3
#define POEN2A  4
#define POEN2B  5

#define PCTL    _SFR_MEM8(0xB7)
#define PRUN    0
#define PCCYC   1
#define PCLKSEL 5
#define PPRE0   6
#define PPRE1   7

#define PMIC0   _SFR_MEM8(0xB8)
#define PRFM00  0
#define PRFM01  1
#define PRFM02  2
#define PAOC0   3
#define PFLTE0  4
#define PELEV0  5
#define PISEL0  6
#define POVEN0  7

#define PMIC1   _SFR_MEM8(0xB9)
#define PRFM10  0
#define PRFM11  1
#define PRFM12  2
#define PAOC1   3
#define PFLTE1  4
#define PELEV1  5
#define PISEL1  6
#define POVEN1  7

#define PMIC2   _SFR_MEM8(0xBA)
#define PRFM20  0
#define PRFM21  1
#define PRFM22  2
#define PAOC2   3
#define PFLTE2  4
#define PELEV2  5
#define PISEL2  6
#define POVEN2  7

#define PIM     _SFR_MEM8(0xBB)
#define PEOPE   0
#define PEVE0   1
#define PEVE1   2
#define PEVE2   3

#define PIFR    _SFR_MEM8(0xBC)
#define PEOP    0
#define PEV0    1
#define PEV1    2
#define PEV2    3

/* Reserved [0xBD..0xC7] */

#define LINCR   _SFR_MEM8(0xC8)
#define LCMD0   0
#define LCMD1   1
#define LCMD2   2
#define LENA    3
#define LCONF0  4
#define LCONF1  5
#define LIN13   6
#define LSWRES  7

#define LINSIR  _SFR_MEM8(0xC9)
#define LRXOK   0
#define LTXOK   1
#define LIDOK   2
#define LERR    3
#define LBUSY   4
#define LIDST0  5
#define LIDST1  6
#define LIDST2  7

#define LINENIR _SFR_MEM8(0xCA)
#define LENRXOK 0
#define LENTXOK 1
#define LENIDOK 2
#define LENERR  3

#define LINERR  _SFR_MEM8(0xCB)
#define LBERR   0
#define LCERR   1
#define LPERR   2
#define LSERR   3
#define LFERR   4
#define LOVERR  5
#define LTOERR  6
#define LABORT  7

#define LINBTR  _SFR_MEM8(0xCC)
#define LBT0    0
#define LBT1    1
#define LBT2    2
#define LBT3    3
#define LBT4    4
#define LBT5    5
#define LDISR   7

/* Combine LINBRRL and LINBRRH */
#define LINBRR  _SFR_MEM16(0xCD)

#define LINBRRL _SFR_MEM8(0xCD)
#define LDIV0   0
#define LDIV1   1
#define LDIV2   2
#define LDIV3   3
#define LDIV4   4
#define LDIV5   5
#define LDIV6   6
#define LDIV7   7
#define LINBRRH _SFR_MEM8(0xCE)
#define LDIV8   0
#define LDIV9   1
#define LDIV10  2
#define LDIV11  3

#define LINDLR  _SFR_MEM8(0xCF)
#define LRXDL0  0
#define LRXDL1  1
#define LRXDL2  2
#define LRXDL3  3
#define LTXDL0  4
#define LTXDL1  5
#define LTXDL2  6
#define LTXDL3  7

#define LINIDR  _SFR_MEM8(0xD0)
#define LID0    0
#define LID1    1
#define LID2    2
#define LID3    3
#define LID4    4
#define LID5    5
#define LP0     6
#define LP1     7

#define LINSEL  _SFR_MEM8(0xD1)
#define LINDX0  0
#define LINDX1  1
#define LINDX2  2
#define LAINC   3

#define LINDAT  _SFR_MEM8(0xD2)
#define LDATA0  0
#define LDATA1  1
#define LDATA2  2
#define LDATA3  3
#define LDATA4  4
#define LDATA5  5
#define LDATA6  6
#define LDATA7  7

/* Reserved [0xD3..0xD7] */

#define CANGCON _SFR_MEM8(0xD8)
#define SWRES   0
#define ENASTB  1
#define TEST    2
#define LISTEN  3
#define SYNTTC  4
#define TTC     5
#define OVRQ    6
#define ABRQ    7

#define CANGSTA _SFR_MEM8(0xD9)
#define ERRP    0
#define BOFF    1
#define ENFG    2
#define RXBSY   3
#define TXBSY   4
#define OVFG    6

#define CANGIT  _SFR_MEM8(0xDA)
#define AERG    0
#define FERG    1
#define CERG    2
#define SERG    3
#define BXOK    4
#define OVRTIM  5
#define BOFFIT  6
#define CANIT   7

#define CANGIE  _SFR_MEM8(0xDB)
#define ENOVRT  0
#define ENERG   1
#define ENBX    2
#define ENERR   3
#define ENTX    4
#define ENRX    5
#define ENBOFF  6
#define ENIT    7

#define CANEN2  _SFR_MEM8(0xDC)
#define ENMOB0  0
#define ENMOB1  1
#define ENMOB2  2
#define ENMOB3  3
#define ENMOB4  4
#define ENMOB5  5

#define CANEN1  _SFR_MEM8(0xDD)

#define CANIE2  _SFR_MEM8(0xDE)
#define IEMOB0  0
#define IEMOB1  1
#define IEMOB2  2
#define IEMOB3  3
#define IEMOB4  4
#define IEMOB5  5

#define CANIE1  _SFR_MEM8(0xDF)

#define CANSIT2 _SFR_MEM8(0xE0)
#define SIT0    0
#define SIT1    1
#define SIT2    2
#define SIT3    3
#define SIT4    4
#define SIT5    5

#define CANSIT1 _SFR_MEM8(0xE1)

#define CANBT1  _SFR_MEM8(0xE2)
#define BRP0    1
#define BRP1    2
#define BRP2    3
#define BRP3    4
#define BRP4    5
#define BRP5    6

#define CANBT2  _SFR_MEM8(0xE3)
#define PRS0    1
#define PRS1    2
#define PRS2    3
#define SJW0    5
#define SJW1    6

#define CANBT3  _SFR_MEM8(0xE4)
#define SMP     0
#define PHS10   1
#define PHS11   2
#define PHS12   3
#define PHS20   4
#define PHS21   5
#define PHS22   6

#define CANTCON _SFR_MEM8(0xE5)

/* Combine CANTIML and CANTIMH */
#define CANTIM  _SFR_MEM16(0xE6)

#define CANTIML _SFR_MEM8(0xE6)
#define CANTIMH _SFR_MEM8(0xE7)

/* Combine CANTTCL and CANTTCH */
#define CANTTC  _SFR_MEM16(0xE8)

#define CANTTCL _SFR_MEM8(0xE8)
#define CANTTCH _SFR_MEM8(0xE9)

#define CANTEC  _SFR_MEM8(0xEA)

#define CANREC  _SFR_MEM8(0xEB)

#define CANHPMOB _SFR_MEM8(0xEC)
#define CGP0    0
#define CGP1    1
#define CGP2    2
#define CGP3    3
#define HPMOB0  4
#define HPMOB1  5
#define HPMOB2  6
#define HPMOB3  7

#define CANPAGE _SFR_MEM8(0xED)
#define INDX0   0
#define INDX1   1
#define INDX2   2
#define AINC    3
#define MOBNB0  4
#define MOBNB1  5
#define MOBNB2  6
#define MOBNB3  7

#define CANSTMOB _SFR_MEM8(0xEE)
#define AERR    0
#define FERR    1
#define CERR    2
#define SERR    3
#define BERR    4
#define RXOK    5
#define TXOK    6
#define DLCW    7

#define CANCDMOB _SFR_MEM8(0xEF)
#define DLC0    0
#define DLC1    1
#define DLC2    2
#define DLC3    3
#define IDE     4
#define RPLV    5
#define CONMOB0 6
#define CONMOB1 7

#define CANIDT4 _SFR_MEM8(0xF0)
#define RB0TAG  0
#define RB1TAG  1
#define RTRTAG  2
#define IDT0    3
#define IDT1    4
#define IDT2    5
#define IDT3    6
#define IDT4    7

#define CANIDT3 _SFR_MEM8(0xF1)
#define IDT5    0
#define IDT6    1
#define IDT7    2
#define IDT8    3
#define IDT9    4
#define IDT10   5
#define IDT11   6
#define IDT12   7

#define CANIDT2 _SFR_MEM8(0xF2)
#define IDT13   0
#define IDT14   1
#define IDT15   2
#define IDT16   3
#define IDT17   4
#define IDT18   5
#define IDT19   6
#define IDT20   7

#define CANIDT1 _SFR_MEM8(0xF3)
#define IDT21   0
#define IDT22   1
#define IDT23   2
#define IDT24   3
#define IDT25   4
#define IDT26   5
#define IDT27   6
#define IDT28   7

#define CANIDM4 _SFR_MEM8(0xF4)
#define IDEMSK  0
#define RTRMSK  2
#define IDMSK0  3
#define IDMSK1  4
#define IDMSK2  5
#define IDMSK3  6
#define IDMSK4  7

#define CANIDM3 _SFR_MEM8(0xF5)
#define IDMSK5  0
#define IDMSK6  1
#define IDMSK7  2
#define IDMSK8  3
#define IDMSK9  4
#define IDMSK10 5
#define IDMSK11 6
#define IDMSK12 7

#define CANIDM2 _SFR_MEM8(0xF6)
#define IDMSK13 0
#define IDMSK14 1
#define IDMSK15 2
#define IDMSK16 3
#define IDMSK17 4
#define IDMSK18 5
#define IDMSK19 6
#define IDMSK20 7

#define CANIDM1 _SFR_MEM8(0xF7)
#define IDMSK21 0
#define IDMSK22 1
#define IDMSK23 2
#define IDMSK24 3
#define IDMSK25 4
#define IDMSK26 5
#define IDMSK27 6
#define IDMSK28 7

/* Combine CANSTML and CANSTMH */
#define CANSTM  _SFR_MEM16(0xF8)

#define CANSTML _SFR_MEM8(0xF8)
#define CANSTMH _SFR_MEM8(0xF9)

#define CANMSG  _SFR_MEM8(0xFA)



/* Values and associated defines */


#define SLEEP_MODE_IDLE (0x00<<1)
#define SLEEP_MODE_ADC (0x01<<1)
#define SLEEP_MODE_PWR_DOWN (0x02<<1)
#define SLEEP_MODE_STANDBY (0x06<<1)

/* Interrupt vectors */
/* Vector 0 is the reset vector */
/* Analog Comparator 0 */
#define ANACOMP0_vect            _VECTOR(1)
#define ANACOMP0_vect_num        1

/* Analog Comparator 1 */
#define ANACOMP1_vect            _VECTOR(2)
#define ANACOMP1_vect_num        2

/* Analog Comparator 2 */
#define ANACOMP2_vect            _VECTOR(3)
#define ANACOMP2_vect_num        3

/* Analog Comparator 3 */
#define ANACOMP3_vect            _VECTOR(4)
#define ANACOMP3_vect_num        4

/* PSC Fault */
#define PSC_FAULT_vect            _VECTOR(5)
#define PSC_FAULT_vect_num        5

/* PSC End of Cycle */
#define PSC_EC_vect            _VECTOR(6)
#define PSC_EC_vect_num        6

/* External Interrupt Request 0 */
#define INT0_vect            _VECTOR(7)
#define INT0_vect_num        7

/* External Interrupt Request 1 */
#define INT1_vect            _VECTOR(8)
#define INT1_vect_num        8

/* External Interrupt Request 2 */
#define INT2_vect            _VECTOR(9)
#define INT2_vect_num        9

/* External Interrupt Request 3 */
#define INT3_vect            _VECTOR(10)
#define INT3_vect_num        10

/* Timer/Counter1 Capture Event */
#define TIMER1_CAPT_vect            _VECTOR(11)
#define TIMER1_CAPT_vect_num        11

/* Timer/Counter1 Compare Match A */
#define TIMER1_COMPA_vect            _VECTOR(12)
#define TIMER1_COMPA_vect_num        12

/* Timer/Counter1 Compare Match B */
#define TIMER1_COMPB_vect            _VECTOR(13)
#define TIMER1_COMPB_vect_num        13

/* Timer1/Counter1 Overflow */
#define TIMER1_OVF_vect            _VECTOR(14)
#define TIMER1_OVF_vect_num        14

/* Timer/Counter0 Compare Match A */
#define TIMER0_COMPA_vect            _VECTOR(15)
#define TIMER0_COMPA_vect_num        15

/* Timer/Counter0 Compare Match B */
#define TIMER0_COMPB_vect            _VECTOR(16)
#define TIMER0_COMPB_vect_num        16

/* Timer/Counter0 Overflow */
#define TIMER0_OVF_vect            _VECTOR(17)
#define TIMER0_OVF_vect_num        17

/* CAN MOB, Burst, General Errors */
#define CAN_INT_vect            _VECTOR(18)
#define CAN_INT_vect_num        18

/* CAN Timer Overflow */
#define CAN_TOVF_vect            _VECTOR(19)
#define CAN_TOVF_vect_num        19

/* LIN Transfer Complete */
#define LIN_TC_vect            _VECTOR(20)
#define LIN_TC_vect_num        20

/* LIN Error */
#define LIN_ERR_vect            _VECTOR(21)
#define LIN_ERR_vect_num        21

/* Pin Change Interrupt Request 0 */
#define PCINT0_vect            _VECTOR(22)
#define PCINT0_vect_num        22

/* Pin Change Interrupt Request 1 */
#define PCINT1_vect            _VECTOR(23)
#define PCINT1_vect_num        23

/* Pin Change Interrupt Request 2 */
#define PCINT2_vect            _VECTOR(24)
#define PCINT2_vect_num        24

/* Pin Change Interrupt Request 3 */
#define PCINT3_vect            _VECTOR(25)
#define PCINT3_vect_num        25

/* SPI Serial Transfer Complete */
#define SPI_STC_vect            _VECTOR(26)
#define SPI_STC_vect_num        26

/* ADC Conversion Complete */
#define ADC_vect            _VECTOR(27)
#define ADC_vect_num        27

/* Watchdog Time-Out Interrupt */
#define WDT_vect            _VECTOR(28)
#define WDT_vect_num        28

/* EEPROM Ready */
#define EE_READY_vect            _VECTOR(29)
#define EE_READY_vect_num        29

/* Store Program Memory Read */
#define SPM_READY_vect            _VECTOR(30)
#define SPM_READY_vect_num        30

#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#  define _VECTORS_SIZE 124
#else
#  define _VECTORS_SIZE 124U
#endif


/* Constants */

#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#  define SPM_PAGESIZE 256
#  define FLASHSTART   0x0000
#  define FLASHEND     0xFFFF
#else
#  define SPM_PAGESIZE 256U
#  define FLASHSTART   0x0000U
#  define FLASHEND     0xFFFFU
#endif
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#  define RAMSTART     0x0100
#  define RAMSIZE      4096
#  define RAMEND       0x10FF
#else
#  define RAMSTART     0x0100U
#  define RAMSIZE      4096U
#  define RAMEND       0x10FFU
#endif
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#  define E2START     0
#  define E2SIZE      2048
#  define E2PAGESIZE  8
#  define E2END       0x07FF
#else
#  define E2START     0U
#  define E2SIZE      2048U
#  define E2PAGESIZE  8U
#  define E2END       0x07FFU
#endif
#define XRAMEND      RAMEND


/* Fuses */

#define FUSE_MEMORY_SIZE 3

/* Low Fuse Byte */
#define FUSE_SUT_CKSEL0  (unsigned char)~_BV(0)
#define FUSE_SUT_CKSEL1  (unsigned char)~_BV(1)
#define FUSE_SUT_CKSEL2  (unsigned char)~_BV(2)
#define FUSE_SUT_CKSEL3  (unsigned char)~_BV(3)
#define FUSE_SUT_CKSEL4  (unsigned char)~_BV(4)
#define FUSE_SUT_CKSEL5  (unsigned char)~_BV(5)
#define FUSE_CKOUT       (unsigned char)~_BV(6)
#define FUSE_CKDIV8      (unsigned char)~_BV(7)
#define LFUSE_DEFAULT    (FUSE_SUT_CKSEL0 & FUSE_SUT_CKSEL2 & FUSE_SUT_CKSEL3 & FUSE_SUT_CKSEL4 & FUSE_CKDIV8)


/* High Fuse Byte */
#define FUSE_BOOTRST     (unsigned char)~_BV(0)
#define FUSE_BOOTSZ0     (unsigned char)~_BV(1)
#define FUSE_BOOTSZ1     (unsigned char)~_BV(2)
#define FUSE_EESAVE      (unsigned char)~_BV(3)
#define FUSE_WDTON       (unsigned char)~_BV(4)
#define FUSE_SPIEN       (unsigned char)~_BV(5)
#define FUSE_DWEN        (unsigned char)~_BV(6)
#define FUSE_RSTDISBL    (unsigned char)~_BV(7)
#define HFUSE_DEFAULT    (FUSE_BOOTSZ0 & FUSE_BOOTSZ1 & FUSE_SPIEN)


/* Extended Fuse Byte */
#define FUSE_BODLEVEL0   (unsigned char)~_BV(0)
#define FUSE_BODLEVEL1   (unsigned char)~_BV(1)
#define FUSE_BODLEVEL2   (unsigned char)~_BV(2)
#define FUSE_PSCRVB      (unsigned char)~_BV(3)
#define FUSE_PSCRVA      (unsigned char)~_BV(4)
#define FUSE_PSCRB       (unsigned char)~_BV(5)
#define EFUSE_DEFAULT    (0xFF)



/* Lock Bits */
#define __LOCK_BITS_EXIST
#define __BOOT_LOCK_BITS_0_EXIST
#define __BOOT_LOCK_BITS_1_EXIST


/* Signature */
#define SIGNATURE_0 0x1E
#define SIGNATURE_1 0x96
#define SIGNATURE_2 0x84




#endif /* #ifdef _AVR_ATMEGAS64M1_H_INCLUDED */

