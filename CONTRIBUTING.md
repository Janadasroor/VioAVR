# Contributing to VioAVR

Thank you for your interest in contributing to **VioAVR**! We welcome contributions from developers, embedded software engineers, hardware designers, and systems programmers.

---

## 📜 Code of Conduct

We are committed to providing a welcoming, professional, and inclusive environment for everyone. Please treat all contributors and maintainers with respect.

---

## 🛠️ Development Setup & Workflow

### 1. Prerequisites

Ensure you have the following installed on your host system:
* **CMake** 3.25 or higher
* **C++20 Compiler**: GCC 11+, Clang 14+, or MSVC 2022+
* **Ninja Build System** (`sudo apt install ninja-build` or `brew install ninja`)
* **Python 3** (for ATDF device descriptor tools)
* *(Optional)* **ngspice** (for co-simulation testing)
* *(Optional)* **avr-gcc** / **avr-libc** (for firmware test compilation)

### 2. Building from Source

```bash
# Clone the repository
git clone https://github.com/Janadasroor/VioAVR.git
cd VioAVR

# Configure build with Ninja generator
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build all targets in parallel
cmake --build build -j$(nproc)
```

### 3. Running Test Suites

VioAVR requires 100% test suite pass rates before any pull request can be merged:

```bash
# Run unit tests and co-simulation test suite
ctest --test-dir build -j$(nproc) --output-on-failure
```

---

## 📐 Architectural Mandates & Guidelines

When submitting code changes, please observe the following core rules:

1. **Hardware Accuracy First**: 
   * Official Microchip ATDF descriptors (cached under `avr-pack/`) are the absolute source of truth for register maps, bitmasks, and interrupt vector offsets.
   * Do not introduce arbitrary hardware magic numbers or hardcode I/O addresses inside core peripheral implementation files (`src/core/*.cpp`). Always pass parameters via `Descriptor` structs.

2. **C++20 Best Practices**:
   * Maintain clean C++20 conventions: prefer `std::span`, `constexpr`, explicit bitwise typing (`u8`, `u16`, `u32`), and zero-allocation dynamic execution paths where possible.

3. **Performance Preservation**:
   * The x86-64 JIT engine (`avr_jit.cpp`) is tuned for high-throughput execution. Ensure changes inside peripheral tick loops (`tick_peripherals()`) do not degrade core simulation throughput.

---

## 🔀 Submitting a Pull Request (PR)

1. **Fork the Repository**: Create your feature branch off `main` (`git checkout -b feature/amazing-feature`).
2. **Commit Changes**: Use descriptive commit messages (e.g., `core: implement TCA split mode timer logic`).
3. **Verify Build & Tests**: Run `ctest` across all targets to ensure zero regressions.
4. **Open a PR**: Target the `main` branch of `Janadasroor/VioAVR` with a clear description of the feature or bug fix.

---

## 💼 Commercial Contributions & Licensing

VioAVR is released under a **GPLv3 / Commercial Dual-License**. 

* Open-source contributions are licensed under GNU General Public License v3.0 (GPLv3).
* If you have questions regarding commercial licensing or embedding VioAVR into proprietary software, please reach out directly to **janadasroor@gmail.com**.
