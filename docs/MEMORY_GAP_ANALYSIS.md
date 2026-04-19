# VioAVR Memory & EEPROM Gap Analysis

This document outlines the current state of memory and EEPROM emulation in VioAVR and identifies missing features or inaccuracies when compared to Microchip ATDF specifications and hardware datasheets.

## 1. NVMCTRL (AVR8X / megaAVR-0) Gaps

| Feature | Status | Impact |
| --- | --- | --- |
| **Flash Wait States** | Missing | High frequency performance tests will be over-optimistic. Needs `NVMCTRL.CTRLB` support. |
| **EEPROM Page Writing** | Partial | `EEWP` commands are supported but the hardware page-buffer clear/load cycle is simplified. |
| **Boot Row Protection** | Missing | Lack of enforcement for `BOOTRST` and boot section lockbits. |
| **User Row Access** | Partial | Simulated as data but lacks NVM-specific erase/write protection enforcement. |

## 2. Legacy SPM / EEPROM (AVR Series) Gaps

| Feature | Status | Impact |
| --- | --- | --- |
| **LPM Signature Read** | Missing | Code using `LPM` with `SIGRD` (SPMCSR bit 5) will read flash instead of signature. |
| **LPM Fuse Read** | Missing | Sequence involving `BLBSET` to read fuses via `LPM` is not implemented in `AvrCpu`. |
| **EEPROM Wait states** | Missing | Lack of CPU stalling (4 cycles for read, 2 cycles for write start) as required by datasheet. |
| **EEPROM Protection** | Missing | Support for `EESAVE` fuse (keeping EEPROM after Chip Erase) is not enforced. |

### CPU Stalling during Access (Legacy)
According to the ATmega328P datasheet (and similar legacy models):
- **EEPROM Read**: When the `EERE` bit is set, the CPU is halted for **4 clock cycles** before the next instruction is executed.
- **EEPROM Write**: When the `EEPE` bit is set, the CPU is halted for **2 clock cycles**.

Implementing this requires a feedback mechanism from the `Eeprom` peripheral to the `AvrCpu` to inject wait states via the `MemoryBus`.

## 3. High-Fidelity Memory Bus Accuracy

### Cycle-Locked Access
Newer AVRs (e.g. ATmega4809) have a "Wait State" mechanism for Flash access. When running at 20MHz (5V), 1 wait state might be required.
- **Current implementation**: Always 1-cycle access.
- **Requirement**: `MemoryBus` must check `NVMCTRL.CTRLB` and stall the CPU for the requested number of cycles.

### SPM Section Stalling
- **RWW (Read-While-Write)**: The CPU should be able to execute code from the NRWW section while writing to the RWW section.
- **Stall Logic**: If the CPU attempts to fetch from the RWW section while it is busy, the hardware should stall until the operation completes. VioAVR currently lacks this "Section Stalling" logic.

## 4. External Memory (XMEM) Support

| Feature | Status | Notes |
| --- | --- | --- |
| **Wait State (XMCRA/B)** | Missing | Any configuration of wait states for external SRAM is ignored. |
| **Pin Multiplexing** | Partial | Address/Data bus pins are claimed via `PinMux` but not high-impedance handled correctly. |

## 5. Summary of Needed Modifications

1. **`AvrCpu::lpm()`**: Add check for `SIGRD` bit in the device-specific `SPMCSR` address.
2. **`MemoryBus::tick()`**: Implement wait-state stalling for CPU fetches.
3. **`MemoryBus::execute_nvm_command()`**: Add more granular support for page-buffer interactions (PBC, EEPBC).
4. **`NvmCtrl`**: Implement `CTRLB` register and its influence on the bus.

> [!NOTE]
> High-priority: LPM Signature/Fuse reading as it is commonly used in RTOS and initial startup code to identify the chip.
