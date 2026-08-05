# VioAVR

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Architecture](https://img.shields.io/badge/Architecture-AVR%20%7C%20XMEGA%20%7C%20AVR--Dx-orange.svg)]()
[![JIT Speed](https://img.shields.io/badge/JIT%20Throughput-146%2B%20MHz-brightgreen.svg)]()

VioAVR is an industrial-grade, cycle-accurate **AVR Instruction Set Simulator (ISS)** and mixed-signal hardware co-simulation engine written in modern **C++20**.

It delivers **146+ MHz simulation speeds** through a custom lightweight x86-64 Dynamic JIT Compiler and provides native integration with **ngspice / XSpice** for real-time digital/analog circuit co-simulation.

---

> [!IMPORTANT]
> ### 💼 Commercial & Enterprise Dual-Licensing
> VioAVR is available under a **Commercial Dual-License** for organizations embedding VioAVR into proprietary/closed-source products, online hardware simulators, or automated CI/CD firmware testing pipelines without open-source GPLv3 restrictions.
> 
> **For commercial license inquiries, custom MCU driver development, or full source IP acquisition:**  
> 📧 Contact: **janadasroor@gmail.com**

---

## Key Features

* **⚡ Ultra-Fast Dynamic JIT Compiler**: Custom x86-64 basic-block JIT delivers **146+ MHz bare core** and **62+ MHz full machine** simulation speed (5x–15x faster than standard switch interpreters).
* **📱 299+ Hardware Descriptors**: Auto-generated from official Microchip ATDF Device Family Packs covering **ATmega, ATtiny, AVR-Dx/Ex/Lx/Sx, megaAVR-0, tinyAVR-0/1/2, and XMEGA** families.
* **🔌 NGSpice Mixed-Signal Co-Simulation**: Native POSIX Shared Memory Bridge (`BridgeShm`) and `d_cosim` XSpice plugin for real-time digital MCU / analog circuit interaction (ADC inputs, DAC outputs, dynamic VCC tracking).
* **🤖 Autonomous AI Agent & Vibe-Coding Ready**: High-speed, non-interactive CLI execution (`vioavr run`, `vioavr trace`, `vioavr docs`) enables AI coding agents (Claude Code, Codex, OpenCode, Antigravity) to execute, trace, and auto-debug compiled `.hex` firmware loops headlessly.
* **🎛️ Rich Peripheral Ecosystem**: Full hardware emulation for GPIO, UART/UART8X, SPI/SPI8X, TWI/TWI8X, USI, TCA/TCB/TCD/TCE Timers, RTC, AWEX, 10/12-bit ADC, AC, DAC, CCL, EVSYS, NVMCTRL, CPUINT, EEPROM, WDT, CRC, CAN, USB SIE, LCD, PSC, DMA, OPAMP, ZCD, and PTC.
* **🐞 Integrated GDB Stub**: Source-level remote debugging with `gdb-multiarch` via GDB Remote Serial Protocol (RSP).
* **💻 Unified CLI & Terminal Documentation**: Full ANSI terminal CLI (`vioavr`) with built-in instant documentation (`vioavr docs <topic>`) and interactive debugging REPL (`vioavr debug`).

---

## System Architecture

```
                                 ┌─────────────────────────────────┐
                                 │       Firmware (.hex / .elf)    │
                                 └────────────────┬────────────────┘
                                                  │
                                                  ▼
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                                   VioAVR Core Engine                                    │
│                                                                                         │
│  ┌────────────────────────┐    ┌────────────────────────┐    ┌───────────────────────┐  │
│  │   x86-64 JIT Compiler  │ ◄─►│   AVR CPU / Register   │ ◄─►│   Event System        │  │
│  │   (146+ MHz Execution) │    │   State Machine        │    │   (EVSYS / CCL)       │  │
│  └────────────────────────┘    └────────────────────────┘    └───────────────────────┘  │
│                                             │                                           │
│                                             ▼                                           │
│  ┌───────────────────────────────────────────────────────────────────────────────────┐  │
│  │                          Unified Peripheral Bus (299+ MCUs)                       │  │
│  │    [GPIO]   [UART8X]   [SPI]   [TWI]   [Timers A/B/C/D]   [ADC/DAC]   [USB]   ...   │  │
│  └───────────────────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────┬───────────────────────────────────────────┘
                                              │
                    ┌─────────────────────────┴─────────────────────────┐
                    │ Shared Memory Bridge / XSpice d_cosim Interface   │
                    └─────────────────────────┬─────────────────────────┘
                                              │
                                              ▼
                                 ┌─────────────────────────────────┐
                                 │     NGSpice Analog Simulator    │
                                 └─────────────────────────────────┘
```

---

## Quick Start Guide

### 1. Prerequisites

* **CMake** 3.25 or higher
* **C++20 Compatible Compiler** (GCC 11+, Clang 14+, MSVC 2022+)
* **Ninja Build System** (`ninja-build`)
* **Python 3**

### 2. Build & Installation

```bash
# Clone the repository
git clone https://github.com/Janadasroor/VioAVR.git
cd VioAVR

# Configure & Build using Ninja
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### 3. Usage Examples

```bash
# 1. Run a compiled firmware image
./build/apps/vioavr/vioavr run firmware.hex --mcu ATmega328P

# 2. Trace instruction execution cycle-by-cycle
./build/apps/vioavr/vioavr trace firmware.hex --mcu ATmega328P --max-cycles 100

# 3. Launch interactive CLI Debugger REPL
./build/apps/vioavr/vioavr debug firmware.hex --mcu ATmega328P

# 4. Search built-in terminal documentation
./build/apps/vioavr/vioavr docs overview
./build/apps/vioavr/vioavr docs evsys

# 5. Start GDB server for source-level debugging
./build/apps/vioavr/vioavr gdb firmware.hex --mcu ATmega328P --port 1234
# In another terminal: gdb-multiarch -ex 'target remote :1234' firmware.elf

# 6. Run performance benchmark
./build/apps/vioavr/vioavr benchmark --cycles 100000000
```

---

## Project Structure

```
├── include/vioavr/core/     # Public C++20 core API & device descriptors
├── src/core/                # Simulation engine & x86-64 JIT implementation
├── apps/
│   └── vioavr/              # Unified CLI tool (run, trace, debug, docs, gdb, bridge)
├── cosim/                   # NGSpice co-simulation shim & analog bridge models
├── tests/                   # Core unit tests & mixed-signal co-simulation tests
├── tools/                   # Microchip ATDF device descriptor auto-generator
└── avr-pack/                # Official Microchip ATDF device family packs
```

---

## Contributing

We welcome community contributions! Please read our [CONTRIBUTING.md](CONTRIBUTING.md) guide before submitting pull requests or issues.

---

## License

VioAVR is dual-licensed:

* **Open-Source License**: Released under the **GNU General Public License v3.0 (GPLv3)**. See [LICENSE](LICENSE) for details.
* **Commercial License**: Available under proprietary terms for commercial/closed-source embedding. Contact **janadasroor@gmail.com** for licensing inquiries.

---

*VioAVR is part of the VIOSpice ecosystem. Created by Janada Sroor.*
