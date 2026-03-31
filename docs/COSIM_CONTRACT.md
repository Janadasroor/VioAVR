# VioAVR Co-Simulation Contract

This document defines the technical contract between the VioAVR Instruction Set Simulator (ISS) and external simulators (primarily ngspice and Qt-based UI).

## 1. Timing and Synchronization

### 1.1 Cycle-to-Time Mapping
- The simulation assumes a fixed AVR clock frequency (default: 16 MHz).
- One AVR cycle corresponds to exactly `1.0 / Frequency` seconds (e.g., 62.5 ns @ 16 MHz).
- The `SyncEngine` is responsible for yielding execution to the host at specific synchronization points.

### 1.2 Synchronization Ownership (The "Master" Loop)
- **VioAVR as Master**: The ISS runs in its own thread and "pauses" at regular intervals (defined by `ISyncPolicy`) to allow the analog simulator to catch up.
- **ngspice as Master**: (To be evaluated) ngspice calls into the ISS via a shared library (XSPICE) to request a specific amount of time advancement.
- **Decision**: VioAVR currently implements a **Hybrid Pull/Push** model via the `SyncEngine`. The ISS pushes cycle updates and pin changes to the bridge, and the bridge can pull the ISS into a pause state.

### 1.3 Quantum and Drift
- A "Quantum" is the maximum number of AVR cycles the ISS is allowed to run before forced synchronization.
- **Drift**: Pin changes occurring *within* a quantum are timestamped with the exact cycle count. The analog simulator must decide whether to rewind or interpolate these events.
- **VioAVR Guarantee**: All `PinStateChange` events are emitted with precise cycle timestamps relative to the start of the simulation.

## 2. Pin State and Analog Mapping

### 2.1 Digital-to-Analog (Output)
- AVR Digital Low (0) -> Mapping defined by bridge (e.g., 0V).
- AVR Digital High (1) -> Mapping defined by bridge (e.g., 5V or 3.3V).
- The bridge is responsible for converting `PinStateChange` events into ngspice "event-driven" sources or voltage steps.

### 2.2 Analog-to-Digital (Input)
- **Thresholding**: VioAVR peripherals (GPIO, INT, Timer) use a normalized threshold model (0.0 to 1.0).
- **Hysteresis**: (Planned) Implement a Schmitt-trigger model in the `AnalogSignalBank` to prevent rapid oscillations at the threshold.
- **Sampling**: Input signals are sampled at the start of each AVR cycle.

## 3. Threading and Memory Safety

### 3.1 Pin Update Strategy
- **Outputs**: `PinStateChange` events are buffered in the `SyncEngine` and consumed by the bridge. This is a single-producer (ISS), single-consumer (Bridge) relationship.
- **Inputs**: External writes to `AnalogSignalBank` must be thread-safe. A `std::atomic` or `std::mutex` will protect the shared voltage levels.

### 3.2 Command/Control Path
- The `SyncEngine::resume()` and `SyncEngine::wait_until_resumed()` provide the primary handshake for pausing the ISS for debugging or analog convergence.
