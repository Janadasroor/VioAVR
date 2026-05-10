# CustomAVR Test Kit - Phase 2 Complete

## Overview

A comprehensive test suite for validating custom AVR chip definitions, ensuring hardware accuracy including memory maps, register addresses, pin mappings, and peripheral behavior.

## Status: ✅ ALL TESTS PASSING

```
[doctest] test cases: 12 | 12 passed | 0 failed | 0 skipped
[doctest] assertions: 62 | 62 passed | 0 failed |
[doctest] Status: SUCCESS!

100% tests passed, 0 tests failed out of 11
```

## Test Kit Structure

```
tests/custom_chip/
├── custom_avr.hpp              # Custom chip device descriptor
├── test_custom_avr.cpp         # Comprehensive test suite (12 test suites)
├── CMakeLists.txt              # Build configuration
├── pin_accuracy_validator.py   # Pin mapping validation script
├── README.md                   # User documentation
└── TEST_KIT_SUMMARY.md         # This file
```

## Test Coverage

### 1. Memory Map Verification ✅
- Flash: 32KB (0x8000 words)
- SRAM: 2KB starting at 0x100
- EEPROM: 1KB
- Stack pointer auto-calculation

### 2. GPIO Port Register Map ✅
- Port B: 0x23-0x25 (PIN/DDR/PORT)
- Port C: 0x26-0x28
- Port D: 0x29-0x2B

### 3. Timer Register Map ✅
- Timer0 (8-bit): TCNT0, OCR0A/B, TCCR0A/B
- Timer1 (16-bit): TCNT1, OCR1A/B, ICR1 with TEMP register handling

### 4. UART Register Map ✅
- UDR0, UCSR0A/B/C, UBRR0
- Status bit vs control bit handling

### 5. Pin Mux Accuracy ✅ (CRITICAL)
| Function | Pin | Port.Bit |
|----------|-----|----------|
| RXD | 16 | PD0 |
| TXD | 17 | PD1 |
| OC1A | 1 | PB1 |
| OC1B | 2 | PB2 |
| MOSI | 3 | PB3 |
| MISO | 4 | PB4 |
| SCK | 5 | PB5 |
| ADC0-5 | 8-13 | PC0-5 |

### 6. Interrupt Vector Table ✅
- 26 vectors matching ATmega328P
- Reset, INT0/1, PCINT0-2
- Timer0/1/2 vectors
- UART RX/UDRE/TX
- ADC, TWI, etc.

### 7. Functional Accuracy ✅
- Timer CTC mode
- UART state
- GPIO read/write

### 8. Reset State ✅
- SREG = 0x00
- SP = SRAM end
- Peripherals at reset values

### 9. Instruction Timing ✅
- NOP: 1 cycle
- RJMP: 2 cycles
- PUSH: 2 cycles

### 10. Pin Mux Priority ✅
- UART overrides GPIO on TX/RX pins
- Timer outputs override GPIO
- SPI overrides GPIO

## How to Use

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

### Run via CTest
```bash
ctest -R custom
```

### Run Individual Test Categories
```bash
ctest -R custom_memory_map -V
ctest -R custom_pin_mux -V
ctest -R custom_timing -V
```

### Run Python Pin Validator
```bash
cd /home/jnd/cpp_projects/VioAVR/build/tests/custom_chip
python3 ../../../tests/custom_chip/pin_accuracy_validator.py
```

## Key Design Decisions

### 1. SRAM Range Calculation
- `sram_range().end = sram_start + sram_bytes - 1`
- For 0x100-0x8FF, the end address is 0x8FF (not 0x900)
- This matches real AVR hardware where end is inclusive

### 2. Timer 16-bit Register Access
- High byte must be written first (goes to TEMP register)
- Low byte write triggers the 16-bit update
- Reading works similarly (high byte from TEMP)

### 3. UART Register Semantics
- UCSR0A: Mostly read-only status (U2X/MPCM writable)
- UDRE bit reflects TX buffer state, not directly writable
- UCSR0B/C: Fully writable control registers

### 4. Stack Pointer Reset
- When `spl_reset/sph_reset = 0`, SP auto-calculates from SRAM end
- Ensures correct initialization without hardcoded values

## Pin Accuracy Validation

The Python validator checks:
1. ✓ UART on PD0/PD1
2. ✓ SPI on PB3-5
3. ✓ I2C on PC4/5
4. ✓ ADC on PC0-5
5. ✓ Timer outputs on correct pins

## Extending for New Chips

To test a different custom chip:

1. **Create new descriptor** (copy `custom_avr.hpp`):
   ```cpp
   struct MyChipDescriptor {
       static constexpr std::string_view name = "MyChip";
       static constexpr u16 flash_words = ...;
       // ... other definitions
   };
   ```

2. **Update test fixture** in `test_custom_avr.cpp`:
   - Adjust `create_device_descriptor()`
   - Modify `setup_peripherals()` for your chip's peripherals

3. **Adjust test expectations**:
   - Update memory sizes
   - Change register addresses
   - Modify pin mappings

4. **Rebuild and run**:
   ```bash
   make vioavr_custom_avr_test
   ./tests/custom_chip/vioavr_custom_avr_test
   ```

## Integration with Main Project

The test kit is fully integrated:
- CMake automatically includes it
- CTest discovers all test categories
- Built with the main `make` command
- Uses same doctest framework as other tests

## What This Validates

✅ **Register Map Accuracy**: Every peripheral register at correct address  
✅ **Memory Layout**: Flash, SRAM, EEPROM sizes and boundaries  
✅ **Pin Mapping**: No mismatches - critical for real hardware  
✅ **Interrupt Vectors**: Correct priority and ordering  
✅ **Functional Behavior**: Peripherals work as expected  
✅ **Timing**: Instruction cycle counts match datasheet  

## CI/CD Ready

```bash
# Quick smoke test
ctest -R custom_avr_full_suite

# Full validation
ctest -R custom --output-on-failure

# Pin accuracy (Python)
python3 tests/custom_chip/pin_accuracy_validator.py
```

---

**Created**: Phase 2 - Custom Chip Test Kit  
**Status**: ✅ All tests passing  
**Files**: 5 (hpp, cpp, CMakeLists.txt, py, md)  
**Test Cases**: 12 suites, 62 assertions  
