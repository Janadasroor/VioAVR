# VioAVR

VioAVR is a high-performance, lightweight AVR Instruction Set Simulator (ISS) written in C++. It is specifically designed to be integrated into **VIOSpice**, a mixed-mode electronic circuit simulator that combines digital microcontroller simulation with analog circuitry using **ngspice**.

## Vision

The goal of VioAVR is to provide a "Proteus-like" experience for open-source tools. By bridging the gap between digital AVR emulation and analog SPICE simulation, VioAVR allows developers to simulate complex embedded systems where the MCU interacts with real-world analog components (sensors, power electronics, motors, etc.).

## Features

- **High-Performance Core:** A cycle-accurate ISS capable of running production AVR binaries (.hex files).
- **Device Family Pack (DFP) Support:** Automatically supports over 135 AVR chip variants (ATmega, ATtiny, etc.) via the built-in `DeviceCatalog`.
- **Mixed-Mode Ready:** Built with synchronization hooks (`SyncEngine`) designed for lock-step execution with analog simulators like **ngspice**.
- **Peripheral Support:** 
  - GPIO Ports with bit-granular tracking.
  - 8-bit and 16-bit Timers (CTC, PWM, External Clocking).
  - USART/UART with interrupt support.
  - ADC (Analog-to-Digital Converter) with auto-triggering.
  - Analog Comparator.
  - External and Pin-Change Interrupts.
  - Watchdog Timer (WDT), EEPROM, and SPI.
- **Zero-Dependency Core:** The simulation engine is pure C++ and does not depend on heavy libraries, making it portable and easy to embed.
- **Optional Qt Layer:** A bridge for GUI-based observation and debugging.

## Project Structure

- `src/core/`: The heart of the simulator (CPU, Memory Bus, Peripherals).
- `include/vioavr/core/`: Public headers for the simulation engine.
- `atmega/`: Official AVR Device Family Pack (DFP) data and headers.
- `scripts/`: Tools for generating device descriptors and processing HEX files.
- `apps/vioavr-cli/`: A command-line runner for testing and tracing.
- `tests/`: Extensive test suite covering CPU instructions and peripheral behavior.

## Getting Started

### Prerequisites

- CMake (3.15+)
- A C++17 compatible compiler (GCC, Clang, MSVC)
- Python 3 (for device generation scripts)

### Building

```bash
mkdir build
cd build
cmake ..
make
```

### Running Tests

```bash
cd build
ctest
```

## Relationship with VIOSpice

VioAVR is the digital engine for **VIOSpice**. While VioAVR handles the opcode execution and digital logic, VIOSpice manages the "mixed-signal bridge" that translates digital high/low states into SPICE voltage levels and vice-versa, allowing for true co-simulation of firmware and hardware.

## License

This project is licensed under the **Apache License 2.0**. See the [LICENSE](LICENSE) file for the full text.

---
*Created by Janada Sroor. VioAVR is part of the Viospice ecosystem.*
