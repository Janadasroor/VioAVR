# ATmega328P Completion Roadmap

This file is the execution checklist for making `ATmega328P` behave like a full-chip target rather than a partial AVR ISS profile.

## Source Status Legend

- `repo-verified`: confirmed from the current VioAVR codebase structure or behavior surface
- `atdf/header-verified`: confirmed from `ATmega328P.atdf` and/or `iom328p.h` in `atmega/`
- `needs-datasheet`: plausible and important, but not yet audited against the Microchip datasheet in this pass
- `implementation-target`: an engineering task derived from simulator scope, not a factual claim about current hardware data

## Goal

Reach a level where common `ATmega328P` firmware behaves predictably and close to hardware, with enough fidelity for:

- headless firmware validation
- differential testing against `simavr`
- eventual VIOSpice mixed-signal co-simulation

## Scope Rule

Until this file is mostly complete, `ATmega328P` is the priority device. New MCU expansion should not outrun correctness on this chip.

## Phase 0: Descriptor And Composition Baseline

- [x] Verify core `ATmega328P` descriptor fields against local ATDF/header
  Status: `atdf/header-verified`
  Files: `include/vioavr/core/devices/atmega328p.hpp`, `scripts/gen_device.py`, `atmega/atdf/ATmega328P.atdf`
- [x] Confirm flash, SRAM, EEPROM, and vector count/ordering
  Status: `atdf/header-verified`
- [x] Confirm IO ranges, `SPL`, `SPH`, and `SREG` addresses explicitly in generated/runtime use
  Status: `atdf/header-verified`
- [x] Extend generation to capture reset values for key registers where practical
  Status: `atdf/header-verified`
- [x] Extend generation to capture more chip metadata needed by runtime models
  Status: `atdf/header-verified`
  Examples: pin roles, timer output pins, capture pin, reset pin, sleep capabilities, boot/reset metadata, RWW boundaries, signature/fuses/lockbits
- [x] Remove hard-coded `ATmega328P` peripheral addresses from CLI setup and rely on descriptor-backed construction
  Status: `repo-verified`
  Files: `apps/vioavr-cli/main.cpp`
- [x] Build one canonical `ATmega328P` machine factory used by tests, CLI, and embedding APIs
  Status: `repo-verified`
  Files: `src/core/machine.cpp`, `include/vioavr/core/machine.hpp`
  Candidate files: `src/core/`, `include/vioavr/core/`

## Phase 1: Reset, Boot, Sleep, And Interrupt Correctness

- [ ] Distinguish reset causes
  Status: `atdf/header-verified`
  Targets: power-on reset, external reset, brown-out reset, watchdog reset
- [ ] Model reset-cause flags and expose them through the correct MCU control/status register path
  Status: `needs-datasheet`
- [ ] Verify reset state for CPU, stack pointer, peripheral registers, and interrupt state
  Status: `needs-datasheet`
- [ ] Replace generic sleep handling with real `ATmega328P` sleep modes
  Status: `needs-datasheet`
  Modes: idle, ADC noise reduction, power-down, power-save, standby if supported by current scope
- [ ] Implement wake-up rules per interrupt/peripheral source
  Status: `needs-datasheet`
- [ ] Validate `SEI` latency semantics and pending-interrupt servicing timing
  Status: `needs-datasheet`
- [ ] Validate `RETI`, nested servicing boundaries, and post-ISR resume behavior
  Status: `repo-verified`
- [ ] Verify interrupt priority and vector dispatch against the real `ATmega328P` vector table
  Status: `atdf/header-verified`
  Files: `src/core/avr_cpu.cpp`, `src/core/memory_bus.cpp`, `src/core/device_catalog.cpp`
- [ ] Add focused interrupt timing regression tests
  Status: `implementation-target`
  Files: `tests/core/`

## Phase 2: Timer Fidelity

### Timer0

- [ ] Complete all `Timer0` waveform generation modes used on `ATmega328P`
  Status: `needs-datasheet`
- [ ] Validate compare match and overflow flag timing
  Status: `needs-datasheet`
- [ ] Validate `OC0A` and `OC0B` output pin behavior
  Status: `atdf/header-verified`
