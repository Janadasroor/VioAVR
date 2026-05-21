# VioAVR Bug Audit

> Generated: 2026-05-21
> Scope: Full codebase audit of CPU core, ADC, AC, timers, serial peripherals, memory bus, GPIO, pin mux, CCL, EVSYS, PSC, DAC, EEPROM, PLL, LCD, USB, CAN, and power management.

---

## 🔴 CRITICAL

### C1 — All 16 BRBS/BRBC branch aliases have inverted patterns
**File:** `src/core/avr_cpu.cpp:678-693`

The AVR ISA encodes BRBS (branch if bit set) as `0xF4ss` and BRBC (branch if bit clear) as `0xF0ss`. Every entry in the instruction table has these swapped — `0xF0ss` patterns dispatch to branch-if-set handlers, `0xF4ss` dispatch to branch-if-clear handlers. Every conditional branch (BRCS/BRCC, BREQ/BRNE, BRMI/BRPL, BRVS/BRVC, BRLT/BRGE, BRHS/BRHC, BRTS/BRTC, BRIE/BRID) branches on the **opposite condition**.

### C2 — `consume_interrupt_request` TOCTOU race between arbitration and matching
**File:** `src/core/memory_bus.cpp:378-420`

First loops all peripherals calling `pending_interrupt_request()` to find the best candidate, then loops again to find which peripheral matches. Peripheral state can change between loops (first loop calls `catch_up_all_peripherals`). Matching can fail even with valid pending interrupts, causing lost interrupts.

### C3 — ADC stale scheduler callback not cancelled on conversion abort
**File:** `src/core/adc.cpp:78,124`

When conversion is aborted (ADEN clear or PRADC), `converting_` is set to false but the scheduler event scheduled in `start_conversion()` is never cancelled. Old event fires later, sees `converting_ == true` (set by new conversion), and completes the new conversion prematurely.

### C4 — TCA `consume_interrupt_request` never clears interrupt flags
**File:** `src/core/tca.cpp:343-348`

Returns `true` without touching `intflags_`. Once an interrupt is asserted, the ISR re-fires forever because the flag is never cleared on vector entry.

### C5 — EEPROM `complete_write()` sets shadowed `interrupt_pending_`
**File:** `src/core/eeprom.cpp:238`, `include/vioavr/core/eeprom.hpp:58`

`Eeprom` declares its own `interrupt_pending_` at eeprom.hpp:58 which **shadows** `IoPeripheral::interrupt_pending_`. `complete_write()` at line 238 sets the derived-class member; the base-class member stays `false`. The interrupt controller checks the base-class member — the EEPROM ready interrupt is **never delivered**.

### C6 — EEPROM `interrupt_pending_` shadows base class member (same root as C5)
**File:** `include/vioavr/core/eeprom.hpp:58`

`consume_interrupt_request()` at eeprom.cpp:164 also clears the wrong member.

### C7 — PLL lock delay hardcoded to 100 cycles (should be ~16000)
**File:** `include/vioavr/core/pll.hpp:26`

`static constexpr u64 LOCK_DELAY_CYCLES = 100;` — at 16MHz this is 6.25us. Real AVR PLL lock time is ~1ms (~16000 cycles). PSC high-resolution mode becomes active far too early.

### C8 — PinMux `reset()` does not clear `active_claims`
**File:** `src/core/pin_mux.cpp:144-155`

`reset()` clears `drive_levels`, `output_mask`, `pullup_mask`, `wired_and_mask` but **not `active_claims`**. After system reset, peripheral pin claims survive. GPIO (highest-priority after reset) is never restored as pin owner. PORT writes have no effect.

### C9 — SPI SPIF cleared on SPDR read without prior SPSR read
**File:** `src/core/spi.cpp:74-78`

Real AVR SPI requires: (1) read SPSR while SPIF=1, then (2) read SPDR. This code clears SPIF on **any** SPDR read regardless of prior SPSR read.

### C10 — USB UEINTX writes clear on 0 instead of write-1-to-clear
**File:** `src/core/usb.cpp:215-267`

`ep.interrupt_flags &= (value | 0xA0U)` — a flag is cleared when the CPU writes 0 to its bit position. Real hardware clears flags by writing 1. Completely inverted.

### C11 — TWI8X slave START detection checks wrong signal
**File:** `src/core/twi8x.cpp:99`

