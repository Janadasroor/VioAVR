# VioAVR Architecture

## Directory Layout

```text
VioAVR/
├── CMakeLists.txt
├── TODO.md
├── apps/
│   └── vioavr-cli/
├── docs/
│   └── ARCHITECTURE.md
├── include/
│   └── vioavr/
│       ├── core/
│       └── qt/
├── src/
│   ├── core/
│   └── qt/
└── tests/
    └── core/
```

## Build Split

- `VioAVR::core`
  Zero-Qt ISS library. Owns CPU state, Harvard memory model, peripheral abstraction, image loading, and synchronization hooks.
- `VioAVR::qt`
  Optional Qt6 bridge layer. Adapts core callbacks into signals and later hosts UI-facing synchronization helpers.
- `vioavr_cli`
  Minimal non-Qt runner for smoke tests, headless tracing, and batch workflows.

## Core Design

### `DeviceDescriptor`

- Centralizes MCU-specific capacities and address-map boundaries for `ATmega8` and `ATmega328`.
- Keeps execution code free of scattered chip constants and makes future AVR variants additive instead of invasive.
- Exposes constexpr helpers for SRAM and data-space ranges so reset and bounds logic stay branch-light.
- Now also carries peripheral-layout metadata for the current ADC, Timer0, `INT0`, `UART0`, and `PCINT0` models, so trigger selection, register addresses, masks, and vector slots can be provided by the MCU profile instead of duplicated across tests and peripheral construction.

### `MemoryBus`

- Stores flash as dense `std::vector<u16>` word storage to match AVR fetch granularity.
- Exposes byte-granular program-memory reads on top of word storage so instructions like `LPM` can stay accurate without abandoning cache-friendly flash layout.
- Stores data memory as one contiguous device-sized byte vector covering register file, I/O, extended I/O, and SRAM to keep the hot path linear and cache-friendly while still supporting multiple MCU sizes.
- Provides fixed low-I/O aliasing through the AVR data-space offset, so `IN`/`OUT` and generic data-memory instructions hit the same underlying addresses.
- Routes reads and writes in the memory-mapped I/O ranges to `IoPeripheral` instances.
- Exposes `std::span` views for instrumentation and future trace tooling without copying.

### `AvrCpu`

