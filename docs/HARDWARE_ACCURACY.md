# VioAVR Hardware Accuracy Standards

This document defines the requirements and process for ensuring that VioAVR simulation matches physical AVR hardware as described in the official Microchip/Atmel Type Definition Files (ATDF) and datasheets.

## Accuracy Pillars

### 1. Memory Map Accuracy
- **Flash**: Correct size, page size, and RWW/NRWW boundary (if applicable).
- **SRAM**: Correct start address and size.
- **EEPROM**: Correct size and page size.
- **I/O Space**: Correct mapping of standard I/O (0x20-0x5F) and Extended I/O.
- **Fuses/Lockbits**: Correct address space offsets.

### 2. Register & Bit Fidelity
- **Address Integrity**: Every peripheral register must match the ATDF offset.
- **Reset Values**: Registers must initialize to the state defined in ATDF `initval`.
- **Bitfield Mapping**: Peripheral logic must use bitmasks that match the ATDF `mask` values.
- **Read/Write Semantics**: Observe `ocd-rw` attributes (e.g., read-only status flags).

### 3. Interrupt Vectoring
- **Vector Table**: Indices must match the ATDF interrupt table.
- **Priority**: Hardware-defined priority (usually lower index = higher priority) must be respected.
- **Enable/Flag Logic**: Interrupts must only fire when the corresponding enable bit is set and the flag is latched.

### 4. Pin Multiplexing (PinMux)
- **Signal Mapping**: Peripheral functions (TXD, ADC0, OC0A, etc.) must be bound to the correct physical PORT/PIN.
- **Override Logic**: Peripheral activation must correctly override GPIO control (e.g., enabling UART TX overrides the DDR/PORT bit for that pin).

## Verification Tools

The following scripts are provided to automate the accuracy audit:

### 1. `scripts/atdf_export.py`
Exports all hardware metadata from an ATDF file to a structured JSON.
```bash
./scripts/atdf_export.py --mcu ATmega328P
```

### 2. `scripts/gen_audit_md.py`
Generates a comprehensive Markdown audit checklist from the exported JSON metadata.
```bash
python3 scripts/gen_audit_md.py --mcu ATmega328P --meta ATmega328P_metadata.json
```

### 3. `scripts/re-audit.sh`
A convenience script to export and generate a new audit report in one step.
```bash
./scripts/re-audit.sh ATmega328P
```

## Accuracy Workflow for New Chips

When adding support for a new AVR chip (e.g., `ATmega2560`):

1.  **Metadata Extraction**: Run `./scripts/atdf_export.py --mcu ATmega2560`.
2.  **Generate Audit**: Run `python3 scripts/gen_audit_md.py --mcu ATmega2560 --meta ATmega2560_metadata.json`.
3.  **Core Implementation**:
    - Use `scripts/gen_device.py` to generate the initial C++ `DeviceDescriptor`.
    - Manually verify against `docs/ACCURACY_ATmega2560.md`.
4.  **Peripheral Validation**:
    - For each peripheral (Timer, UART, etc.), ensure bitmasks in `src/core/` match the audit file.
    - Check that `initval` in ATDF matches the `reset()` state in C++.
5.  **Behavioral Validation**:
    - Run `scripts/verify_against_simavr.py` with test programs to ensure cycle-accurate behavior.

## Verification Sources
- **Primary**: Bundled `.atdf` files in `atmega/atdf/`.
- **Secondary**: Official Microchip datasheets.
- **Validation**: `simavr` trace comparisons (using `scripts/verify_against_simavr.py`).
- **Hardware**: Logic analyzer traces from physical MCUs (when available).