`if (scl == PinLevel::high && last_intended_sda_ == PinLevel::high)` — the master drives `last_intended_sda_` low during START, so the slave **never** detects a START from the master.

### C12 — CCL never calls `set_interrupt_pending()` on base class
**File:** `src/core/ccl.cpp`, `src/core/zcd.cpp`

Both CCL and ZCD manage their own interrupt flags but never call `IoPeripheral::set_interrupt_pending()`. The interrupt controller's cached mask never reflects CCL/ZCD interrupts — they are invisible.

### C13 — CCL `luts_` hardcoded to 4 elements but descriptor supports 8
**File:** `include/vioavr/core/ccl.hpp:70`

`std::array<LutState, 4> luts_` but `CclDescriptor.luts` is `LutDescriptor luts[8]`. On devices with `lut_count > 4`, every access to `luts_[i]` for `i > 3` is an out-of-bounds array access.

### C14 — ADC hardcoded MUX5 bit position (bit 5) is wrong for most devices
**File:** `src/core/adc.cpp:262`

Hardcodes ADCSRB bit 5 as MUX5. On ATmega328P, bit 5 is **ACME** (Analog Comparator Mux Enable). On ATmega2560, MUX5 is at bit 3. Should be device-parameterized.

---

## 🟠 HIGH

### H1 — ADC default mux fallback truncates to 4 bits
**File:** `src/core/adc.cpp:272`

`return { static_cast<u8>(mux & 0x0FU), 0, 1.0f, false }` — masks with 0x0F losing MUX4 (bit 4) and MUX5 (bit 5). Channels 8-15 on 5-bit mux devices broken in fallback path.

### H2 — ADC missing `source_id` in interrupt requests
**File:** `src/core/adc.cpp:148,155`

`InterruptRequest` has both `vector_index` and `source_id`. ADC never sets `source_id`. Other peripherals (AnalogComparator, ExtInterrupt) do. Breaks software interrupt prioritization.

### H3 — ADC pin map/ownership limited to 8 channels
**File:** `src/core/adc.cpp:207,279`

Descriptor arrays support 16 channels but both `get_voltage()` and `update_pin_ownership()` hardcode a limit of 8. Channels 8-15 on ATmega2560 lose pin-map voltage resolution and DIDR digital disable.

### H4 — ADC pin claiming gated on DIDR0 instead of ADEN
**File:** `src/core/adc.cpp:284-291`

Pins are claimed/released based on DIDR0 bits, not ADEN. Real AVR claims pins for analog use when ADEN=1, independent of DIDR0.

### H5 — ADC ADCH/ADCL read latching not implemented
**File:** `src/core/adc.cpp:93-103`

Real AVR latches both result registers: reading ADCL freezes ADCH until it is also read. Conversion completing between ADCL and ADCH reads produces corrupted 10-bit value.

### H6 — AC8x DACREF ignores VREF peripheral
**File:** `src/core/ac8x.cpp:91`

DACREF voltage calculated against hardcoded `vdd_` instead of the VREF peripheral's selected reference voltage. VREF.AC0REFSEL can select 0.55V, 1.1V, 1.5V, 2.5V, 4.34V, or AVDD.

### H7 — AC8x `consume_interrupt_request` does not clear CMPIF flag
**File:** `src/core/ac8x.cpp:35-40`

Returns `true` without clearing `status_ & ~0x01U`. Flag stays set permanently; interrupt controller re-fires immediately.

### H8 — AC8x INVERT bit (MUXCTRLA.7) not implemented
**File:** `src/core/ac8x.cpp:98`

`bool new_state = (p_volts > n_volts)` — never checks `muxctrla_ & 0x80U` to invert the output.

### H9 — AC8x hysteresis (HYSMODE) not implemented
**File:** `src/core/ac8x.cpp:98`

CTRLA bits 2:1 (HYSMODE) select 10/25/50mV hysteresis. Comparison uses sharp threshold with no hysteresis window.

### H10 — TCA missing UPDATE at BOTTOM for WGMODE 3/4/5/6
**File:** `src/core/tca.cpp:363-387`

UPDATE condition only set at TOP. For modes 3/4/5/6, UPDATE also occurs at BOTTOM. Double-buffered PER/CMP transfers lost.

### H11 — TCA `get_wo_level` returns false for WO3-WO5 in split mode
**File:** `src/core/tca.cpp:444-465`