- Owns the architectural register file as `std::array<u8, 32>`.
- Keeps hot scalar state compact: `PC`, `SP`, `SREG`, cycle counter, and run state.
- Separates `fetch()` from `decode_and_execute()` and uses a compact size-deduced opcode descriptor table so instructions can be added incrementally one handler at a time without desynchronizing a manual table count.
- Talks only to `MemoryBus` and an optional `SyncEngine`.
- Includes explicit 16-bit register-pair helpers for `X`, `Y`, and `Z`, so pointer-based instructions can build on a stable base instead of re-decoding pair semantics ad hoc.
- Current bring-up instruction set covers `NOP`, `LDI`, `MOV`, `MOVW`, `MUL`, `MULS`, `MULSU`, `FMUL`, `FMULS`, `FMULSU`, `COM`, `NEG`, `SWAP`, `ADIW`, `ADD`, `ADC`, `INC`, `ASR`, `LSR`, `ROR`, `SBC`, `SBCI`, `SUB`, `DEC`, `SBIW`, `SUBI`, `CPI`, `ANDI`, `ORI`, `CP`, `CPC`, `CPSE`, `SBRC`, `SBRS`, `BST`, `BLD`, `SER`, `BSET`, `BCLR`, `TST`, `AND`, `OR`, `EOR`, `SBI`, `CBI`, `SBIS`, `SBIC`, `IN`, `OUT`, `LD`, `ST`, `LDS`, `STS`, `LPM`, `SPM`, `PUSH`, `POP`, `RCALL`, `RJMP`, `IJMP`, `JMP`, `ICALL`, `CALL`, `RET`, `RETI`, `SLEEP`, `BREAK`, `WDR`, `SEI`, `CLI`, `BREQ`, `BRNE`, `BRCS`, `BRCC`, `BRMI`, `BRPL`, `BRVS`, `BRVC`, `BRLT`, `BRGE`, `BRHS`, `BRHC`, `BRTS`, `BRTC`, `BRIE`, and `BRID`, with indirect `LD`/`ST` implemented for `X`, `Y`, and `Z` in plain, post-increment, pre-decrement, and displacement forms and `LPM` implemented for `r0`, `Rd,Z`, and `Rd,Z+`. `LSL`, `ROL`, and `CLR` are already validated through their AVR alias encodings on `ADD Rd,Rd`, `ADC Rd,Rd`, and `EOR Rd,Rd`, the multiply family writes its 16-bit product to `r1:r0`, the fractional multiply family applies the AVR left-shifted writeback on top of that product path, and both update only `Z` and `C`. `SBR` and `CBR` fall out naturally as `ORI` and `ANDI` opcode aliases, `BRLO` and `BRSH` already fall out of `BRCS` and `BRCC`, and the other status-bit aliases are handled through generic `BSET`/`BCLR` and `BRBS`/`BRBC` decode paths. `SLEEP` currently moves the core into the explicit `sleeping` state until an interrupt wakes it, but the CPU now owns sleep-time advancement itself: `step()` consumes one sleep cycle, and `run()` continues ticking peripherals while the core is asleep so timer-driven wakeups do not require external bus ticking. `BREAK` moves it into `paused`, `WDR` is present as a cycle-accurate placeholder until watchdog peripheral modeling exists, and `SPM` is modeled as an explicit unsupported flash-write boundary that halts before any program-memory mutation. This is enough to validate ALU state updates, compare/borrow behavior, immediate-mask/compiler-style constant folding patterns, counter-style loop ops, one-byte arithmetic/shift behavior, explicit `SREG` bit manipulation, status-driven branch aliases, `T`-bit transfer semantics, stack movement, 16-bit pair math and pair transfers, signed, mixed-sign, and fractional multiply datapaths, practical SRAM-pointer plumbing, flash-table reads through `Z`, indirect `Z`-based control flow, low-I/O and register-bit skip behavior, and basic interrupt/control-flow behavior without committing to the full decoder yet.

### `IoPeripheral`

