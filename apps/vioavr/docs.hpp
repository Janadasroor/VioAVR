#pragma once

#include "ansi.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

// ---------------------------------------------------------------------------
// Built-in documentation system (inspired by FerroMNA's `docs` subcommand)
// ---------------------------------------------------------------------------

struct DocEntry {
    std::string_view name;       // topic key
    std::string_view short_desc; // one-line summary
    std::string_view long_text;  // full body with syntax, examples, etc.
};

static constexpr DocEntry DOCS[] = {
    {
        "overview",
        "VioAVR — AVR Microcontroller Simulator overview",
        "VioAVR is a fast, cycle-accurate AVR microcontroller simulator that supports\n"
        "classic ATmega, ATtiny, AT90, AVR-Dx/Ex/Lx/Sx, megaAVR-0, tinyAVR-0/1/2,\n"
        "and XMEGA device families.\n"
        "\n"
        "Key features:\n"
        "  • Cycle-accurate AVR CPU with JIT compilation (10-100x speedup)\n"
        "  • All major peripherals: GPIO, UART, SPI, TWI, ADC, timers, interrupts, WDT\n"
        "  • ngspice co-simulation via shared memory bridge\n"
        "  • GDB stub for source-level debugging\n"
        "  • Instruction tracing with register/SREG/memory output\n"
        "  • 290+ device descriptors generated from Microchip ATDF packs\n"
        "  • EVSYS event system for peripheral chaining\n"
        "\n"
        "See 'vioavr docs <topic>' for specific topics:\n"
        "  mcus, run, trace, benchmark, bridge, gdb, hex, eeprom, evsys, jit,\n"
        "  co-sim, programming-guide, a-device, co-sim-tests, device-descriptors,\n"
        "  xspice-models\n"
    },
    {
        "mcus",
        "Supported microcontroller families and how to select them",
        "VioAVR supports over 290 MCU models across these families:\n"
        "\n"
        "  Classic AVR       ATmega, ATtiny, AT90 — 8-bit with separate I/O space\n"
        "  megaAVR-0         1-series, 2-series — LUT/CCL, EVSYS, TCA/TCB\n"
        "  tinyAVR-0/1/2     Small-pin-count with unified memory map\n"
        "  AVR-Dx            DA, DB, DD — advanced 8-bit with multi-voltage I/O\n"
        "  AVR-Ex            EA, EB — extreme low power\n"
        "  AVR-Lx            LA — low leakage\n"
        "  AVR-Sx            SA, SD, SG — safety-oriented (SIL)\n"
        "  XMEGA             A, B, C, D — 16-bit timer array, DMA, AES\n"
        "\n"
        "Usage:\n"
        "  vioavr run firmware.hex --mcu ATmega328P\n"
        "  vioavr info ATmega4809\n"
        "  vioavr list-devices --filter atmega\n"
        "\n"
        "The MCU name is case-insensitive.\n"
    },
    {
        "run",
        "Run firmware on a simulated MCU",
        "The 'run' subcommand loads an Intel HEX file and executes it on the\n"
        "selected microcontroller.\n"
        "\n"
        "Usage:\n"
        "  vioavr run [options] <hex_file>\n"
        "\n"
        "Options:\n"
        "  --mcu <name>        MCU model (default: ATmega328P)\n"
        "  --max-cycles <n>    Stop after N cycles (suffix: k, M)\n"
        "  --summary           Show detailed execution summary\n"
        "  --show-state        Show GPIO port states after run\n"
        "  --eeprom-file <f>   Load/save EEPROM data from file\n"
        "  --verbose           Show debug-level log output\n"
        "  --quiet             Suppress all non-error output\n"
        "  --color <mode>      Color: auto, always, never (default: auto)\n"
        "\n"
        "Example:\n"
        "  vioavr run blink.hex --mcu ATmega328P --summary\n"
        "  vioavr run firmware.hex --mcu ATmega4809 --max-cycles 1M --show-state\n"
    },
    {
        "trace",
        "Cycle-by-cycle instruction tracing",
        "The 'trace' subcommand executes firmware one instruction at a time\n"
        "and prints each instruction with its opcode, address, register writes,\n"
        "SREG changes, and optional memory writes.\n"
        "\n"
        "Usage:\n"
        "  vioavr trace [options] <hex_file>\n"
        "\n"
        "Options:\n"
        "  --mcu <name>        MCU model (default: ATmega328P)\n"
        "  --max-cycles <n>    Stop after N cycles (default: 1000)\n"
        "  --registers         Show register writes (on by default)\n"
        "  --no-registers      Hide register writes\n"
        "  --mem-writes        Show memory write operations\n"
        "  --color <mode>      Color: auto, always, never\n"
        "\n"
        "The trace output shows:\n"
        "  • Instruction address, mnemonic, and raw opcode\n"
        "  • Register writes (R0-R31, SREG with flag states)\n"
        "  • Memory writes (when --mem-writes is set)\n"
        "  • Interrupt events\n"
        "\n"
        "SREG flag legend:  C=Carry  Z=Zero  N=Negative  V=Overflow\n"
        "                   S=Sign  H=Half-Carry  T=Transfer  I=Interrupt\n"
    },
    {
        "benchmark",
        "JIT/Interpreter performance benchmarking",
        "The 'benchmark' subcommand measures simulation throughput by running\n"
        "a tight NOP loop on the selected MCU.\n"
        "\n"
        "Usage:\n"
        "  vioavr benchmark [options]\n"
        "\n"
        "Options:\n"
        "  --mcu <name>        MCU model (default: ATmega328P)\n"
        "  --cycles <n>        Cycle count (default: 100M, suffix: k, M)\n"
        "  --color <mode>      Color: auto, always, never\n"
        "\n"
        "Output:\n"
        "  • Live progress bar (when connected to a TTY)\n"
        "  • Wall-clock time\n"
        "  • Throughput in MHz (MIPS)\n"
        "\n"
        "Typical results (Release build):\n"
        "  ATmega328P JIT:      ~140 MHz\n"
        "  ATmega328P Interp:   ~70 MHz\n"
        "  ATmega4809 JIT:      ~60 MHz (larger device state)\n"
    },
    {
        "bridge",
        "ngspice co-simulation bridge daemon",
        "The 'bridge' subcommand starts a shared memory bridge daemon that\n"
        "connects the AVR simulator to ngspice for mixed-signal co-simulation.\n"
        "\n"
        "Usage:\n"
        "  vioavr bridge [options]\n"
        "\n"
        "Options:\n"
        "  --mcu <name>        MCU model (default: atmega328p)\n"
        "  --instance <id>     SHM instance name (default: MCU name)\n"
        "  --gdb <port>        Start GDB stub on port for debugging\n"
        "  --freq <hz>         CPU clock frequency (default: 16000000)\n"
        "  --color <mode>      Color: auto, always, never\n"
        "\n"
        "The bridge uses POSIX shared memory (/dev/shm) to exchange digital\n"
        "pin states and analog voltages between VioAVR and ngspice.\n"
        "The ngspice netlist must include a d_vioavr A-device model.\n"
        "\n"
        "Example:\n"
        "  vioavr bridge --mcu atmega4809 --instance my_avr &\n"
        "  ngspice -b my_circuit.cir\n"
    },
    {
        "gdb",
        "GDB remote debugging stub",
        "The 'gdb' subcommand starts a GDB Remote Serial Protocol server,\n"
        "allowing you to debug firmware with gdb-multiarch or avr-gdb.\n"
        "\n"
        "Usage:\n"
        "  vioavr gdb [options] <hex_file>\n"
        "\n"
        "Options:\n"
        "  --mcu <name>        MCU model (default: ATmega328P)\n"
        "  --port <n>          TCP port (default: 1234)\n"
        "  --color <mode>      Color: auto, always, never\n"
        "\n"
        "Connecting:\n"
        "  gdb-multiarch -ex 'target remote :1234' firmware.elf\n"
        "\n"
        "Supported GDB operations:\n"
        "  • Breakpoints (software, up to 32)\n"
        "  • Single-step, continue\n"
        "  • Read/write registers and memory\n"
        "  • Flash programming\n"
        "  • Interrupt handling\n"
    },
    {
        "hex",
        "Intel HEX file format support",
        "VioAVR loads firmware from standard Intel HEX (.hex) files.\n"
        "\n"
        "Supported record types:\n"
        "  00  Data record\n"
        "  01  End-of-file record\n"
        "  02  Extended segment address\n"
        "  04  Extended linear address\n"
        "  05  Start linear address (entry point)\n"
        "\n"
        "The loader automatically maps addresses to the correct memory\n"
        "region (flash vs. EEPROM) based on the device descriptor.\n"
        "Non-flash records (e.g. EEPROM data at different address ranges)\n"
        "are dispatched to the appropriate memory controller.\n"
    },
    {
        "eeprom",
        "EEPROM persistence and file I/O",
        "VioAVR supports loading and saving EEPROM content from binary files.\n"
        "\n"
        "Usage:\n"
        "  vioavr run firmware.hex --eeprom-file data.eep\n"
        "\n"
        "The EEPROM file is loaded before simulation starts and saved\n"
        "after simulation completes. This allows persistent state across\n"
        "multiple simulation runs.\n"
        "\n"
        "The file format is a raw binary dump of the EEPROM address space,\n"
        "starting at address 0. The file size should match the device's\n"
        "EEPROM size (e.g., 1024 bytes for ATmega328P, 256 bytes for ATmega4809).\n"
    },
    {
        "evsys",
        "Event System (EVSYS) for peripheral chaining",
        "The Event System allows peripherals to communicate directly without\n"
        "CPU intervention, enabling low-latency chaining of:\n"
        "\n"
        "  • TCA → TCB timer cascade\n"
        "  • ADC → TCA/TCB triggered measurements\n"
        "  • CCL → event-controlled logic\n"
        "  • AC → event-triggered ADC\n"
        "  • RTC → periodic event generation\n"
        "\n"
        "Generators produce events (e.g., TCA overflow, ADC conversion done).\n"
        "Users consume events to trigger actions in other peripherals.\n"
        "\n"
        "Implementation status:\n"
        "  ✓ TCA overflow → TCB clock cascade\n"
        "  ✓ CCL LUT output events\n"
        "  ✓ RTC periodic events\n"
        "  ✓ ADC conversion complete events\n"
        "  ✓ Pin-change events\n"
    },
    {
        "jit",
        "Just-In-Time (JIT) compilation engine",
        "VioAVR includes a lightweight x86-64 JIT compiler that translates\n"
        "AVR instruction blocks directly to x86 machine code.\n"
        "\n"
        "Key characteristics:\n"
        "  • Translates basic blocks on first execution\n"
        "  • Cached compiled blocks for repeated code paths\n"
        "  • Fallback to interpreter for self-modifying code\n"
        "  • 10-100x speedup vs. pure interpretation\n"
        "  • Disabled automatically when tracing is active\n"
        "\n"
        "The JIT is enabled by default in Release builds.\n"
        "Use --verbose to see JIT statistics at runtime.\n"
    },
    {
        "co-sim",
        "Mixed-signal co-simulation with ngspice",
        "VioAVR co-simulates with ngspice for mixed-signal designs.\n"
        "The AVR runs as a digital component while analog circuits\n"
        "(filters, amplifiers, sensors) run in SPICE.\n"
        "\n"
        "Architecture:\n"
        "  • d_vioavr A-device in ngspice netlist\n"
        "  • POSIX shared memory bridge for cycle-level sync\n"
        "  • Configurable step size (default: 1 us)\n"
        "  • Digital I/O: up to 32 pins with VIH/VIL thresholds\n"
        "  • Analog I/O: 128 input channels, 8 output channels\n"
        "  • Pull-up resistor network for open-drain buses\n"
        "\n"
        "Workflow:\n"
        "  1. vioavr bridge --mcu atmega4809 --instance my_avr &\n"
        "  2. ngspice -b mixed_signal.cir\n"
        "\n"
        "See also: 'vioavr docs bridge' for bridge options.\n"
        "          tests/core/viospice/ for test examples.\n"
    },
    {
        "programming-guide",
        "AI reference: writing firmware + netlists for modern AVR chips",
        "─ Memory Architecture ──────────────────────────────────────────────\n"
        "\n"
        "Modern AVRs (AVR-Dx/Ex/Lx/Sx, megaAVR-0, tinyAVR-0/1/2):\n"
        "  Unified memory map. Only the first 32 I/O registers (0x00-0x1F)\n"
        "  support IN/OUT. All other I/O registers accessed via STS/LDS\n"
        "  (or ST/LD with Y/Z pointer).\n"
        "\n"
        "Classic AVRs (ATmega, ATtiny, AT90):\n"
        "  Separate I/O space (0x00-0x3F) with IN/OUT instructions.\n"
        "  Extended I/O (0x60+) via STS/LDS.\n"
        "\n"
        "  SRAM starts after I/O register space. Address map:\n"
        "    0x0000-0x001F   Register file (R0-R31)\n"
        "    0x0020-0x005F   I/O registers (IN/OUT accessible)\n"
        "    0x0060+         SRAM / extended I/O\n"
        "\n"
        "─ Clock System ────────────────────────────────────────────────────\n"
        "\n"
        "  Modern AVRs boot from internal RC oscillator. Default frequency\n"
        "  varies: AVR-Dx = 20 MHz, megaAVR-0 = 16 MHz, tinyAVR-0/1/2 = 16 MHz.\n"
        "\n"
        "  AVR-Dx/Ex: Configure via CLKCTRL.MAINCLKSEL and CLKCTRL.OSCHFCTRLA.\n"
        "  megaAVR-0: CLKCTRL register. Prescaler via CLKCTRL.PDIV + CLKCTRL.PSCTRL.\n"
        "  XMEGA: Internal 32 MHz oscillator. Can enable PLL for higher freq.\n"
        "\n"
        "  VioAVR note: CLKCTRL writes are skipped when timer count > 0.\n"
        "  This is by design for co-simulation test determinism.\n"
        "\n"
        "─ Event System (EVSYS) ────────────────────────────────────────────\n"
        "\n"
        "  EVSYS lets peripherals communicate without CPU intervention.\n"
        "  Generators  →  event channels  →  users.\n"
        "\n"
        "  Common event chains:\n"
        "    TCA0 overflow  →  TCB0 clock (TCB counts on event)\n"
        "    ADC0 done      →  TCA/TCB trigger\n"
        "    CCL LUT output →  other peripherals\n"
        "    AC0 out        →  ADC trigger\n"
        "    RTC periodic   →  event every N RTC ticks\n"
        "\n"
        "  Each event channel has a generator mux and a user mux.\n"
        "  To chain TCA0→TCB0:\n"
        "    1. Set EVSYS.CHANNELn = EVSYS_CHANNELn_TCA0_OVF_LSB_gc\n"
        "    2. Set EVSYS.USERTCB0COUNT = EVSYS_USER_CHANNELn_gc\n"
        "    3. Set TCB0.CTRLA = TCB_ENABLE_bm | (0x07 << 1)\n"
        "       (CLKSEL=7 means 'clock from event system')\n"
        "\n"
        "  CRITICAL GOTCHA: TCB CLKSEL is a 3-bit field at bits [3:1].\n"
        "  Write (7 << 1) = 0x0E, NOT just 7.\n"
        "  The mask is 0x0E, and the getter masks with 0x07.\n"
        "  Earlier VioAVR versions had a 0x03 mask (2-bit) bug.\n"
        "\n"
        "─ Timer/Counter ───────────────────────────────────────────────────\n"
        "\n"
        "  TCA (16-bit):\n"
        "    - Normal mode: single 16-bit, TOP in PER, compare in CMPn\n"
        "    - Split mode: two 8-bit timers, each with 2 compares\n"
        "    - Waveform output on port pins via WO\n"
        "    - Prescaler: CLK_PER / 1,2,4,8,16,64,256\n"
        "    - On XMEGA, WG mode is in CTRLD, NOT CTRLB.\n"
        "\n"
        "  TCB (16-bit):\n"
        "    - Simpler timer, supports time-out check, input capture, PWM\n"
        "    - CLKSEL options: CLK_PER (default), CLK_TCA, event system (CLKSEL=7)\n"
        "    - When clocked by event, TCB increments on each event pulse\n"
        "    - Cascades: TCA overflow can directly clock TCB\n"
        "\n"
        "  TCD (12-bit, AVR-DD only):\n"
        "    - Advanced timer with dual outputs, dead-band insertion\n"
        "    - Error correction / fault protection\n"
        "    - Synchronized operation possible\n"
        "\n"
        "  XMEGA TC (16-bit):\n"
        "    - Counters TC0-TC4, each with 2 compare channels (CCA, CCB)\n"
        "    - Waveform Generation mode in CTRLD (NOT CTRLB)\n"
        "    - AWEX extends TC: dead-time, pattern generation, swap\n"
        "    - AWEX+TC share pins; VioAVR uses is_enabled() guard\n"
        "\n"
        "─ CCL (Configurable Custom Logic) ─────────────────────────────────\n"
        "\n"
        "  Up to 4 LUTs. Each LUT has 3 inputs with independent mux:\n"
        "    - Input 0: IN pin, AC0, TCA0, TCB0, event, or VCC/GND\n"
        "    - Input 1: IN pin, AC1, TCA1, TCB1, event, or VCC/GND\n"
        "    - Input 2: IN pin, AC2, TCO, event, or VCC/GND\n"
        "\n"
        "  TRUTH register is an 8-bit LUT (bit 0 = input 000, bit 7 = 111).\n"
        "  Example: AND gate → TRUTH = 0x80 (only output high when all 3 high).\n"
        "  Example: OR gate  → TRUTH = 0xFE (output high unless all 3 low).\n"
        "\n"
        "  CCL output can drive a pin (CCLx_OUT) or become an event generator.\n"
        "  Must enable CCL, the specific LUT, and set clock source.\n"
        "\n"
        "─ Interrupt Controller (CPUINT) ───────────────────────────────────\n"
        "\n"
        "  On megaAVR-0, tinyAVR-0/1/2, AVR-Dx/Ex:\n"
        "    - CPUINT manages interrupt priority and vector handling\n"
        "    - Round-robin scheduling: CPUINT.CTRLA bit 1 = RREN\n"
        "    - Interrupt levels (LVL): 0=off, 1=low, 2=medium, 3=high\n"
        "    - IVSEL: select application vs boot interrupt vector section\n"
        "\n"
        "  Classic AVRs: fixed priority (lower vector = higher priority).\n"
        "  No CPUINT peripheral.\n"
        "\n"
        "─ ADC ────────────────────────────────────────────────────────────\n"
        "\n"
        "  Modern AVRs:\n"
        "    - Resolution: 10-bit (megaAVR-0, tinyAVR) or 12-bit (AVR-DB/DD)\n"
        "    - Window comparator mode: compare result against WINLT/WINHT\n"
        "    - Accumulation and decimation (sample averaging)\n"
        "    - Trigger sources: software, event, timer, free-running\n"
        "    - Internal references: VDD, 1.024V, 2.048V, 2.5V, 4.096V\n"
        "\n"
        "  Co-sim analog inputs: ADC channels map to A-device analog input\n"
        "  pins (128 input channels available).\n"
        "\n"
        "─ NVMCTRL (Non-Volatile Memory Controller) ───────────────────────\n"
        "\n"
        "  Flash self-programming:\n"
        "    1. Wait NVMCTRL.STATUSB = ready\n"
        "    2. Load page buffer with data (serial writes to NVMCTRL.DATA)\n"
        "    3. Set NVMCTRL.CTRLA to page erase/command\n"
        "    4. Set NVMCTRL.CTRLA to page write command\n"
        "    5. Wait NVMCTRL.STATUSB = ready\n"
        "\n"
        "  NVMCTRL.CTRLC bits control wait states based on clock speed:\n"
        "    0-12 MHz: 0 wait states (READY)\n"
        "    12-24 MHz: 1 wait state\n"
        "    24-48 MHz: 2 wait states\n"
        "\n"
        "─ GPIO ───────────────────────────────────────────────────────────\n"
        "\n"
        "  Modern AVRs:\n"
        "    - PORTx.DIR = direction (1=output, 0=input)\n"
        "    - PORTx.OUT = output value\n"
        "    - PORTx.IN = input value (read only)\n"
        "    - PORTx.PINnCTRL per-pin configuration: pull-up, slew rate, etc.\n"
        "    - PORTx.VPCTRL for multi-voltage I/O (AVR-Dx)\n"
        "\n"
        "  Classic AVRs:\n"
        "    - DDRx = direction\n"
        "    - PORTx = output value + pull-up enable (when input)\n"
        "    - PINx = input value (write to toggle PORT on some families)\n"
        "\n"
        "─ Co-Simulation Netlist ───────────────────────────────────────────\n"
        "\n"
        "  When building ngspice netlists with the d_vioavr A-device:\n"
        "\n"
        "    .spiceinit  (NOT .codemodel — that is not a valid dot command)\n"
        "\n"
        "    A_d_vioavr  [digital_pins]  [analog_in_pins]  [analog_out_pins]\n"
        "      + instance=<name>\n"
        "      + freq=<hz>\n"
        "      + VIH=<V>\n"
        "      + VIL=<V>\n"
        "\n"
        "  Conventions:\n"
        "    - Digital pin count = 32 (PIN_COUNT)\n"
        "    - A-device vectors = 32\n"
        "    - Step size = 1 us (shim_batches)\n"
        "    - Pull-up resistors: 10 kOmega to VCC on open-drain buses\n"
        "    - Analog inputs: index 0-127, outputs: 0-7\n"
        "    - Use d_cosim + dac_bridge + R_pull circuit for level translation\n"
        "\n"
        "  Typical bridge invocation:\n"
        "    vioavr bridge --mcu atmega4809 --instance my_project\n"
        "\n"
        "─ VioAVR-Specific Quirks ──────────────────────────────────────────\n"
        "\n"
        "  1. XMEGA ClkCtrl writes are skipped when tc_count > 0\n"
        "     (machine.cpp:237, viospice.cpp:157) — by design for\n"
        "     co-simulation. Default 32 MHz is used.\n"
        "\n"
        "  2. TCB CLKSEL field is 3 bits (mask 0x07, NOT 0x03).\n"
        "     Write (7 << 1) = 0x0E for event-driven clocking.\n"
        "\n"
        "  3. TCA WG mode on XMEGA is in CTRLD, not CTRLB.\n"
        "\n"
        "  4. AWEX shares pins with TC; is_enabled() guard prevents\n"
        "     conflicts when AWEX is not configured.\n"
        "\n"
        "  5. JIT engine saves/restores r14 around compiled blocks.\n"
        "\n"
        "  6. SBIW C flag formula: c = !v15 && r15 is correct.\n"
        "     Immediate range for SBIW is 0-63.\n"
        "\n"
        "  7. SREG bitfield uses bool:1, not u8:1 (carry truncation fix).\n"
        "     Requires -Wpedantic pragma for the union.\n"
        "\n"
        "  8. block_cycles += 1 (1-cycle base for all instructions).\n"
        "\n"
        "  9. LDD/STD q-extraction and bus_data stale-on-OUT have been\n"
        "     fixed in the JIT code path.\n"
        "\n"
        "  10. Classic AVR descriptors are hand-crafted.\n"
        "      Only AVR-Dx/Ex/Lx/Sx are auto-generated from Microchip ATDF.\n"
        "      Regenerating classic descriptors would lose pin-mapping data.\n"
        "\n"
        "  11. Over 290 device descriptors available.\n"
        "      Use 'vioavr list-devices --filter <keyword>' to search.\n"
        "\n"
        "─ Recommended Initialization Sequence ─────────────────────────────\n"
        "\n"
        "  For a typical modern AVR firmware:\n"
        "\n"
        "    1. Disable interrupts (CLI)\n"
        "    2. Configure clock: CLKCTRL, wait for oscillator stable\n"
        "    3. Configure GPIO direction (PORTx.DIR) and pull-ups\n"
        "    4. Configure event system (EVSYS channels + users)\n"
        "    5. Configure CCL (LUT truth tables, inputs, outputs)\n"
        "    6. Configure timers (TCA prescaler, PER, CMP, split mode if used)\n"
        "    7. Configure TCB (CLKSEL, PER, CCMP)\n"
        "    8. Configure ADC (reference, resolution, trigger, channel)\n"
        "    9. Configure interrupts (CPUINT level, enable peripheral IRQ)\n"
        "    10. Enable global interrupts (SEI)\n"
        "    11. Main loop\n"
        "\n"
        "  Error handling: check NVMCTRL.STATUSB before flash operations.\n"
        "  Register access: verify write-enable locks (e.g. NVMCTRL.CTRLA\n"
        "  requires writing the correct key in some families).\n"
    },
    {
        "device-descriptors",
        "Device descriptor generation, accuracy model, and catalog",
        "VioAVR supports 341 device descriptors (343 names, one duplicate entry):\n"
        "  298 auto-generated classic/AVR8X (include/vioavr/core/devices/)\n"
        "  45  auto-generated XMEGA (include/vioavr/core/devices/xmega/)\n"
        "  3   hand-crafted (atmega328p.hpp, atmega4809.hpp, at90pwm3.hpp)\n"
        "\n"
        "Generation Pipeline:\n"
        "  1. Microchip Device Family Packs in avr-pack/*/atdf/ (ATDF XML)\n"
        "  2. tools/gen_device_descriptor.py (3500 loc) parses ATDF, emits\n"
        "     C++20 constexpr DeviceDescriptor headers\n"
        "  3. Two code paths: generate() for classic/AVR8X, generate_xmega()\n"
        "  4. Alternative: scripts/gen_device.py via JSON metadata export\n"
        "  5. Registration: manual in src/core/device_catalog.cpp (linear search)\n"
        "\n"
        "Descriptor struct fields (include/vioavr/core/device.hpp:1350-1608):\n"
        "  Identity: name, flash_words, sram_bytes, sram_start, eeprom_bytes\n"
        "  Interrupts: vector_count, vector_size (2 or 4 bytes)\n"
        "  Memory: flash_page_size, register_file_range, io_range, ext_io_range\n"
        "  AVR8X unified map: mapped_flash, mapped_eeprom, fuses, signatures\n"
        "  Core: spl/sph/sreg/rampz/eind/spmcsr/ccp/prr/smcr/mcusr addresses\n"
        "  Peripheral arrays: ADCs (5 types), ACs (3), timers (9 types),\n"
        "    AWEX, EVSYS, CCL, VREF, CLKCTRL, UART/SPI/TWI/USI, NVMCTRL,\n"
        "    CPUINT, PCINT, EEPROM, WDT, CAN, USB, LCD, PSC, DAC, EBI,\n"
        "    IRCOM, OPAMP, DMA, ZCD, CFD, PTC, LIN\n"
        "  Analog: operating_voltage_v, vil_factor, vih_factor\n"
        "  Ports: port_count (max 16), each with name, pin/ddr/port addr,\n"
        "    AVR8X extensions (dirset/dirclr/dirtgl/outset/outclr/outtgl),\n"
        "    pin_ctrl_base, intflags, vport_base, remap\n"
        "\n"
        "Hand-crafted vs auto-generated:\n"
        "  Hand-crafted are richer: ADC pin maps, auto-trigger sources,\n"
        "  timer OC pin assignments, WGM masks, fuse arrays, PRR bits,\n"
        "  operating voltage. Auto-generated have accurate register\n"
        "  addresses but omit pin-routing and configuration details.\n"
        "\n"
        "Known limitations:\n"
        "  - ATmega4809 UART8x txd/rxd swapped (pre-existing)\n"
        "  - ADC pin routing not populated in auto-generated headers\n"
        "  - Timer OC pin assignments, fuse bytes, PRR bits missing\n"
        "  - Register matching uses prefix heuristics (false positives possible)\n"
        "  - All devices default to 16 MHz (classic) or 32 MHz (XMEGA)\n"
        "  - 3 hand-edited headers have no automated update path\n"
    },
    {
        "a-device",
        "ngspice d_vioavr A-device PDK: shared memory bridge and co-sim shim",
        "VioAVR provides two ngspice integration paths:\n"
        "\n"
        "1) d_vioavr XSPICE A-Device (Shared Memory Bridge)\n"
        "   Interface spec: scratch/cmpp_build/d_vioavr/ifspec.ifs\n"
        "\n"
        "   Ports (3 total):\n"
        "     avr_pins[0..128]       digital inout (PORTB/C/D default)\n"
        "     avr_analog_pins[0..128] analog voltage input (ADC channels)\n"
        "     avr_dac_pins[0..32]    analog voltage output (DAC channels)\n"
        "\n"
        "   Parameters (5):\n"
        "     hex_file        string  \"firmware.hex\"\n"
        "     mcu_type        string  \"atmega328\" (also SHM instance name)\n"
        "     frequency       real    16e6 Hz\n"
        "     vref            real    5.0 V\n"
        "     quantum_cycles  int     1000 cycles per sync step\n"
        "\n"
        "   Implementation: compiled object only (scratch/cmpp_build/\n"
        "   viospice_bridge.o). Source not in tree. Connects via POSIX\n"
        "   shared memory to vioavr-bridge-daemon.\n"
        "\n"
        "2) d_cosim In-Process Co-Simulation Shim\n"
        "   Source: cosim/avr_cosim_shim.cpp -> libavr_cosim.so\n"
        "   Loaded by d_cosim XSPICE model. No SHM — embeds VioAVR ISS\n"
        "   in-process via dlopen.\n"
        "\n"
        "   Architecture:\n"
        "     - Up to MAX_CHIPS = 8 independent AVR cores\n"
        "     - PINS_PER_CHIP = 32 (PORTA=0-7, PORTB=8-15, PORTC=16-23,\n"
        "       PORTD=24-31)\n"
        "     - Global pin ID = chip * 32 + local_pin\n"
        "     - Minimum step: 100 ns\n"
        "\n"
        "   sim_args format:\n"
        "     [\"mcu1\",\"hex1\", ..., \"mcuN\",\"hexN\", \"jit_flag\",\n"
        "      \"hc05\", \"data=<hex>\", \"inject_at=<vtime>:<hex>\",\n"
        "      \"pty_<path>\", \"<adc_voltage>\"]\n"
        "\n"
        "   Pin state mapping:\n"
        "     Output HIGH push-pull    -> {ONE, STRONG}\n"
        "     Output LOW push-pull     -> {ZERO, STRONG}\n"
        "     Open-drain HIGH (released) -> {ZERO, HI_IMPEDANCE}\n"
        "     Open-drain LOW (driven)  -> {ZERO, STRONG}\n"
        "     First step overrides XSPICE default {ZERO,STRONG} to\n"
        "     {ZERO,HI_IMPEDANCE} so pull-ups define initial bus state.\n"
        "\n"
        "3) Analog Bridge XSPICE Models (viospice.cm)\n"
        "   cosim/avr_analog_bridge/ -> build/viospice.cm\n"
        "\n"
        "   avr_adc_bridge(channel=N): injects circuit node voltage into\n"
        "     VioAVR analog signal bank. dlsym to vioavr_cosim_get_active_handle.\n"
        "   avr_dac_bridge(channel=N): reads DAC voltage from VioAVR outputs.\n"
        "   avr_vcc_bridge: propagates VCC to VDD-dependent peripherals.\n"
        "\n"
        "4) Shared Memory Bridge Protocol (out-of-process)\n"
        "   Header: include/vioavr/core/bridge_shm.hpp\n"
        "   Segment: /vioavr_shm_<instance>, magic=0x56494F38 \"VIO8\", v1\n"
        "\n"
        "   Semaphore handshake:\n"
        "     Client writes inputs -> sem_post(sem_req)\n"
        "     Server reads, steps, writes outputs -> sem_post(sem_ack)\n"
        "\n"
        "   Schmitt trigger hysteresis (bridge_shm_server.cpp):\n"
        "     if v >= VIH (3.0V default) -> digital=1\n"
        "     else if v <= VIL (1.5V default) -> digital=0\n"
        "     else: state unchanged\n"
        "     If VIH=VIL=0.0, threshold skipped, digital_inputs[] direct.\n"
        "\n"
        "   Commands: 0x01=RESET, 0x02=LOAD_HEX, 0x10000=TOGGLE_VCD\n"
        "   CPU state snapshot: PC, SP, SREG, RAMPZ, EIND, GPRs[32]\n"
        "   Telemetry: cycles, time_ns, SPM ops, interrupts, sleep cycles\n"
        "\n"
        "5) C API (include/vioavr/core/viospice_c.h)\n"
        "   vioavr_create/destroy/load_hex/reset/enable_jit\n"
        "   vioavr_step_duration/tick_timer2_async\n"
        "   vioavr_add_pin_mapping/set_external_pin/set_external_voltage\n"
        "   vioavr_set_operating_voltage/get_analog_outputs\n"
        "   vioavr_enable_hc05/hc05_has_tx_byte/hc05_read_tx_byte\n"
        "   vioavr_hc05_inject_data/set_hc05_pty_fd\n"
        "   vioavr_get_cycles/consume_pin_changes\n"
    },
    {
        "co-sim-tests",
        "Co-simulation test patterns, golden reference data, and verification methodology",
        "14 co-simulation test sources in tests/core/viospice/:\n"
        "  viospice_ngspice_all_devices_test  (300s timeout, all-device sweep)\n"
        "  viospice_xmega_tc_pwm_test          (XMEGA TC, PER=399, CCA=200)\n"
        "  viospice_xmega_dac_test             (DAC0 data=2048, Vref=5V)\n"
        "  viospice_xmega_ac_adc_test          (AC: 3.0V vs 1.5V, ADC ch0)\n"
        "  viospice_xmega_awex_test            (AWEX complementary PWM)\n"
        "  viospice_multi_i2c_test             (2 ATmega4809 I2C, 0xA5)\n"
        "  viospice_attiny_tca_pwm_test        (ATtiny412 TCA WO0 PA3)\n"
        "  viospice_atmega4809_tca_pwm_test    (ATmega4809 TCA WO0 PA0)\n"
        "  viospice_peripheral_functional_test (8 tests: INT0, UART, AC8X,\n"
        "    PI regulator, PWM+RC filter, AC interrupt, Schmitt trigger, WDT)\n"
        "  bridge_shm_test, attiny_shm_test    (SHM bridge functional)\n"
        "  hc05_fidelity_test, timer2_async_test (non-ngspice)\n"
        "\n"
        "Verification Methodology:\n"
        "  1. Run ngspice batch mode, capture stdout to sim_matrix.log\n"
        "  2. Parse log: \"Index time sig1 sig2 ...\"\n"
        "  3. Bucket voltages: LOW < 0.1V (strict) or < 0.5V, HIGH > 4.0V\n"
        "  4. Count transitions; assert expected toggling behavior\n"
        "  5. No external .ref/.golden/.expected files — all verification\n"
        "     is in-code within test .cpp files\n"
        "\n"
        "Key Expected Values:\n"
        "  DAC: data=2048, Vref=5V => ~2.5V (range 2.0V-3.0V)\n"
        "  XMEGA TC PWM: PER=399, CCA=200, 32MHz => 12.5us period, 50%\n"
        "  PI regulator: setpoint=717 (3.5V), integral clamped +/-2000\n"
        "  PWM+RC filter: OCR1A=153/255=60%, R=1k, C=1u => 3.0V (2.8-3.2)\n"
        "  AC comparator: 3.0V > 1.5V => ACO=1 => PA0 HIGH\n"
        "  UART 0x55: 10-bit frame, ~8.5us/bit, >=6 edges\n"
        "  WDT: PB0 HIGH at 0ms, LOW 13-22ms, HIGH again >25ms\n"
        "\n"
        "Ngspice solver settings (uniform):\n"
        "  .options reltol=0.005 method=gear trtol=7 chgtol=1e-13\n"
        "  Typical .tran 0.5u 50u to .tran 1u 10m\n"
        "  d_cosim: queue_size=1024 irreversible=1 delay=1e-9\n"
        "  Pull-down: 10k on digital probes. I2C: 4.7k pull-ups to VCC\n"
        "\n"
        "Firmware generation: inline C++ strings compiled at test time,\n"
        "  via avr-gcc -mmcu=<chip> -Os -> .elf -> .hex -> VioAVR load.\n"
    },
    {
        "xspice-models",
        "XSPICE event-driven engine and code model architecture",
        "XSPICE is ngspice's event-driven mixed-mode simulation framework\n"
        "(Georgia Tech Research Corp, 1991). It extends SPICE with:\n"
        "  Digital A-devices: event-driven, 4-state by 4-strength resolution\n"
        "  Analog code models: MIF_VOLTAGE/CURRENT/DIFF_VOLTAGE/DIFF_CURRENT\n"
        "  Hybrid models: analog + digital ports, called by EVTcall_hybrids()\n"
        "\n"
        "Signal Resolution (Digital_t):\n"
        "  States: ZERO, ONE, UNKNOWN\n"
        "  Strengths: STRONG, RESISTIVE, HI_IMPEDANCE, UNDETERMINED\n"
        "  Wired-AND/OR resolution for bus contention\n"
        "\n"
        "Event Engine (ngspice/src/xspice/evt/):\n"
        "  evtinit.c: allocates queues, counts hybrid instances\n"
        "  evtdeque.c: dequeues events at matching sim time\n"
        "  evtiter.c: determines which instances to evaluate\n"
        "  evtcall_hybrids.c: calls hybrids after analog Newton iteration\n"
        "  evtbackup.c / evtaccept.c: state backup + commit for rejection\n"
        "\n"
        "Hybrid models:\n"
        "  Identified by mif->is_hybrid flag during EVTinit().\n"
        "  Stored in ckt->evt->info.hybrids[].\n"
        "  Called with CALL_TYPE=MIF_STEP_PENDING after each Newton step.\n"
        "  Can force timestep truncation via cm_analog_set_temp_bkpt().\n"
        "  Irreversible models (d_cosim): no state backup, always called.\n"
        "\n"
        "Code Model Interface (ICM API):\n"
        "  Entry point: void ucm_<name>(Mif_Private_t*)\n"
        "  Interface via .ifs (port/param spec) + .mod (macro source)\n"
        "  Generated: ifspec.c (SPICEdev struct) + cfunc.c (implementation)\n"
        "\n"
        "Key ICM Macros:\n"
        "  INIT, TIME, CALL_TYPE {ANALOG, EVENT, STEP_PENDING, INIT}\n"
        "  PARAM(name), PORT_SIZE(p), PORT_NULL(p)\n"
        "  INPUT(p[i]), OUTPUT(p[i]), OUTPUT_STATE(p[i]), OUTPUT_STRENGTH(p[i])\n"
        "  OUTPUT_DELAY(p[i], t), OUTPUT_CHANGED(p[i])\n"
        "  cm_event_get_ptr(0/1, offset), cm_event_alloc(id, sz)\n"
        "  cm_event_queue(time), cm_analog_set_temp_bkpt(time)\n"
        "  cm_irreversible(n), STATIC_VAR(name)\n"
        "\n"
        "Co-Simulation Protocol (cosim.h):\n"
        "  .so must export Cosim_setup(struct co_info*):\n"
        "    in_count, out_count, inout_count, handle, vtime, method\n"
        "    cleanup(), step(), in_fn(), out_fn() callbacks\n"
        "    lib_argc/argv, sim_argc/argv, dlopen_fn\n"
        "\n"
        "d_cosim Event Flow:\n"
        "  1. check_input(): detect changes, queue as pend_in structs\n"
        "  2. run(): iterate queue, call in_fn + step for each event\n"
        "  3. advance(): if output from future, cm_analog_set_temp_bkpt()\n"
        "  4. accept_output() buffers changes, output() dispatches with delay\n"
        "\n"
        "30+ digital code models in ngspice:\n"
        "  d_and, d_buffer, d_dff, d_dlatch, d_fdiv, d_genlut, d_inverter,\n"
        "  d_jkff, d_lut, d_nand, d_nor, d_open_c, d_open_e, d_or, d_osc,\n"
        "  d_process, d_pulldown, d_pullup, d_pwm, d_ram, d_source, d_srff,\n"
        "  d_srlatch, d_state, d_cosim, adc_bridge, bidi_bridge\n"
        "\n"
        "VioAVR-specific models (viospice.cm):\n"
        "  avr_adc_bridge, avr_dac_bridge, avr_vcc_bridge\n"
        "  Registered via dlmain.c with cmDEVices[] + cmDEVICESCNT exports.\n"
        "  Loaded via .codemodel /path/to/viospice.cm in netlist.\n"
    },
    {
        "list",
        "List available documentation topics",
        "Usage:\n"
        "  vioavr docs              List all topics\n"
        "  vioavr docs <topic>      Show documentation for <topic>\n"
        "  vioavr docs --search <k> Search documentation for keyword\n"
    },
};