`if (!is_enabled() || index >= 3) return false` — H-timer outputs always false. H-timer PWM is dead.

### H12 — TCB CLKSEL=2,3 cause silent stall
**File:** `src/core/tcb.cpp:135-139`

For CLKSEL=2 (CLK_PER/4) and CLKSEL=3 (cascaded from TCA), `perform_tick(false)` returns immediately. Timer never counts.

### H13 — TCB edge detection is level-based, not edge-based
**File:** `src/core/tcb.cpp:220-227`

`bool match_edge = (level != edge_select)` — compares current level against expected polarity. No previous level stored. Repeated captures on sustained events.

### H14 — Timer16 noise canceler counter never reset after successful capture
**File:** `src/core/timer16.cpp:233-244`

After 4th matching sample passes, counter keeps incrementing. Every subsequent sample passes immediately.

### H15 — Timer16 external clock on T1 pin (CS=6,7) not implemented
**File:** `src/core/timer16.cpp:92-113`

CS=6 (falling edge T1) and CS=7 (rising edge T1) fall through with no action. Timer deadlocked.

### H16 — Timer10 match-at-TOP fires before overflow (100% duty glitch)
**File:** `src/core/timer10.cpp:170-198`

When `ocra_ == ocrc_` (100% duty), match fires pin LOW then overflow fires pin HIGH in same tick. Real hardware prioritizes overflow.

### H17 — Timer10 no clock select / prescaler implementation
**File:** `src/core/timer10.cpp:60-67`

All clock select bits silently ignored. Timer always ticks at CPU rate.

### H18 — PSC 16-bit reads bypass temp register latch
**File:** `src/core/psc.cpp:85-105`

Write path correctly uses temp register (high-byte-first). Read path reads each byte from live register. Inconsistent 16-bit reads.

### H19 — DAC voltage calculation uses 1023 instead of 1024
**File:** `src/core/dac.cpp:134`

`voltage_ = static_cast<double>(data_) / 1023.0` — correct for 10-bit DAC is `/ 1024.0`. Systematic -0.1% error.

### H20 — Mapped EEPROM writes bypass all protection sequences
**File:** `src/core/eeprom.cpp:134-143`

AVR8X mapped-data EEPROM writes store directly without checking EEMPE, write-in-progress, or timed protection sequence.

### H21 — UART frame format hardcoded to 10 bits
**File:** `src/core/uart.cpp:89`

`const u64 limit = bit_duration * 10` — ignores UCSRC character size (5-9 bits), parity, and stop bits.

### H22 — UART no RX state machine
**File:** `src/core/uart.cpp:69-113`

Only TX half implemented. No start-bit validation, noise rejection, or bit-timing for RX.

### H23 — SPI no CPOL/CPHA modeling
**File:** `src/core/spi.cpp`

Clock polarity (CPOL) and phase (CPHA) bits completely ignored. All 4 SPI modes treated as mode 0.

### H24 — CAN bit timing ignores sync/prop/phase segments
**File:** `src/core/can.cpp:99-102`

Only extracts BRP from CANBT1. Discards SyncSeg, PropSeg, PhSeg1, PhSeg2. Bit time treated as 1 TQ.

### H25 — CAN transmit wait time hardcoded to 1000 cycles
**File:** `src/core/can.cpp:353`

`tx_wait_cycles_ = 1000` — independent of bit rate, message length, bit stuffing.

### H26 — USB UEDATX write doesn't check TXINI or endpoint direction
**File:** `src/core/usb.cpp:272-279`

CPU can write to UEDATX when TXINI not set (bank not ready) or endpoint is OUT. No overflow check.

### H27 — EUSART EUCSRA write overwrites entire register
**File:** `src/core/eusart.cpp:277`

`eucsra_ = value` — TXC (bit 7) should be write-1-to-clear only. Blind overwrite can set TXC arbitrarily.

### H28 — CCL same TCB read for all 3 LUT inputs
**File:** `src/core/ccl.cpp:199-201`

`tcbs_[index]` returned for all j=0,1,2. Each LUT input should come from different TCBs.

### H29 — EVSYS STROBE doesn't update `channel_levels_`
**File:** `src/core/evsys.cpp:60-71`

Generates pulse to user callbacks but never sets `channel_levels_[i]`. Stale data from `get_channel_level()`.

### H30 — MemoryBus mapped_eeprom stalls CPU on every buffer write
**File:** `include/vioavr/core/memory_bus.hpp:368`

