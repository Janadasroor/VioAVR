# VioAVR Master TODO

## Incoming: Phase 4 & Beyond

VioAVR is now in **Beta**. The core simulator is stable, and the focus is shifting towards deep integration with **ngspice** and **XSpice**.

### Phase 4: The Co-Simulation Bridge
- [ ] **XSpice Plugin API**: Finalize the C interface between `SyncEngine` and `ngspice`.
- [ ] **Shared Memory Bridge**: Optimize high-speed pin state exchange for large-scale transients.
- [ ] **Interrupt Latency Validation**: Ensure ISR entry/exit cycle budgets match exact hardware silicon.
- [ ] **Proprietary Simulation Hooks**: Option to load Microchip's `.so` models for comparison.
- [ ] **VCD Tracing**: Native support for exporting signal logs to Value Change Dump (.vcd) format.

### Phase 5: Ecosystem & Tooling
- [ ] **GDB Server**: Add remote debugging support so VioAVR can be a backend for GDB.
- [ ] **WebAssembly (Wasm)**: Compile the core with Emscripten for in-browser AVR simulation demos.
- [ ] **MCU Coverage Expansion**:
    - [ ] 32-bit AVR core (AVR32) baseline support.
    - [ ] UPDI / PDI programming protocol emulation.
- [ ] **Documentation**: Formalize `ARCHITECTURE.md` with clock-tree and pipeline diagrams.

---

### Recently Completed (See [CHANGELOG.md](CHANGELOG.md))
- [x] Automated Device Catalog (135+ MCUs).
- [x] Full Peripheral Suite (UART, ADC, Timers, SPI, TWI).
- [x] Cycle-accurate ALU core.
- [x] Intel HEX loader.
- [x] Mixed-mode `SyncEngine`.
- [x] Unit Test Suite (60+ tests).