- [ ] Validate prescaler and external clock edge behavior
  Status: `atdf/header-verified`
- [ ] Validate buffering and register update timing where applicable
  Status: `needs-datasheet`
  Files: `src/core/timer8.cpp`, `include/vioavr/core/timer8.hpp`

### Timer1

- [ ] Complete `Timer1` normal, CTC, fast PWM, and phase-correct/phase-frequency-correct PWM behavior as applicable
  Status: `needs-datasheet`
- [ ] Implement and validate input capture on `ICP1`
  Status: `atdf/header-verified`
- [ ] Validate `OC1A` and `OC1B` output pin behavior
  Status: `atdf/header-verified`
- [ ] Validate compare, overflow, and capture interrupt timing/ordering
  Status: `atdf/header-verified`
- [ ] Validate TOP selection rules and buffered register update semantics
  Status: `needs-datasheet`
  Files: `src/core/timer16.cpp`, `include/vioavr/core/timer16.hpp`

### Timer2

- [ ] Add `Timer2` model if missing from runtime composition
  Status: `repo-verified`
- [ ] Implement normal, CTC, and PWM behavior
  Status: `atdf/header-verified`
- [ ] Implement interrupt behavior and register semantics
  Status: `atdf/header-verified`
- [ ] Leave asynchronous crystal-specific accuracy as a later subtask if needed, but do not block core `Timer2` support on it
  Status: `atdf/header-verified`

## Phase 3: GPIO, Pin Map, And Alternate Functions

- [ ] Promote the chip from “ports only” to a package/pin-aware model
  Status: `repo-verified`
- [ ] Define per-pin mappings for `PORTB`, `PORTC`, and `PORTD`
  Status: `atdf/header-verified`
- [ ] Model alternate function ownership and conflicts
  Status: `atdf/header-verified`
  Examples: UART, SPI, TWI, timer outputs, ADC inputs, `INT0`, `INT1`
- [ ] Model pull-ups, tri-state behavior, and reset-pin semantics
  Status: `needs-datasheet`
- [ ] Bind peripheral outputs through a unified pin-mux layer instead of ad hoc direct wiring
  Status: `implementation-target`
  Files: `include/vioavr/core/pin_map.hpp`, `src/core/gpio_port.cpp`, peripheral source files

## Phase 4: Serial And Bus Peripherals

### UART0

- [ ] Validate baud-rate divisor handling and frame timing
  Status: `needs-datasheet`
- [ ] Model transmit shift-register timing separately from register writes if not already done
  Status: `needs-datasheet`
- [ ] Validate RX complete, TX complete, and UDRE flag/interrupt behavior
  Status: `atdf/header-verified`
- [ ] Add firmware-level tests using realistic serial traffic
  Status: `implementation-target`
  Files: `src/core/uart0.cpp`, `tests/core/`

### SPI

- [ ] Complete master-mode transfer timing
  Status: `needs-datasheet`
- [ ] Validate `SPIF`, `WCOL`, and interrupt behavior
  Status: `atdf/header-verified`
- [ ] Bind MOSI/MISO/SCK/SS through the chip pin model
  Status: `atdf/header-verified`
- [ ] Add slave-mode support if required by target workloads
  Status: `implementation-target`
  Files: `src/core/spi.cpp`

### TWI

- [ ] Implement the AVR TWI state machine to a firmware-usable level
  Status: `needs-datasheet`
- [ ] Validate status-code progression, START/STOP handling, ACK/NACK behavior, and interrupt flow
  Status: `needs-datasheet`
- [ ] Bind SDA/SCL through the chip pin model
  Status: `atdf/header-verified`
  Files: `src/core/twi.cpp`

## Phase 5: Analog And Non-Volatile Blocks

### ADC

- [ ] Validate `ADMUX`, `ADCSRA`, `ADCSRB`, result latching, and conversion timing
  Status: `atdf/header-verified` for register presence, `needs-datasheet` for behavior
- [ ] Support `ATmega328P` channel selection and reference behavior to a practical level
  Status: `atdf/header-verified` for channel pins, `needs-datasheet` for electrical/reference behavior
- [ ] Validate auto-trigger sources against descriptor data
  Status: `needs-datasheet`