Every byte write to mapped EEPROM calls `request_cpu_stall(54400U)` — even for buffer writes. Can double-stall.

### H31 — PRR writes stored but never propagated to peripherals
**File:** `src/core/cpu_control.cpp:170-175`

`prr_`, `prr0_`, `prr1_` stored in member variables but **never read**. Power reduction is completely non-functional.

### H32 — `reset()` unconditionally sets PORF
**File:** `src/core/cpu_control.cpp:54-56`

`mcusr_ = MCUSR_PORF` — watchdog reset should set WDRF, external reset EXTRF. All non-POR resets wrong.

---

## 🟡 MEDIUM

### M1 — ADC `wants_tick()` returns false but `tick()` has active logic
**File:** `include/vioavr/core/adc.hpp:37`, `src/core/adc.cpp:75-91`

Dead code in production — conversion timing relies on scheduler, not tick.

### M2 — ADC missing DIDR2 support for >8 channels
**File:** `src/core/adc.cpp:279`, `include/vioavr/core/adc.hpp:101`

Devices with >8 analog channels (e.g., ATmega2560) have DIDR2 for channels 8-15. Not implemented.

### M3 — ADC two conflicting `auto_trigger_source_` setting mechanisms
**File:** `src/core/adc.cpp:301-303, 339-341`

`select_auto_trigger_source()` (public API) vs `update_auto_trigger_source_from_register()` (ADCSRB). Can disagree.

### M4 — ADC reserved ADCSRA bits not masked to 0 on read
**File:** `src/core/adc.cpp:104`

`return adcsra_` — reserved/unused bits may contain stale values written by software.

### M5 — ADC division by zero if `vref_` is 0 or negative
**File:** `src/core/adc.cpp:238,245`

No guard against `vref_ <= 0.0`. IEEE 754 division produces +inf; `static_cast<u16>(inf)` is UB.

### M6 — AC8x MUXNEG/MUXPOS mask uses 0x07 instead of 0x03
**File:** `src/core/ac8x.cpp:84-85`

`& 0x07` reads reserved bits 2 and 5. Works for valid values 0-3 but spuriously samples extra bits.

### M7 — AC8x negative input pin map uses hardcoded +4 offset
**File:** `src/core/ac8x.cpp:93`

`n_volts = signal_bank_->voltage(n_mux + 4)` — acknowledged as "dummy logic". Should be device-specific.

### M8 — AC8x positive input pin map uses direct channels
**File:** `src/core/ac8x.cpp:88`

MUXPOS values 0-3 used directly as signal bank indices without device-specific mapping.

### M9 — AC8x OUTEN bit (CTRLA.6) not implemented
**File:** `src/core/ac8x.cpp`

Output on physical pin never driven.

### M10 — AC8x LPMODE and RUNSTDBY bits not implemented
**File:** `src/core/ac8x.cpp`

CTRLA bits 3 (LPMODE) and 7 (RUNSTDBY) never checked.

### M11 — AC8x event system user callback is empty
**File:** `src/core/ac8x.cpp:20-22`

Registered callback is empty lambda. Event-triggered comparison not implemented.

### M12 — AC8x no propagation delay modeled
**File:** `src/core/ac8x.cpp:76,97-101`

Comparator output transitions instantly on every tick. No settling time.

### M13 — AC8x dacctrla_ initializer {0} vs reset value 0xFF
**File:** `include/vioavr/core/ac8x.hpp:55`, `src/core/ac8x.cpp:45`

Member initializer is `{0}` but `reset()` sets it to 0xFF. Value incorrect if read before reset().

### M14 — TCA FRQ mode erroneously sets OVF flag
**File:** `src/core/tca.cpp:354,390`

Setting `update_cond = true` in FRQ mode at CMP0 causes OVF flag set. FRQ mode should NOT set OVF.

### M15 — TCB measurement mode overflow doesn't set CAPT flag
**File:** `src/core/tcb.cpp:189-194`

Counter wrap from 0xFFFF to 0x0000 in measurement modes should set `intflags_` bit 0.

### M16 — Timer16 initial noise_canceler_counter_ = 10 defeats 4-sample requirement
**File:** `src/core/timer16.cpp:228`

Initial value 10 means first edge passes noise canceler immediately (no 4-sample delay enforced).

### M17 — Timer16 new OCR at BOTTOM can cause same-tick spurious match
**File:** `src/core/timer16.cpp:182-186`

