# VioAVR

VioAVR is a high-performance, cycle-accurate AVR Instruction Set Simulator (ISS) written in C++20. It simulates a broad range of AVR microcontrollers — from classic ATmega and ATtiny to modern AVR-Dx/Ex/Lx/Sx, megaAVR-0, tinyAVR-0/1/2, and XMEGA families — and is designed for tight integration with **ngspice** for mixed-signal co-simulation.

## Features

- **Cycle-Accurate Core** — Full AVR instruction set with JIT compilation delivering 60–140 MHz effective throughput on x86-64.
- **290+ Device Descriptors** — Auto-generated from Microchip ATDF Device Family Packs covering ATmega, ATtiny, AVR-Dx/Ex/Lx/Sx, and XMEGA. 3 hand-crafted descriptors (ATmega328P, ATmega4809, AT90PWM3) with richer peripheral detail.
- **Comprehensive Peripherals** — GPIO, UART/UART8X, SPI/SPI8X, TWI/TWI8X, USI, TCA/TCB/TCD/TCE timers, RTC, AWEX, ADC (10/12-bit), AC, DAC, CCL, EVSYS, NVMCTRL, CPUINT, EEPROM, WDT, CRC, CAN, USB, LCD, PSC, DMA, OPAMP, ZCD, PTC, IRCOM, and more.
- **Event System (EVSYS)** — Peripheral-to-peripheral chaining without CPU intervention (TCA→TCB cascade, ADC→timer trigger, CCL output events, etc.).
- **Configurable Custom Logic (CCL)** — Up to 4 LUTs with programmable truth tables and pin/event/analog inputs.
- **ngspice Co-Simulation** — Two integration paths: in-process `d_cosim` shim (low latency, multi-chip) and out-of-process POSIX shared memory bridge (d_vioavr A-device). Analog bridge models for ADC input, DAC output, and dynamic VCC tracking.
- **JIT Compilation** — Lightweight x86-64 JIT translates AVR basic blocks to native code with automatic fallback for self-modifying code.
- **GDB Stub** — Remote debugging via GDB Remote Serial Protocol with breakpoints, single-stepping, register/memory access, and flash programming.
- **Unified CLI** — `vioavr <subcommand>` with `run`, `trace`, `benchmark`, `info`, `list-devices`, `bridge`, `gdb`, `docs`, and `help`. Full ANSI terminal control with colors, progress bars, and `--color auto|always|never`.
- **Built-in Documentation** — `vioavr docs <topic>` for instant reference on MCU families, peripherals, co-simulation, device descriptor pipeline, XSPICE architecture, and firmware programming guide.
- **Extensive Test Suite** — 300+ ngspice co-simulation tests and 36 core tests covering CPU instructions, peripherals, and mixed-signal scenarios.

## Project Structure

```
├── include/vioavr/core/     Public headers (device descriptors, CPU, memory bus, peripherals)
├── src/core/                Simulation engine implementation
├── apps/
│   ├── vioavr/              Unified CLI (run, trace, benchmark, info, docs, gdb, bridge)
│   ├── vioavr-cli/          Legacy CLI (backward compatible)
│   └── vioavr-bridge-daemon/ SHM bridge daemon
├── cosim/                   ngspice co-simulation shim and analog bridge models
├── tests/
│   ├── core/                Unit and co-simulation tests
│   └── firmware/            Reference firmware for cross-simulator comparison
├── tools/                   ATDF device descriptor generator
├── scripts/                 Validation, audit, and export scripts
├── avr-pack/                Microchip ATDF data (device family packs)
└── scratch/                 Demo circuits and generated test fixtures
```

## Quick Start

### Prerequisites

- CMake 3.15+
- C++20 compatible compiler (GCC 11+, Clang 14+)
- Python 3 (for device descriptor generation)

### Build & Test

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Run a firmware image
./build/apps/vioavr/vioavr run firmware.hex --mcu ATmega328P

# Trace execution
./build/apps/vioavr/vioavr trace firmware.hex --mcu ATmega328P --max-cycles 100

# List supported MCUs
./build/apps/vioavr/vioavr list-devices --filter mega

# Built-in documentation
./build/apps/vioavr/vioavr docs overview
./build/apps/vioavr/vioavr docs --search evsys

# Run tests
ctest --test-dir build -j$(nproc) --output-on-failure
```

### Co-Simulation with ngspice

```bash
# Start bridge daemon (out-of-process)
./build/apps/vioavr/vioavr bridge --mcu atmega4809 --instance my_avr &

# Run ngspice with d_vioavr A-device
ngspice -b my_circuit.cir
```

### GDB Debugging

```bash
./build/apps/vioavr/vioavr gdb firmware.hex --mcu ATmega328P --port 1234
gdb-multiarch -ex 'target remote :1234' firmware.elf
```

## License

Apache License 2.0. See [LICENSE](LICENSE).

---

*VioAVR is part of the VIOSpice ecosystem. Created by Janada Sroor.*