static constexpr size_t kDocCount = sizeof(DOCS) / sizeof(DOCS[0]);

// ---------------------------------------------------------------------------
// Display helpers
// ---------------------------------------------------------------------------
static void print_topic_header(const DocEntry& entry) {
    std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
              << "docs: " << entry.name
              << Terminal::reset_all() << "\n"
              << Terminal::fg(Terminal::Color::bright_black)
              << std::string(Terminal::width(), '-')
              << Terminal::reset_all() << "\n";
}

static void print_topic_long(const DocEntry& entry) {
    std::cout << Terminal::fg(Terminal::Color::bright_black) << entry.short_desc
              << Terminal::reset_all() << "\n\n";
    std::cout << entry.long_text << "\n";
}

static void print_topic_list() {
    std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
              << "docs: available topics"
              << Terminal::reset_all() << "\n"
              << Terminal::fg(Terminal::Color::bright_black)
              << std::string(Terminal::width(), '-')
              << Terminal::reset_all() << "\n\n";

    size_t name_w = 0;
    for (auto& d : DOCS) {
        if (d.name.size() > name_w) name_w = d.name.size();
    }
    name_w = std::min(name_w + 2, size_t(20));

    // Header
    std::cout << Terminal::style(Terminal::Style::bold)
              << "  " << std::left << std::setw(static_cast<int>(name_w)) << "Topic"
              << "  Description"
              << Terminal::reset_all() << "\n";
    std::cout << Terminal::fg(Terminal::Color::bright_black)
              << "  " << std::string(name_w, '-') << "  "
              << std::string(Terminal::width() - name_w - 4, '-')
              << Terminal::reset_all() << "\n";

    for (auto& d : DOCS) {
        std::cout << "  " << Terminal::fg(Terminal::Color::green)
                  << std::left << std::setw(static_cast<int>(name_w)) << d.name
                  << Terminal::reset_all()
                  << "  " << d.short_desc << "\n";
    }
    std::cout << "\n"
              << Terminal::fg(Terminal::Color::bright_black)
              << "Run 'vioavr docs <topic>' for details on a specific topic."
              << Terminal::reset_all() << "\n";
}

