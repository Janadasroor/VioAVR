# VioAVR Implemented Features

This document provides a comprehensive list of features currently implemented in the VioAVR Instruction Set Simulator.

## 1. Core CPU (`AvrCpu`)

- **Instruction Set Architecture (ISA):**
  - Support for over 130 AVR instructions, including:
    - **Arithmetic/Logic:** `ADD`, `ADC`, `SUB`, `SBC`, `AND`, `OR`, `EOR`, `COM`, `NEG`, `SBR`, `CBR`, `INC`, `DEC`, `TST`, `CLR`, `SER`.
    - **Branching:** `RJMP`, `IJMP`, `JMP`, `RCALL`, `ICALL`, `CALL`, `RET`, `RETI`, `CPSE`, `CP`, `CPC`, `CPI`, `SBRC`, `SBRS`, `SBIC`, `SBIS`, `BRBS`, `BRBC`, and all status-bit aliases (`BREQ`, `BRNE`, `BRCS`, etc.).
    - **Data Transfer:** `MOV`, `MOVW`, `LDI`, `LD`, `ST`, `LDS`, `STS`, `LPM`, `SPM`, `IN`, `OUT`, `PUSH`, `POP`.
    - **Bit Manipulation:** `SBI`, `CBI`, `LSL`, `LSR`, `ROL`, `ROR`, `ASR`, `SWAP`, `BSET`, `BCLR`, `BST`, `BLD`.
    - **Multiply:** `MUL`, `MULS`, `MULSU`, `FMUL`, `FMULS`, `FMULSU`.
    - **System:** `NOP`, `SLEEP`, `WDR`, `BREAK`.
- **Cycle-Accuracy:** Every instruction consumes the exact number of cycles specified in the AVR datasheets.
- **Interrupt Controller:**
  - Vector-based interrupt dispatching.
  - Hardware-style priority (lowest vector index has highest priority).
  - Non-destructive observation and destructive consumption of interrupt requests.
  - Support for nested interrupts via `RETI`.
- **Power Management:**
  - `SLEEP` mode implementation with peripheral-driven wake-up.
  - Idle cycle advancement during sleep.
- **Register File:** Full 32-register GPR file (`r0-r31`) with support for 16-bit pairs (`X`, `Y`, `Z`).

## 2. Memory System (`MemoryBus`)

- **Unified Data Space:** Linear mapping of registers, I/O, and SRAM.
- **Flash Memory:** Word-organized storage for program instructions with byte-granular `LPM` access.
- **EEPROM:** Fully modeled on-chip EEPROM with address-triggered read/write cycles.
- **External Memory (XMEM):** Support for external SRAM interfacing with wait-state modeling.
- **Address Decoding:** Modular `IoPeripheral` attachment system for memory-mapped I/O.

## 3. Digital Peripherals

- **GPIO Ports:**
  - Full `PIN`, `DDR`, and `PORT` semantics.
  - Bit-granular state tracking and change notification.
  - Support for pull-up resistors and Schmitt-threshold inputs.
- **Timers:**
  - **Timer8 (8-bit):** Supports CTC, PWM (Fast/Phase Correct), and external clocking on `Tn` pins.
  - **Timer16 (16-bit):** Supports Input Capture, multiple Output Compare units, and various PWM modes.
  - **Timer10 (High-Speed):** High-resolution 10-bit PWM timers.
- **Serial Communication:**
  - **USART/UART:** Full-duplex asynchronous serial with interrupt support (TXC, RXC, UDRE) and FIFO queues.
  - **SPI:** Master/Slave modes, programmable bit order, and clock phases.
  - **TWI (I2C):** Full Master/Slave support with status-code generation and multi-byte buffering.
- **Interrupts:**
  - **External Interrupts (INTn):** Configurable edge (rising/falling/any) or level triggering.
  - **Pin Change Interrupts (PCINT):** Grouped masked changes on any GPIO pin.

## 4. Analog & Mixed-Signal

- **Analog-to-Digital Converter (ADC):**
  - 10-bit resolution with multi-channel multiplexing.
  - **Auto-Triggering:** Can be triggered by Timer overflow/compare, Analog Comparator, or External Interrupts.
  - Programmable conversion times.
- **Analog Comparator:**
  - Differential input with optional hysteresis.
  - Interrupt on toggle, rising, or falling edges.
  - Can trigger ADC conversions.
- **AnalogSignalBank:**
  - A shared bridge between co-simulation voltages and digital thresholds.
  - Allows one analog source to drive multiple peripherals (ADC, Comparator, GPIO, INTn).

## 5. System Functions

- **Watchdog Timer (WDT):** Configurable timeout and interrupt/reset capability.
- **PLL:** High-speed clock generation for USB and specialized timers.
- **USB:** Basic endpoint and FIFO management for USB-enabled AVRs.
- **CAN:** Controller Area Network logic for automotive-grade AVR variants.

## 6. Simulation & Tooling

- **Device Catalog:** Automatic support for ~135 chips based on official Microchip ATDF data.
- **SyncEngine:** A high-precision synchronization hook for lock-step co-simulation with SPICE (ngspice).
- **Hex Loader:** Robust Intel HEX parser for loading production binaries.
- **Tracing:** Detailed execution tracing including registers, memory, and peripheral state changes.
- **Qt Bridge:** Optional UI layer for visualization and interactive debugging.
