# VioAVR Master TODO

## Current Status: 48/67 tests passing (72%)

### Fixes Applied This Session
- [x] Device header generation from ATDF (flash_words/sram_bytes corrected)
- [x] CPU bounds check: halt when PC >= loaded_program_words
- [x] Test navigation: all cpu.run(N) → step_to() pattern in 34 files
- [x] UART: reset flags, transmission timing, interrupt clearing
- [x] GPIO: removed spurious DDR pin changes
- [x] Timer prescaler: cycle accumulation for prescaler > 1
- [x] Timer CTC mode: compare match vs overflow flag handling
- [x] PCI addresses: corrected to datasheet values (PCICR=0x68, PCIFR=0x3B, PCMSK0=0x6B)
- [x] ADC auto-trigger map: fallback to defaults when device map empty
- [x] Ext interrupt: fixed vector index (1 not 2) and firmware test expectations
- [x] Watchdog: fixed ISR address (vector_index * 2 = 12)
- [x] Compare/multibyte tests: fixed navigation expectations

### Remaining: 19 Failing Tests (Simulator Implementation Bugs)
- [ ] cpu_interrupt_test - Timer interrupt not triggering properly
- [ ] cpu_interrupt_chaining_test - Interrupt chain logic
- [ ] cpu_interrupt_priority_test - Priority logic
- [ ] cpu_pin_change_interrupt_firmware_test - PCI timing
- [ ] cpu_sleep_wake_test - CPU sleep mode
- [ ] cpu_timer_pwm_test - PWM output logic
- [ ] cpu_timer_dual_interrupt_test - Dual interrupt handling
- [ ] cpu_timer_external_clock_firmware_test - External clock sampling
- [ ] cpu_uart_firmware_test - UART CPU integration
- [ ] cpu_uart_interrupt_test - UART interrupt logic
- [ ] cpu_adc_* tests (6 files) - ADC auto-trigger/timing
- [ ] cpu_analog_* tests (3 files) - Analog frontend logic

### Phase 4: Co-Simulation Bridge (Blocked until tests pass)
- [ ] XSpice Plugin API
- [ ] Shared Memory Bridge
- [ ] Interrupt Latency Validation
- [ ] VCD Tracing

### Phase 5: Ecosystem & Tooling (Blocked)
- [ ] GDB Server
- [ ] WebAssembly (Wasm)
- [ ] MCU Coverage Expansion
- [ ] ARCHITECTURE.md

### Debugging Tools Created
- [x] `tests/programs/trace_runner.cpp` - Dumps register trace as CSV
- [x] `tests/programs/compare_traces.py` - Compares VioAVR vs expected
- [x] `tests/programs/simavr_compare.py` - SimAVR comparison framework