After BOTTOM, tcnt_=1. If newly loaded ocra_buffer_ is 1, match fires immediately on same tick.

### M18 — PSC counter value never readable via I/O register interface
**File:** `src/core/psc.cpp:74-108`

No read case for counter value. Firmware reading PSC counter for synchronization cannot work.

### M19 — PSC `handle_fault()` is dead code
**File:** `src/core/psc.hpp:92`, `src/core/psc.cpp:378-404`

Declared, defined, never called. Contains logic inconsistent with inline fault handling.

### M20 — PSC PRFM mode 5 handled inconsistently across code paths
**File:** `src/core/psc.cpp:244-257, 342-357, 390-392`

tick() treats mode 5 as "no action", notify_fault() treats it as retrigger, handle_fault() (dead) groups with retrigger.

### M21 — PSC fault blanking counter off-by-one
**File:** `src/core/psc.cpp:198-201, 221-224`

Decrements blanking counter first, then suppresses fault. May be one cycle short vs hardware.

### M22 — PSC notify_fault() for channel B doesn't update last_fault_level_
**File:** `src/core/psc.cpp:369-375`

Channel A updates it, channel B doesn't. Asymmetric.

### M23 — DAC notify_auto_trigger doesn't update physical pin output
**File:** `src/core/dac.cpp:97-121`

Updates `data_` and voltage but pin output only updated in tick(). Stale voltage between trigger and tick.

### M24 — DAC tick() dereferences bus_ without null check
**File:** `src/core/dac.cpp:42-46`

No null check on `bus_` before calling `bus_->pin_mux()`. Crashes if DAC instantiated without bus.

### M25 — EEPROM master_write_enable_timeout_ can have multiple outstanding callbacks
**File:** `src/core/eeprom.cpp:182-188`

Writing EEMPE=1 twice before first 4-cycle timeout expires schedules second callback without canceling first.

### M26 — EEPROM `eecr_` never stores EEPE bit; faked only on read
**File:** `src/core/eeprom.cpp:93, 191-197`

`eecr_` never has EEPE=1. Read path OR's it in. Internal code inspecting `eecr_` directly never sees EEPE.

### M27 — EEPROM descriptor `ucsrc_reset` never used
**File:** `src/core/uart.cpp:56`

Reset value hardcoded to 0x06 (8N1) instead of using `desc_.ucsrc_reset`.

### M28 — UART UCSRA write corrupts read-only flags
**File:** `src/core/uart.cpp:138-139`

Value bits 2-7 not masked — can set FE, DOR, UPE (read-only) and other reserved bits.

### M29 — UART interrupt priority starvation for TXC
**File:** `src/core/uart.cpp:152-173`

Returns first pending (RXC > UDRE > TXC). RXC persistently set makes TXC interrupt impossible to service.

### M30 — UART consume_interrupt_request clears TXC only for exact vector match
**File:** `src/core/uart.cpp:182-186`

TXC cleared only when TX vector consumed. RXC or UDRE consumption leaves TXC uncleared — TXC interrupt lost forever.

### M31 — UART RXEN/TXEN gating not checked
**File:** `src/core/uart.cpp`

Never checks `ucsrb_ & desc_.rxen_mask` before asserting RXC, or `txen_mask` before starting TX.

### M32 — SPI WCOL not cleared after transfer completes
**File:** `src/core/spi.cpp:191`

WCOL set by write during transfer but never cleared by hardware after transfer completes.

### M33 — SPI slave mode ignores external SCK timing
**File:** `src/core/spi.cpp:170-178`

Slave transfer always completes after 1 cycle regardless of master SCK rate.

### M34 — TWI (classic) complete_step processes whole transaction atomically
**File:** `src/core/twi.cpp:146-187`

No per-bit state machine. Entire byte + ACK handled as single atomic step.

### M35 — TWI STA bit cleared immediately instead of after address+ACK
**File:** `src/core/twi.cpp:156-158`

STA cleared when START issues, not after SLA+W/R+ACK completes.

### M36 — TWI STOP doesn't set TWINT
**File:** `src/core/twi.cpp:159-164`

After STOP, returns without setting interrupt_pending_. CPU never knows STOP completed.

### M37 — TWI ACK is placeholder, not real address matching
**File:** `src/core/twi.cpp:194`

ACK determined by rx buffer data availability, not by matching TWAR address.