- Narrow interface for memory-mapped devices such as GPIO ports, timers, UART, ADC, and interrupt controller logic.
- Each peripheral declares one address range and participates in reset, tick, read, and write events.
- Designed so device models can stay fully independent from Qt and ngspice concerns.
- The first concrete peripheral is a GPIO port model with `PINx`, `DDRx`, and `PORTx` semantics suitable for both `IN`/`OUT` and generic data-space access.
- GPIO peripherals now expose queued pin-state changes when output bits transition, which gives the core a direct path to notify `SyncEngine` without introducing Qt into the emulator library.
- A minimal 8-bit timer peripheral now advances from cycle ticks, exposes `TCNT/OCR/TIFR/TIMSK`-style registers, and raises peekable/consumable compare/overflow interrupt requests only when the corresponding source-enable bits are set. Source flags can still accumulate while masked, and are cleared either by firmware writes to `TIFR` or by hardware-style ISR acceptance.
- `Timer8` can now also be constructed directly from `DeviceDescriptor::timer0`, which keeps register addresses, enable masks, and vector indices sourced from the MCU profile rather than hard-coded at the call site.
- Even the Timer0 tests that intentionally keep the older fixed-rate constructor path now source those register addresses and masks from `DeviceDescriptor::timer0`, so the MCU profile remains the single source of truth for Timer0 layout regardless of which timing mode a test is exercising.
- Within one timer instance, compare currently has priority over overflow when both flags are pending at once; servicing compare clears only `OCF`, leaving `TOV` latched for the next interrupt service. That ordering now has dedicated regression coverage.
- The timer model now also supports optional `TCCR0A/TCCR0B`-style control registers. `CS02:0` clock select semantics cover `stopped`, `/1`, `/8`, `/64`, `/256`, and `/1024`, and the external-clock selections can now tick from a bound GPIO input bit on falling or rising edges. That keeps the implementation narrow but makes co-simulated digital nodes capable of driving timer timebases directly.
- That external-clock path is now covered through the CPU instruction stream as well: firmware can configure `OCR0` and `TCCR0B` with normal AVR register writes, while host-driven GPIO transitions advance the timer and become visible through `IN` reads of `TCNT0` and `TIFR`.
- Waveform handling is still intentionally narrow, but CTC mode is now modeled: `WGM01:0` plus `WGM02` are decoded far enough for clear-on-compare counter reset against `OCR`, which gives the ISS a usable timer mode for many firmware polling and interrupt cases without committing to full PWM/output-compare behavior yet.
- The first timer-to-pin bridge is now present as well: `COM0x` compare-output actions can be bound to a GPIO bit, and compare matches drive that bit through the same queued `PinStateChange` path already used for firmware-originated GPIO writes. That keeps timer-generated edges visible to `SyncEngine` without introducing a second event channel.
- A minimal `UART0` peripheral is now present as a second hardware block. It models `UDR0/UCSR0A/UCSR0B/UCSR0C`, RX byte injection, TX completion after a programmable cycle delay, a host-visible transmitted-byte queue, and UART interrupt requests for RX complete, data-register-empty, and TX complete. The flag model is intentionally narrow but enough to exercise firmware register traffic and interrupt plumbing without bringing in baud-rate divisor or framing fidelity yet.
- That UART model is now covered both from the host side and through the CPU instruction path: firmware-style `STS`/`LDS` accesses can enable RX/TX, write `UDR0`, observe `UCSR0A` state transitions as cycles retire, and read injected bytes back from `UDR0`.
- `Uart0` now also has a device-backed constructor sourced from `DeviceDescriptor::uart0`, so the current USART register layout and interrupt vectors come from MCU data rather than repeated ATmega328 literals in test code.
- A minimal external interrupt peripheral is now present for co-simulation-facing edge injection. The first slice models `INT0` only, with `EICRA`, `EIMSK`, and `EIFR`, plus host-driven input-level changes that can latch low-level, any-change, falling-edge, or rising-edge requests and feed the existing interrupt-dispatch path.
- `ExtInterrupt` now also has a device-backed constructor sourced from `DeviceDescriptor::ext_interrupt`, so the current `INT0` register layout and vector slot come from MCU data instead of ad hoc constants in tests or setup code.
- The thresholded `INT0` and analog-front-end interrupt tests now consume that same descriptor layout too, so external-interrupt address knowledge is no longer duplicated across the co-simulation-facing test surface.
- A first pin-change interrupt model is now present as well. It binds one `PCINT` group to a `GpioPort`, samples the port’s visible pin state, applies `PCMSKx` masking, latches `PCIF` on masked changes, and relies on `PCICR` for interrupt enable. That gives the ISS a direct path from externally driven digital node changes into firmware-visible interrupt behavior.
- `PinChangeInterrupt` now also has a device-backed constructor sourced from `DeviceDescriptor::pin_change_interrupt_0`, so the current `PCINT0` register layout, enable/flag masks, and vector slot come from the MCU profile instead of duplicated setup constants.
- A minimal ADC bridge is now present too. The first slice models `ADMUX`, `ADCSRA`, `ADCL`, and `ADCH`, starts conversions from `ADSC`, completes them after a programmable cycle budget, maps host-set normalized analog channel values onto 10-bit results, and raises conversion-complete state through `ADIF` and the ADC interrupt path. This is intentionally narrow but establishes the first analog-to-digital handoff point for VIOSpice-style co-simulation.
- That ADC path now has a first auto-trigger hook as well: a bound analog comparator can start conversions from comparator-output transitions when `ADEN|ADATE` are set. This is still intentionally narrower than the full AVR trigger-mux model, but it establishes the first internal mixed-signal trigger path between peripherals.
- The auto-trigger path now has a second source too: `Timer8` compare matches can notify a bound ADC and start conversions when `ADEN|ADATE` are set. This is still short of a full AVR trigger-mux implementation, but it proves the emulator can coordinate timer-driven sampling internally rather than only from host-side events.
- Those current auto-trigger paths now route through an explicit ADC-side trigger-select register rather than ad hoc direct callbacks. The ADC still owns the `ADTS` selector state locally, but the meaning of each selector slot now resolves through MCU descriptor data instead of being hard-coded entirely inside the peripheral. For the current ATmega8/328 profiles, the implemented sources are free-running, comparator, `INT0`, timer compare, and timer overflow, while the remaining slots are reserved in the descriptor map for future timer-driven expansion.
- `ADTS=0` is now active as well: free-running mode restarts conversions from the ADC’s own completion boundary rather than from another peripheral, so the selector no longer has a “defined but inert” value in the current model.
- A minimal analog comparator bridge is now present as well. The first slice models `ACSR`, accepts host-set normalized `AIN0` and `AIN1` voltages, derives `ACO` from their differential with a small hysteresis band, and raises comparator interrupts for toggle/falling/rising modes. This gives the co-simulation boundary a second analog-facing path beyond the ADC without committing to the full analog multiplexer, bandgap, or timer-capture coupling yet.
- Analog-facing peripherals can now also bind to a shared `AnalogSignalBank`, so one host-owned set of normalized voltages can drive multiple consumers consistently. The current users are the ADC channel array and the analog comparator’s `AIN0/AIN1` inputs, which avoids duplicating voltage state when both need to observe the same co-simulated node values.
- That shared front-end now reaches digital consumers too: `GpioPort` can bind individual input bits to `AnalogSignalBank` channels through the existing Schmitt-threshold model, and `ExtInterrupt` can bind `INT0` to a bank channel using the same threshold/hysteresis path. This gives VIOSpice a single host-owned analog source object that can influence ADC results, comparator output, GPIO sampling, pin-change interrupts, and external interrupt edges without separate per-peripheral voltage stores.
- Because `Timer8` external clocking samples a bound `GpioPort` bit, that same front-end now also reaches timer timebases indirectly: an `AnalogSignalBank` voltage can cross the GPIO threshold model, produce digital edges, and clock `CS=110/111` timer modes without a separate analog-to-timer path.

