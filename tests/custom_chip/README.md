# CustomAVR Test Kit

A comprehensive test suite for validating custom AVR chip definitions, ensuring hardware accuracy including memory maps, register addresses, pin mappings, and peripheral behavior.

## Overview

This test kit validates that a custom AVR chip simulation behaves exactly like real hardware:

- **Memory Map Accuracy**: Flash, SRAM, EEPROM sizes and addresses
- **Register Map Accuracy**: All peripheral register addresses match datasheet
- **Pin Mux Accuracy**: Peripheral functions map to correct physical pins
- **Interrupt Vectors**: Correct vector table with proper priorities
- **Instruction Timing**: Cycle-accurate execution
- **Functional Accuracy**: Peripherals behave like real hardware

## Files

| File | Description |
|------|-------------|
| `custom_avr.hpp` | Custom chip device descriptor |
| `test_custom_avr.cpp` | Comprehensive test suite (10 test suites, 30+ test cases) |
| `CMakeLists.txt` | Build configuration with individual test targets |
| `pin_accuracy_validator.py` | Python script for pin mapping validation |
| `README.md` | This file |

## Quick Start

### Build the Test
```bash
cd /home/jnd/cpp_projects/VioAVR/build
cmake ..
make vioavr_custom_avr_test
```

### Run All Tests
```bash
./tests/custom_chip/vioavr_custom_avr_test
```

### Run Specific Test Category
```bash
# Memory map tests only
./tests/custom_chip/vioavr_custom_avr_test -tc="*[CustomAVR]*Memory Map*"

# Pin mux tests only
./tests/custom_chip/vioavr_custom_avr_test -tc="*[CustomAVR]*Pin Mux*"

# Instruction timing tests
./tests/custom_chip/vioavr_custom_avr_test -tc="*[CustomAVR]*Timing*"
```

### Run via CTest
```bash
ctest -R custom_avr
```

## Test Categories

### 1. Memory Map Verification
- Flash size: 32KB (0x8000 words)
- SRAM: 2KB starting at 0x100
- EEPROM: 1KB
- Stack pointer reset: 0x8FF

### 2. GPIO Port Register Map
- Port B: PINB=0x23, DDRB=0x24, PORTB=0x25
- Port C: PINC=0x26, DDRC=0x27, PORTC=0x28  
- Port D: PIND=0x29, DDRD=0x2A, PORTD=0x2B

### 3. Timer Register Map
- **Timer0 (8-bit)**: TCNT0=0x46, OCR0A=0x47, OCR0B=0x48
- **Timer1 (16-bit)**: TCNT1=0x84, OCR1A=0x88, ICR1=0x86

### 4. UART Register Map
- UDR0=0xC6, UCSR0A=0xC0, UCSR0B=0xC1
- UCSR0C=0xC2, UBRR0=0xC4

### 5. Pin Mux Accuracy (CRITICAL)

| Function | Expected Pin | Port.Bit |
|----------|-------------|----------|
| RXD | 16 | PD0 |
| TXD | 17 | PD1 |
| OC1A | 1 | PB1 |
| OC1B | 2 | PB2 |
| ICP1 | 0 | PB0 |
| MOSI | 3 | PB3 |
| MISO | 4 | PB4 |
| SCK | 5 | PB5 |
| ADC0-5 | 8-13 | PC0-5 |
| SDA | 12 | PC4 |
| SCL | 13 | PC5 |

### 6. Interrupt Vector Table
26 vectors matching ATmega328P:
- RESET (0), INT0 (1), INT1 (2)
- Timer0: COMPA (14), COMPB (15), OVF (16)
- Timer1: CAPT (10), COMPA (11), COMPB (12), OVF (13)
- UART: RX (18), UDRE (19), TX (20)
- ADC (21), TWI (24)

### 7. Functional Accuracy
- Timer0 CTC mode
- UART configuration
- GPIO toggle timing

### 8. Reset State
- SREG = 0x00
- SP = 0x8FF
- All peripherals reset to 0

### 9. Instruction Timing
- NOP: 1 cycle
- RJMP: 2 cycles
- PUSH: 2 cycles

### 10. Pin Mux Priority
- UART TX overrides GPIO on PD1
- SPI overrides GPIO on PB3-5
- Timer OC1A/OC1B overrides GPIO on PB1-2

## Pin Accuracy Validator

The Python script `pin_accuracy_validator.py` provides detailed validation:

```bash
cd /home/jnd/cpp_projects/VioAVR/build/tests/custom_chip
python3 ../../../tests/custom_chip/pin_accuracy_validator.py
```

This validates:
- ✓ UART pins on correct Port D pins
- ✓ SPI pins on correct Port B pins  
- ✓ I2C pins on correct Port C pins
- ✓ ADC channels on PC0-5
- ✓ Timer outputs on correct pins

## Customizing the Chip

To test a different custom chip:

1. Modify `custom_avr.hpp`:
   - Change pin mappings
   - Adjust memory sizes
   - Modify register addresses
   - Add/remove peripherals

2. Update expected values in `test_custom_avr.cpp`

3. Update `pin_accuracy_validator.py` with new expected pin mappings

4. Rebuild and run tests

## Expected Results

All tests should pass:
```
[doctest] test cases: 30+ | 30+ passed | 0 failed |
[doctest] assertions: 100+ | 100+ passed | 0 failed |
```

## Pin Mismatch Detection

The most critical validation is pin mux accuracy. Common errors:

1. **UART on wrong pins**: Breaks serial communication
2. **SPI on wrong pins**: Breaks bootloader/flash programming
3. **Timer outputs wrong**: Breaks PWM applications
4. **ADC channels wrong**: Breaks analog input

The validator catches these by checking:
- Peripheral registers at correct addresses
- Pin indices match hardware
- Pin mux priority (peripheral > GPIO)

## Integration with Main Test Suite

This test kit is integrated into the main CMake build:
- Built automatically with `make`
- Run via `ctest -R custom`
- Individual test targets for debugging

## References

- ATmega328P datasheet for reference implementation
- AVR Instruction Set Manual for cycle timing
- ATDF (AVR Tools Device File) for register definitions