### M38 — TWI slave mode is a stub
**File:** `src/core/twi.cpp:218-222`

handle_slave_step() is empty. Slave mode non-functional.

### M39 — TWI no clock stretching
**File:** `src/core/twi.cpp`

Clock stretching (SCL held low by slave) not modeled.

### M40 — TWI8X slave receives extra bit on rising edge when bits_left == 0
**File:** `src/core/twi8x.cpp:121-144`

9th rising edge (ACK bit) samples into shift register before bits_left check. Corrupts received byte.

### M41 — TWI8X slave RX data phase initializes bits_left = 7 instead of 8
**File:** `src/core/twi8x.cpp:184`

Should be 8. Accidentally correct due to extra-sample bug (M40).

### M42 — TWI8X master auto-read sets bits_left = 8 instead of 9
**File:** `src/core/twi8x.cpp:253`

First read byte after address sets bits_left=8 (should be 9 for data+ACK).

### M43 — TWI8X consume_interrupt_request returns true without clearing
**File:** `src/core/twi8x.cpp:403-405`

Returns `true` unconditionally without clearing any flags. Interrupt re-fires immediately.

### M44 — TWI8X per-cycle loop O(n) performance issue
**File:** `src/core/twi8x.cpp:79`

Loop iterates per cycle for large elapsed_cycles. 1ms = 16000 iterations at 16MHz.

### M45 — USB SOF timing uses CPU clock instead of USB clock (48MHz PLL)
**File:** `src/core/usb.cpp:43-46, 69-86`

`frame_cycles_ = cpu_frequency_hz / 1000` — should use USB clock domain (48MHz PLL), not CPU clock.

### M46 — USB UERST bits not auto-cleared after endpoint reset
**File:** `src/core/usb.cpp:165-179`

Comment says "bit is not automatically cleared" — but real hardware auto-clears. Endpoint stays under perpetual reset.

### M47 — USB data toggle toggles unconditionally on every OUT packet
**File:** `src/core/usb.cpp:382`

Data toggle should only flip when DATA1 follows DATA0 or vice versa. Retransmitted packets (same PID) should not toggle.

### M48 — CAN error counters only increment, never decrement
**File:** `src/core/can.cpp:428-432`

TEC/REC incremented on error but never decremented on successful tx/rx. Device enters Bus-Off permanently.

### M49 — CAN DLC read from MOb configuration, not received message
**File:** `src/core/can.cpp:402`

Number of bytes to copy determined by MOb CDM register, not received message DLC field.

### M50 — CAN MOb data overwritten without checking if CPU is still reading
**File:** `src/core/can.cpp:400-421`

New received message overwrites data buffer unconditionally. No overrun protection.

### M51 — CAN SWRES doesn't re-evaluate interrupts
**File:** `src/core/can.cpp:208-214`

reset() called without evaluate_interrupts(). Spurious interrupt after software reset.

### M52 — EUSART RX overrun/overflow not detected or limited
**File:** `src/core/eusart.cpp:236-237`

rx_queue_ grows unbounded. No overrun flag set on overflow.

### M53 — EUSART inject/consume use u8 but queues store u32
**File:** `src/core/eusart.cpp:296-305`

API uses u8 but queues are deque<u32>. Byte truncation for extended frames (9-17 bit).

### M54 — CCL FEEDBACK reads sequential state instead of combinatorial output
**File:** `src/core/ccl.cpp:176-178, 273`

Sequential unit overwrites outputs_[2n]. FEEDBACK path then reads sequential state, not true combinatorial output.

### M55 — CCL RS latch: S=1 and R=1 clears output instead of holding state
**File:** `src/core/ccl.cpp:261-265`

When both Set and Reset active, per hardware spec output should remain in previous state, not forced false.

### M56 — CCL output pin routing hardcoded for ATmega4809
**File:** `src/core/ccl.cpp:294-300, 320-326`

Port/pin mapping hardcoded. Other AVR8X devices have different CCL output assignments.

### M57 — CCL update_logic() 4-iteration limit with no oscillation detection
**File:** `src/core/ccl.cpp:216-224`

LUT as inverter with FEEDBACK would oscillate. Stops after 4 iterations in arbitrary state.

### M58 — CCL event system callbacks captured `this` — use-after-free risk
**File:** `src/core/ccl.cpp:104-106, 108-110`

