# Contributing to VioAVR

Thank you for your interest in **VioAVR**! I am building a high-performance AVR simulation engine for mixed-mode co-simulation, and contributions are much appreciated.

## Code of Conduct

As a contributor, please be respectful and professionally engaged.

## How to Contribute

### 1. Reporting Bugs
Please use GitHub Issues and include:
-   **Device Model** (e.g., ATmega328P).
-   **Firmware Fragment** or a `.hex` file.
-   **Expected Behavior** vs **Actual Results**.
-   **Cycle Traces** (if available) using `vioavr-cli --trace`.

### 2. Feature Requests
If you'd like to see a new peripheral modeled, feel free to open an issue or start a discussion.

### 3. Pull Requests
1.  **Fork** the repository and create a new branch.
2.  Follow the **C++17 Coding Style** (ClangFormat-style).
3.  **Add Unit Tests** for any new instruction or peripheral.
4.  Ensure all 60+ existing tests pass (`ctest`).
5.  Submit your PR with a clear description of the change.

## Coding Standards

-   **Namespace**: All core code belongs in `vioavr::core`.
-   **Types**: Use the fixed-width aliases from `types.hpp` (e.g., `u8`, `u16`, `u32`).
-   **No Virtual Dispatch**: In hot execution paths, avoid virtual methods to maintain performance.
-   **Documentation**: Header files should use Doxygen-style comments for public APIs.

## Project Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
ctest
```

---
*Created by Janada Sroor. VioAVR is part of the Viospice ecosystem.*
