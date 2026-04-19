# VioSpice: High-Fidelity XSpice Bridge for VioAVR

VioSpice is an XSpice code model that allows you to simulate your AVR firmware within an `ngspice` circuit simulation with cycle-accurate precision and analog signal fidelity.

## Features
- **Shared Memory Bridge**: Low-latency communication between SPICE and the VioAVR emulator.
- **Sub-cycle Synchronization**: Fractional time-step accumulation for perfect timing even with adaptive SPICE solvers.
- **Analog-Digital Hybrid**: Every pin supports both logic states and continuous voltages.
- **Schmitt-Trigger Emulation**: Configurable $V_{IH}$ and $V_{IL}$ thresholds per pin.
- **Multi-Peripheral Support**: Mapped ADC sampling and PWM driving directly to SPICE nodes.

## Port Definition

| Port Name | Direction | Type | Description |
| :--- | :--- | :--- | :--- |
| `avr_pins` | InOut | Digital | Logical digital pins (Port B, C, D maps). |
| `avr_analog_pins` | In | Voltage | Continuous analog inputs for ADC or Logic Thresholding. |

## Parameters

| Name | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `hex_file` | String | "firmware.hex" | Path to the AVR binary to load. |
| `mcu_type` | String | "atmega328" | The target MCU model (must match daemon). |
| `frequency` | Real | 16e6 | CPU Clock frequency in Hz. |
| `quantum_cycles`| Int | 1000 | Max cycles between SPICE/ISS synchronization. |

## Usage Guide

### 1. Build the Plugin
Run the provided build script:
```bash
./scripts/build_spice_model.sh
```
This produces `build/viospice.cm`.

### 2. Start the VioAVR Bridge Daemon
The XSpice model acts as a client. You must have a server daemon running to host the emulator core:
```bash
./build/apps/vioavr-bridge-daemon/vioavr-bridge-daemon --mcu atmega4809 --instance "my_mcu"
```

### 3. Example Netlist (`test.cir`)
```spice
* VioAVR Simulation Demo
.load /path/to/VioAVR/build/viospice.cm

* Circuit: An R-C filter connected to Port D, Bit 1 (mapped to external ID 0)
V1 1 0 PULSE(0 5 1ms 1us 1us 5ms 10ms)
R1 1 2 10k
C1 2 0 1uF

* VioAVR Instance
A_AVR [d0 d1] [2] d_mcu
.model d_mcu d_vioavr(mcu_type="atmega4809" frequency=16e6 hex_file="test.hex")

* Monitor C1 voltage (node 2) mapped to avr_analog_pins[0]
.tran 1us 50ms
.plot v(2)
```

## Advanced: Logic Thresholds
Thresholds are $V_{DD}$ dependent and can be configured via the SHM client or set to defaults (3.0V / 1.5V). When a voltage on an analog port exceeds $V_{IH}$, the corresponding digital pin in the AVR is set HIGH. When it falls below $V_{IL}$, it is set LOW.