Lambda captures `this` with no lifetime management. If CCL destroyed while callback held by evsys, UB.

### M59 — CCL set_memory_bus() string concatenation for TCB names
**File:** `src/core/ccl.cpp:87-91`

Generates temporary std::string objects at runtime. Also, non-standard TCB naming fails lookup.

### M60 — GPIO reset() doesn't clear analog state arrays
**File:** `src/core/gpio_port.cpp:83-102`

pin_levels_, pin_voltages_, has_voltage_input_, has_analog_binding_, pin_bindings_ not cleared on reset.

### M61 — GPIO consume_pin_change never writes cycle_stamp
**File:** `src/core/gpio_port.cpp:189-206`

PinStateChange.cycle_stamp left as default 0. Consumers relying on timing get bogus value.

### M62 — GPIO floating-point == for default thresholds is fragile
**File:** `src/core/gpio_port.cpp:349, 378, 391`

0.3 is not exact in binary IEEE 754. Threshold comparison may fail if constructed via different path.

### M63 — GPIO set_input_voltage() applies voltage with stale threshold when no analog binding
**File:** `src/core/gpio_port.cpp:377-382`

Uses default {0.3, 0.6} threshold when no binding active. Device-specific VIL/VIH factors never applied.

### M64 — GPIO consume_pin_change only returns one bit per call
**File:** `src/core/gpio_port.cpp:189-206`

Multiple simultaneous pin changes require caller to loop. Single call per cycle loses events.

### M65 — GPIO wants_tick() returns true for voltage-only pins but tick() does nothing
**File:** `src/core/gpio_port.cpp:104-108, 385-399`

tick() only samples binding entries, not voltage pins. Wasted ticks.

### M66 — GPIO constructor merges ranges with uninitialized buffer
**File:** `src/core/gpio_port.cpp:28-71`

ranges_ default-initialized (all zero). First non-zero address at 0x0001 would merge into {0x0000, 0x0001}.

### M67 — PinMux reset() doesn't clear pullup_suppressed_
**File:** `src/core/pin_mux.cpp:144-155`

MCUCR.PUD state survives reset. Pull-ups remain suppressed after system reset.

### M68 — PinMux wired-and mode applied to all claimants, not just requester
**File:** `src/core/pin_mux.cpp:176-188`

If any claimant requests wired-and, ALL active claimants enter wired-and mode. Push-pull outputs corrupted.

### M69 — PinMux update_pin_by_address() drops wired_and parameter
**File:** `src/core/pin_mux.cpp:92-98`

Full update_pin() accepts 7th bool wired_and. update_pin_by_address() doesn't — wired-and never settable via address API.

### M70 — PinMux constructor forces minimum 16 ports even for tiny devices
**File:** `src/core/pin_mux.cpp:10`

ATtiny with 6 ports gets 16 port structures. Ports 6-15 are valid targets despite not existing in hardware.

### M71 — PinMux claim_pin() always returns true — conflict detection impossible
**File:** `src/core/pin_mux.cpp:25-39`

Only false for OOB port/bit. Higher-priority owner override returns true — no way for caller to detect.

### M72 — CPUINT highest_priority_vector() in round-robin wraps to RESET vector
**File:** `include/vioavr/core/cpu_int.hpp:28-33`

`return (last_ack_vector_ + 1)` — if last_ack_vector_ is 0xFF, wraps to 0 (RESET). RESET should never be selected.

### M73 — CPUINT is_lvl1_vector() checks `==` instead of `>=`
**File:** `include/vioavr/core/cpu_int.hpp:25`

`lvl1vec_ == vector_index` — datasheet says vectors >= LVL1VEC are Level 1. Only exact match treated as LVL1.

### M74 — CPUINT on_reti() corrupts status_ if called with no active interrupt
**File:** `include/vioavr/core/cpu_int.hpp:39-45`

If status_ == 0, else branch clears bit 0. Corrupts status register.

### M75 — CpuControl CLKPR double-write extends write-enable window
**File:** `src/core/cpu_control.cpp:179-180`

Writing 0x80 while CLKPCE already set resets 4-cycle window. Real hardware does not extend window.

### M76 — CpuControl effective_frequency() uses float — precision loss
**File:** `src/core/cpu_control.cpp:89-91`

float has ~7 decimal digits. 16,000,000 converted to float loses precision for OSCCAL scaling.