static void print_search_results(const std::string& keyword) {
    std::string lower = keyword;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
              << "docs: search results for '" << keyword << "'"
              << Terminal::reset_all() << "\n"
              << Terminal::fg(Terminal::Color::bright_black)
              << std::string(Terminal::width(), '-')
              << Terminal::reset_all() << "\n\n";

    bool found = false;
    for (auto& d : DOCS) {
        // Search in name, short desc, and long text
        std::string haystack(d.name);
        haystack += " ";
        haystack += d.short_desc;
        haystack += " ";
        haystack += d.long_text;

        std::string haystack_lower = haystack;
        std::transform(haystack_lower.begin(), haystack_lower.end(), haystack_lower.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (haystack_lower.find(lower) == std::string::npos) continue;

        found = true;
        std::cout << Terminal::fg(Terminal::Color::green)
                  << d.name << Terminal::reset_all()
                  << Terminal::fg(Terminal::Color::bright_black)
                  << "  -  " << Terminal::reset_all()
                  << d.short_desc << "\n";

        // Find matching lines in long text
        std::string_view text = d.long_text;
        size_t pos = 0;
        while (pos < text.size()) {
            auto eol = text.find('\n', pos);
            std::string_view line = text.substr(pos, eol - pos);
            std::string line_lower(line);
            std::transform(line_lower.begin(), line_lower.end(), line_lower.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            if (line_lower.find(lower) != std::string::npos) {
                std::cout << "    " << Terminal::fg(Terminal::Color::bright_black)
                          << "│ " << Terminal::reset_all() << line << "\n";
            }
            pos = (eol == std::string_view::npos) ? text.size() : eol + 1;
        }
        std::cout << "\n";
    }

    if (!found) {
        std::cout << Terminal::fg(Terminal::Color::yellow)
                  << "  No matches found for '" << keyword << "'\n"
                  << Terminal::reset_all()
                  << "  Run 'vioavr docs' to list all topics.\n";
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
static void docs_show_topic(const std::string& topic) {
    std::string t = topic;
    // Strip leading '.' (like FerroMNA: "docs .tran" works like "docs tran")
    while (!t.empty() && t[0] == '.') t.erase(0, 1);

    if (t == "list" || t == "--list" || t == "-l") {
        print_topic_list();
        return;
    }

    for (auto& d : DOCS) {
        if (d.name == t) {
            print_topic_header(d);
            print_topic_long(d);
            return;
        }
    }

    std::cout << Terminal::fg(Terminal::Color::yellow)
              << "  Topic '" << topic << "' not found.\n"
              << Terminal::reset_all()
              << "  Run 'vioavr docs' to list all topics.\n";
}
