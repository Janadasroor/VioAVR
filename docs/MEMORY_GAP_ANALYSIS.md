# VioAVR Memory & EEPROM Accuracy

This document lists what the memory system does today, what was closed
recently, and what is still open. The previous version of this file
predated a large amount of work and listed features as missing that are
implemented — always check the code before trusting a gap list here.

Status is verified against the code, the Microchip DFP pack
(`avr-pack/`, checked by the `vioavr_pack_header_check` test) and the
datasheets in `docs/datasheets/`.

## 1. NVMCTRL (AVR8X / megaAVR-0)

| Feature | Status | Notes |
| --- | --- | --- |
| **Flash wait states** | Done | `NVMCTRL.CTRLB` bits 1:0 feed `MemoryBus::set_flash_wait_states()`; a flash access then requests that many stall cycles. |
| **EEPROM page writing** | Done | The page buffer tracks dirty bytes, clears on command and writes back only what was loaded (Ex `EEPER`, Dx `EEBER`/`EEMBERn`). |
| **User row access** | Done | `mapped_user_signatures` is backed by a real user row, so the signature bytes read back as the device reports them. |
| **Boot row** | Open | The DA/DB boot row is not modelled: no `NVMCTRL` boot-row commands, no write protection for it. Only matters for firmware that writes the boot row. |
| **Fuse/lockbit read** | Done | `LPM` with `SIGRD` returns the signature bytes, the `BLBSET` sequence returns the fuse bytes, and the lock bits are enforced for `LPM` across the boot/application boundary. |
| **Boot row protection** | Open | `BOOTRST` and the boot section lock bits are not enforced on *writes*; reads are enforced. |

## 2. Legacy SPM / EEPROM (ATmega/ATtiny)

| Feature | Status | Notes |
| --- | --- | --- |
| **SPM section stalling** | Done | A write to the RWW section stalls the CPU when it fetches from that section (commits `26ec773`, `e439fed`); the scope is the targeted section only. |
| **EEPROM read stall** | Done | 4 cycles, as the datasheet asks. |
| **EEPROM write stall** | Done | 2 cycles at the start of the write; the long write itself no longer stalls the CPU (the value is committed asynchronously, the way the mapped write behaves). |
| **EEPROM protection (EESAVE)** | Open | The `EESAVE` fuse is not honoured: a chip erase clears the EEPROM even when the fuse says it should be kept. |
| **LPM signature / fuse read** | Done | See above. |

## 3. External memory (XMEM)

| Feature | Status | Notes |
| --- | --- | --- |
| **Wait states (XMCRA/B)** | Done | `MemoryBus::get_wait_states()` asks the XMEM peripheral for the configured wait states. |
| **Pin multiplexing** | Partial | The address/data pins are claimed through the `PinMux`, but the high-impedance behaviour of the bus pins is not modelled in detail. |

## 4. What is verified, and how

- `vioavr_pack_header_check` compares every generated device descriptor
  against the Microchip DFP headers in `avr-pack/` (14 devices: the
  classic ATmega/ATtiny parts and the AT90PWM3 family). It runs as part of
  the test suite, so a register address cannot drift away from the pack
  unnoticed.
- The `sim_accuracy_regression_test` group covers the memory behaviour
  that the datasheets pin down: NVMCTRL Dx command timing, Ex/Dx erase
  granularity, the asynchronous mapped EEPROM write, the SPM RWW stall and
  the CPU instruction set.
- The behaviour above is compared against the datasheets in
  `docs/datasheets/`, not against silicon. A cross-check against a real
  part (or against  for the CPU core only) is still missing — see
  `docs/HARDWARE_ACCURACY.md`.