### M77 — MemoryBus mapped_flash read falls through to wrong region
**File:** `include/vioavr/core/memory_bus.hpp:277-285`

When word_addr >= flash_.size(), falls through to check other regions then SRAM. Should return 0xFF.

### M78 — MemoryBus consume_pin_change loses unclaimed pin changes
**File:** `src/core/memory_bus.cpp:254-265`

Clears has_pending_pin_changes_ even when no peripheral claims the change. Events lost.

### M79 — MemoryBus on_power_state_change lost for unmapped PRR addresses
**File:** `include/vioavr/core/memory_bus.hpp:387-408`

sync() call runs but on_power_state_change() only runs if write hits dispatch table.

### M80 — MemoryBus read_program_word re-stalls on every NRWW read during SPM
**File:** `include/vioavr/core/memory_bus.hpp:72-74`

request_cpu_stall() uses std::max — re-extends stall to full remaining SPM time on every NRWW read.

### M81 — ExtInterrupt hardcodes INT0 on pin bit_index 2 (PD2)
**File:** `src/core/ext_interrupt.cpp:117`

Assumes INT0 always on PD2. Wrong for devices with different pin mapping.

### M82 — ExtInterrupt tick() ignores elapsed_cycles, may trigger spurious edges
**File:** `src/core/ext_interrupt.cpp:67-74`

refresh_bound_input() reads current level rather than simulating transitions over elapsed window.

### M83 — LCD controller segment limit array access bounds
**File:** `src/core/lcd_controller.cpp:134-137`

lcdpm = lcdcrb_ & 0x0FU yields 0-15 but kSegmentLimits[15] accessed. Bounds assertion missing.

### M84 — LCD frame timing uses float with possible precision loss
**File:** `src/core/lcd_controller.cpp:189`

`cpu_frequency_ / 32768.0` as double then multiplied and cast to u64. Rounding error accumulates.

### M85 — PinChangeInterrupt interrupt_pending_ shadows base class
**File:** `include/vioavr/core/pin_change_interrupt.hpp:52`

Same pattern as EEPROM (C5). Derived class declares own interrupt_pending_ shadowing IoPeripheral member.

### M86 — PinChangeInterrupt unconditional stderr debug output
**File:** `src/core/pin_change_interrupt.cpp:113, 116`

fprintf(stderr, ...) on every PCI register write. Debug artifact, not guarded.

### M87 — CpuControl effective_frequency() uses float for OSCCAL scaling
**File:** `src/core/cpu_control.cpp:89-91`

float precision loss for high frequencies. Should use double or fixed-point.

### M88 — ADC mux_table missing internal references for ATmega328P
**File:** `include/vioavr/core/devices/atmega328p.hpp:71`

Temperature sensor (MUX=8), bandgap 1.1V (MUX=14), GND (MUX=15), differential pairs (MUX=20-27) not modeled.

---

## 📋 SUMMARY

| Severity | Count | Key Areas |
|----------|-------|-----------|
| 🔴 CRITICAL | 14 | CPU branches, interrupt delivery, EEPROM, PLL, PinMux, SPI, USB, TWI8X, CCL, ADC |
| 🟠 HIGH | 32 | ADC, AC8x, TCA, TCB, Timer16/10, PSC, DAC, UART, SPI, CAN, USB, EEPROM, CCL, EVSYS |
| 🟡 MEDIUM | 42 | GPIO, PinMux, CCL, EVSYS, CPUINT, CpuControl, MemoryBus, ExtInterrupt, LCD, Watchdog |
| **Total** | **88** | |

### Quick Fix Guide

**Fix the critical bugs first:**
1. `avr_cpu.cpp:678-693` — Swap `0xF000`/`0xF400` patterns for all 8 branch alias pairs
2. `tca.cpp:343-348` — Clear `intflags_` in `consume_interrupt_request`
3. `eeprom.hpp:58` — Remove shadowed `interrupt_pending_`, use base class
4. `eeprom.cpp:238` — Call `update_interrupt_pending()` instead of setting member directly
5. `pin_mux.cpp:144-155` — Clear `active_claims` in `reset()`
6. `spi.cpp:74-78` — Add `spsr_read_since_spif` flag for proper SPIF clearing sequence
7. `ccl.cpp` / `zcd.cpp` — Call `set_interrupt_pending()` when interrupt state changes
8. `pll.hpp:26` — Make lock delay frequency-dependent (e.g., `cpu_frequency / 1000`)
