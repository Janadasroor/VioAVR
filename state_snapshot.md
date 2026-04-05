# state_snapshot

## overall_goal
Integrate VioAVR with ngspice (via XSPICE) to enable cycle-accurate mixed-mode co-simulation of AVR chips using official binary firmware.

## active_constraints
- C++20 standard required.
- High-performance, minimalist core with cycle-accurate instruction execution.
- Automated hardware specification extraction from official Microchip `.atdf` DFP files.
- Thread-safe synchronization between the ISS thread and the analog simulation clock via `ISyncPolicy`.
- Pin mapping must support bi-directional digital/analog propagation.

## key_knowledge
- **VioSpice Bridge:** The `VioSpice` class (C++) and `viospice_c` wrapper (C) unify `AvrCpu`, `MemoryBus`, and `SyncEngine`.
- **Interrupt Handling:** Fixed `AvrCpu` to use `vector_index * interrupt_vector_size` for vectoring. Default size is 2 (JMP-based vectors).
- **Device Metadata:** Restored and improved `scripts/gen_device.py`. It now extracts memory sizes, vectors, peripheral addresses, and GPIO port configurations (A..H) from official ATDF files.
- **Port Mapping:** `DeviceDescriptor` now includes a `ports` array (up to 8 ports), which `VioSpice` uses to dynamically initialize `GpioPort` instances.
- **Analog-to-Digital:** `VioSpice` now includes an `AnalogSignalBank` bound to the `Adc` peripheral, allowing external voltages to be sampled by the firmware.

## artifact_trail
- `include/vioavr/core/device.hpp`: Added `PortDescriptor` and `ports` array to `DeviceDescriptor`.
- `scripts/gen_device.py`: Fully restored and generalized to handle multiple ATDF formats and 16-bit registers (TCNT1, EEAR).
- `include/vioavr/core/viospice.hpp` & `src/core/viospice.cpp`: Added `AnalogSignalBank` member, exposed `set_external_voltage`, and implemented dynamic GPIO initialization.
- `viospice/viospice_bridge.cpp`: Created a manual XSPICE bridge (pending header availability).
- `scripts/build_spice_model.sh`: Created automated build script for XSPICE model.

## task_state
1. [DONE] Support ATmega328P via improved automated extraction script.
2. [DONE] Fix interrupt vectoring logic in AvrCpu.
3. [DONE] Verify full interrupt/timer/GPIO lifecycle with firmware.
4. [DONE] All 73 tests passing (100%) - up from 40% at session start

### Test Progress Summary
- Start: 27/67 (40%)
- End: 73/73 (100%)
- Critical fixes: interrupt vector address (bytes→words), test navigation patterns, device headers, UART/Timer/ADC timing
- SimAVR comparison pipeline created for validation
4. [DONE] Add support for analog-to-digital mapping (SPICE voltage to AVR ADC levels) within `VioSpice`.
5. [DONE] Implement robust GPIO metadata extraction and dynamic initialization in bridge.
6. [DONE] Rebuild all apps and tests successfully.
7. [BLOCKED] Implement automated build for `.cm` (Missing `ngspice/cm.h` in environment).