### `SyncEngine`

- Boundary between emulator time and external simulation time.
- Receives cycle advancement notifications and pin state changes.
- Will later implement blocking or cooperative synchronization with VIOSpice transient analysis.

### Interrupt Groundwork

- `MemoryBus` now supports non-destructive interrupt observation and destructive interrupt consumption, which keeps vector identity intact without forcing the bus itself to become an interrupt controller.
- When multiple peripherals are requesting service at once, `MemoryBus` now selects the pending interrupt with the lowest vector index instead of relying on peripheral attachment order. That gives the current model a deterministic approximation of AVR priority without introducing a full central interrupt controller yet.
- `AvrCpu` services pending requests before fetch when `SREG.I` is set, pushes the return `PC` onto the AVR stack, clears `I`, jumps to the vector word address, and tracks ISR nesting depth for snapshots.
- `RETI` restores the saved `PC`, re-enables global interrupts, and drops interrupt-handler depth back toward the mainline path.
- With multiple latched sources, the current CPU path now has explicit regression coverage for interrupt chaining: once `RETI` restores `I`, the next pending request is serviced on the following step before the interrupted mainline instruction is fetched again.
- Timer-driven interrupt delivery now uses explicit compare/overflow vector slots instead of the earlier implicit `vector+1` shortcut, which lets peripheral models match the actual MCU vector table more closely.
- This is still intentionally minimal: priority nuances beyond bus scan order, a shared interrupt-controller model, and broader ATmega8/328 vector-map coverage remain follow-on work.

## Recommended Growth Pattern

- Add instruction handlers as small pure functions or member helpers keyed by opcode masks.
- Keep AVR-family configuration data in constexpr descriptors instead of scattering ATmega8/328 constants through execution code.
- Reserve Qt for observation, visualization, and thread handoff. Do not let core timing or bus code depend on Qt types.
- Keep HEX parsing and image validation device-aware so invalid firmware is rejected before execution starts.
