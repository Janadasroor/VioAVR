# Changelog

All notable changes to my **VioAVR** project will be documented in this file.

## [0.1.0] - 2026-03-31

### Added
- **Core CPU Engine**: High-performance AVR Instruction Set Simulator (ISS) with support for arithmetic, logic, branch, and data transfer instructions.
- **Cycle-Accurate Timing**: "Retirement-then-Tick" atomic cycle advancement model.
- **Peripheral Suite**:
    - GPIO (DDR, PORT, PIN mapping with change events).
    - Timer8 and Timer16 (CTC, PWM, Prescalers, External Clocking).
    - UART/USART (Interrupt-driven, RX injection).
    - ADC (Auto-triggering, 10-bit conversion, multiple channels).
    - Analog Comparator (ACSR-backed, interrupt support).
    - SPI and TWI (I2C) communication engines.
    - EEPROM and Watchdog Timer (WDT).
- **Automated Device Catalog**: generator script and catalog supporting over 135 AVR chip variants (ATmega/ATtiny) via official `.atdf` files.
- **SyncEngine**: Mixed-mode co-simulation engine for lock-step synchronization with analog solvers like **ngspice**.
- **Hex Loader**: Robust Intel HEX parser for loading production binaries.
- **CLI Tool**: `vioavr-cli` for headless firmware execution and instruction tracing.
- **Unit Tests**: Comprehensive test suite with over 60 test files covering CPU and peripherals.
- **CI/CD**: GitHub Actions workflow for automated builds and testing.

### Refactored
- Transitioned from hardcoded device descriptors to a dynamic, searchable `DeviceCatalog`.
- Decoupled `MemoryBus` from specific MCU defaults.

### Legal
- Project licensed under the **Apache License 2.0**.
---
*Created by Janada Sroor.*