- [ ] Bind ADC channels through the chip pin model
  Status: `atdf/header-verified`
  Files: `src/core/adc.cpp`

### Analog Comparator

- [ ] Validate comparator interrupt modes and output state semantics
  Status: `atdf/header-verified` for presence, `needs-datasheet` for behavior
- [ ] Verify any intended coupling to ADC auto-trigger and timer capture paths
  Status: `needs-datasheet`
  Files: `src/core/analog_comparator.cpp`

### EEPROM

- [ ] Validate write timing, busy behavior, and interrupt semantics
  Status: `atdf/header-verified` for register/vector presence, `needs-datasheet` for timing behavior
- [ ] Ensure read/write edge cases match firmware expectations
  Status: `needs-datasheet`
  Files: `src/core/eeprom.cpp`

### Watchdog

- [ ] Validate timeout selection, configuration update rules, interrupt-vs-reset mode, and reset behavior
  Status: `atdf/header-verified` for presence/vector, `needs-datasheet` for behavior
  Files: `src/core/watchdog_timer.cpp`

## Phase 6: Fuses, Lock Bits, And Boot Behavior

- [ ] Add fuse model for `ATmega328P`
  Status: `atdf/header-verified` for fuse address-space existence, `needs-datasheet` for semantics
  Minimum: clock source selection, startup behavior, brown-out configuration, boot reset behavior
- [ ] Add boot section sizing and reset-vector redirection where applicable
  Status: `atdf/header-verified` for boot-section presence, `needs-datasheet` for behavior
- [ ] Add lock-bit enforcement for flash write/boot restrictions as needed
  Status: `atdf/header-verified` for lockbit address-space existence, `needs-datasheet` for semantics
- [ ] Decide whether fuse state is configured by API, image metadata, or external config file
  Status: `implementation-target`

## Phase 7: Tooling And Debuggability

- [ ] Add ELF loading with symbol awareness in addition to HEX loading
  Status: `repo-verified`
- [ ] Add machine-readable register/memory/pin dump support
  Status: `implementation-target`
- [ ] Add breakpoints and watchpoints
  Status: `implementation-target`
- [ ] Consider a GDB remote stub after core correctness is stable
  Status: `repo-verified`
  Files: `src/core/hex_image.cpp`, `apps/vioavr-cli/main.cpp`, tracing/debug tooling

## Phase 8: Validation Harness

- [ ] Expand `simavr` differential testing beyond CPU traces
  Status: `repo-verified`
  Files: `tests/programs/compare_simavr.py`, `tests/programs/compare_traces.py`
- [ ] Add datasheet-driven register and timing tests for `ATmega328P`
  Status: `implementation-target`
- [ ] Add firmware-level tests for common workloads
  Status: `implementation-target`
  Examples: `millis()`-style timer usage, UART echo, SPI master transaction, I2C scan, ADC sampling, PWM output, sleep/wake flows
- [ ] Add golden regression traces for interrupt-heavy and peripheral-heavy programs
  Status: `implementation-target`
- [ ] Mark feature-complete checkpoints with explicit pass criteria
  Status: `implementation-target`

## Immediate High-Value Order

If implementation effort must stay focused, do the next work in this order:

1. Interrupt correctness
2. Timer0 and Timer1 fidelity
3. Canonical `ATmega328P` machine factory
4. Pin mux and package-aware pin model
5. UART/SPI/TWI completion
6. ADC/WDT/EEPROM tightening
7. Fuses/boot behavior
8. ELF/debug tooling

## Definition Of “ATmega328P Mostly Done”

Use this as the exit bar before shifting focus to broader MCU coverage:

- [ ] Core interrupt timing is stable under differential tests
- [ ] Timer0, Timer1, and Timer2 run realistic firmware correctly
- [ ] GPIO and alternate-function pins are modeled through a chip pin map
- [ ] UART, SPI, and TWI can run common driver code
- [ ] ADC, EEPROM, and WDT behave well enough for firmware tests
- [ ] Sleep/wake and reset causes are modeled
- [ ] At least one non-trivial `ATmega328P` firmware sample runs end-to-end with expected behavior
