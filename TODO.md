# VioAVR Master TODO

## Current Status: 73/73 tests passing (100%) ✅

### All Session Fixes Applied
- [x] Interrupt vector address: bytes→words conversion
- [x] Test navigation: cpu.run(N) → step_to() in 34 files
- [x] Device headers: regenerated from ATDF
- [x] UART timing: transmission and flags
- [x] Timer prescaler: cycle accumulation
- [x] Timer CTC: compare match handling
- [x] GPIO: spurious DDR fixes
- [x] ADC: comparator notification
- [x] PCI addresses: datasheet values
- [x] SPI/TWI/UART interrupts
- [x] Ext interrupt: vector index
- [x] Watchdog: ISR address
- [x] SimAVR comparison pipeline created

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
