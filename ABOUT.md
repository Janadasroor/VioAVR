# About VioAVR & VIOSpice

## The Problem
Simulating an embedded system traditionally happens in two separate silos:
1. **Digital Simulation:** Tools like GDB or simple ISS runners that simulate the CPU but ignore the external circuit.
2. **Analog Simulation:** Tools like LTspice or ngspice that simulate the circuit but use a simple "voltage source" to mimic the MCU.

In the real world, an AVR microcontroller interacts dynamically with analog components—for example, a PWM signal driving an H-bridge motor controller or an ADC reading a noisy sensor.

## The Solution: Mixed-Mode Co-Simulation
**VIOSpice** aims to be an open-source alternative to Proteus, enabling developers to simulate the entire system at once.

**VioAVR** is the high-performance digital engine for mixed-signal ecosystem. Its primary design goal is to run in "lock-step" with an analog SPICE solver like **ngspice**.

### Key Architecture for Co-Simulation:

1. **Cycle-Accurate Ticking:** VioAVR is designed to step forward by a precise number of cycles (or even fractions of time) as dictated by the analog solver's time step.
2. **The `SyncEngine`:** This is the communication channel between VioAVR and the outside world. It reports digital events (like a pin changing from 0 to 1) so the analog solver can react immediately.
3. **Thresholded Inputs:** VioAVR supports "Analog Signal Banks" that allow the digital chip to "sample" analog voltages and apply hysteresis-based thresholds, just like real CMOS input circuitry.
4. **Device Descriptor Map:** By parsing official `.atdf` files from the AVR Device Family Pack (DFP), VioAVR ensures that its memory map and register behavior exactly match the physical chip, providing high-fidelity simulation for any AVR variant.

## Design Philosophy
- **Minimalist:** The core engine is kept free of GUI and complex OS dependencies to ensure maximum performance.
- **Explicit over Implicit:** All MCU-specific data is centralized in `DeviceDescriptor` structures, avoiding magic constants in the code.
- **Traceable:** Built-in support for instruction tracing and peripheral logging to help developers debug their firmware *and* their circuits simultaneously.

## Integration Roadmap
- [x] AVR Core CPU (most opcodes)
- [x] Basic Peripheral Set (Timers, GPIO, UART, ADC)
- [x] Device Family Pack (DFP) parsing
- [ ] XSPICE / ngspice bridge implementation
- [ ] VHDL/Verilog-style port mapping
- [ ] Full Proteus-style schematic interaction

---
*Created by Janada Sroor. VioAVR is the bridge between the logic of code and the physics of the circuit.*
