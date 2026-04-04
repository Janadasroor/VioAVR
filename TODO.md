# VioAVR Master TODO

## Current Status: 31/67 tests passing (46%)

### Fixes Applied
- [x] Device header generation from ATDF (flash_words, sram_bytes corrected)
- [x] CPU bounds check: halt when PC >= loaded_program_words
- [x] cpu_alu_test.cpp: step_to() pattern replaces cpu.run(100)
- [x] cpu_word_ops_test.cpp: step_to() pattern replaces cpu.run(N)
- [x] Real AVR C programs in tests/programs/ (compiled with avr-gcc -j8)

### Remaining: 36 Failing Tests
- [ ] cpu_small_ops_test - wrong cpu.run(N) cycle counts
- [ ] cpu_compare_test - wrong cpu.run(N) cycle counts
- [ ] cpu_stack_test - wrong cpu.run(N) cycle counts
- [ ] cpu_fmul_test - wrong cpu.run(N) cycle counts
- [ ] cpu_interrupt_test - wrong cpu.run(N) cycle counts
- [ ] cpu_interrupt_priority_test - wrong cpu.run(N) cycle counts
- [ ] cpu_interrupt_chaining_test - wrong cpu.run(N) cycle counts
- [ ] cpu_ext_interrupt_test - wrong cpu.run(N) cycle counts
- [ ] cpu_ext_interrupt_firmware_test - wrong cpu.run(N) cycle counts
- [ ] cpu_pin_change_interrupt_test - wrong cpu.run(N) cycle counts
- [ ] cpu_pin_change_interrupt_firmware_test - wrong cpu.run(N) cycle counts
- [ ] cpu_uart_test - wrong cpu.run(N) cycle counts
- [ ] cpu_uart_interrupt_test - wrong cpu.run(N) cycle counts
- [ ] cpu_uart_firmware_test - wrong cpu.run(N) cycle counts
- [ ] cpu_timer_pwm_test - wrong cpu.run(N) cycle counts
- [ ] cpu_timer_control_test - wrong cpu.run(N) cycle counts
- [ ] cpu_timer_dual_interrupt_test - wrong cpu.run(N) cycle counts
- [ ] cpu_timer_external_clock_firmware_test - wrong cpu.run(N) cycle counts
- [ ] cpu_watchdog_test - wrong cpu.run(N) cycle counts
- [ ] cpu_sleep_wake_test - wrong cpu.run(N) cycle counts
- [ ] cpu_io_test - wrong cpu.run(N) cycle counts
- [ ] cpu_gpio_sync_test - wrong cpu.run(N) cycle counts
- [ ] cpu_voltage_interrupt_test - wrong cpu.run(N) cycle counts
- [ ] cpu_bit_io_test - wrong cpu.run(N) cycle counts
- [ ] cpu_branch_alias_test - wrong cpu.run(N) cycle counts
- [ ] cpu_extended_load_store_test - wrong cpu.run(N) cycle counts
- [ ] cpu_load_store_test - wrong cpu.run(N) cycle counts
- [ ] cpu_multibyte_compare_test - wrong cpu.run(N) cycle counts
- [ ] cpu_register_mapping_test - wrong cpu.run(N) cycle counts
- [ ] cpu_adc_* tests (7 files) - wrong cpu.run(N) cycle counts
- [ ] cpu_analog_* tests (3 files) - wrong cpu.run(N) cycle counts

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
