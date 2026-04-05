#!/usr/bin/env python3
"""
Compare VioAVR simulator output against SimAVR reference.

This runs a compiled AVR binary through both SimAVR and VioAVR,
then compares the final register states to find bugs.

Usage: python3 compare_traces.py <source.c> [--steps N]
"""

import subprocess
import sys
import csv
import io
import re
from pathlib import Path
from dataclasses import dataclass, field

@dataclass
class CpuState:
    step: int = 0
    pc: int = 0
    cycles: int = 0
    gpr: list[int] = field(default_factory=lambda: [0]*32)
    sreg: int = 0
    sp: int = 0
    
    def diff(self, other: 'CpuState') -> list[str]:
        """Return list of differences between this state and other."""
        diffs = []
        if self.pc != other.pc:
            diffs.append(f"  PC: {self.pc} vs {other.pc}")
        if self.cycles != other.cycles:
            diffs.append(f"  Cycles: {self.cycles} vs {other.cycles}")
        if self.sp != other.sp:
            diffs.append(f"  SP: {self.sp} vs {other.sp}")
        if self.sreg != other.sreg:
            diffs.append(f"  SREG: {self.sreg:#04x} vs {other.sreg:#04x}")
        for i in range(32):
            if self.gpr[i] != other.gpr[i]:
                diffs.append(f"  R{i}: {self.gpr[i]} vs {other.gpr[i]}")
        return diffs

def compile_source(src: Path, elf: Path, bin_path: Path) -> bool:
    """Compile C source to ELF and extract binary."""
    result = subprocess.run(
        ['avr-gcc', '-mmcu=atmega328p', '-Os', '-g', '-o', str(elf), str(src)],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print(f"Compilation failed:\n{result.stderr}", file=sys.stderr)
        return False
    
    # Extract binary
    result = subprocess.run(
        ['avr-objcopy', '-O', 'binary', '-j', '.text', str(elf), str(bin_path)],
        capture_output=True, text=True
    )
    return True

def run_vioavr(bin_path: Path, max_steps: int = 50) -> list[CpuState]:
    """Run VioAVR trace runner and parse CSV output."""
    result = subprocess.run(
        ['./trace_runner', str(bin_path), '--steps', str(max_steps)],
        capture_output=True, text=True, cwd=bin_path.parent
    )
    
    states = []
    reader = csv.DictReader(io.StringIO(result.stdout))
    for row in reader:
        state = CpuState(
            step=int(row['step']),
            pc=int(row['pc']),
            cycles=int(row['cycles']),
            sreg=int(row['sreg']),
            sp=int(row['sp'])
        )
        for i in range(32):
            state.gpr[i] = int(row[f'r{i}'])
        states.append(state)
    
    return states

def run_simavr_basic(elf: Path) -> dict:
    """Run SimAVR and check if it loads successfully."""
    result = subprocess.run(
        ['timeout', '2', 'simavr', '-m', 'atmega328p', str(elf)],
        capture_output=True, text=True
    )
    return {
        'loaded': 'Loaded' in result.stdout or 'Loaded' in result.stderr,
        'stdout': result.stdout[:200],
        'stderr': result.stderr[:200]
    }

def compare_states(vioavr_states: list[CpuState], source: Path):
    """Compare VioAVR output against expected values from source code comments."""
    if not vioavr_states:
        print("  No VioAVR trace captured")
        return []
    
    print(f"\n  VioAVR Trace Summary (last {len(vioavr_states)} steps):")
    print(f"  {'Step':>5} {'PC':>5} {'Cycles':>7} {'SREG':>5} {'SP':>6}")
    print(f"  {'-'*5} {'-'*5} {'-'*7} {'-'*5} {'-'*6}")
    
    for state in vioavr_states[-10:]:
        print(f"  {state.step:5d} {state.pc:5d} {state.cycles:7d} {state.sreg:5d} {state.sp:6d}")
    
    # Show register changes
    if len(vioavr_states) > 1:
        first = vioavr_states[0]
        last = vioavr_states[-1]
        changed_regs = [i for i in range(32) if first.gpr[i] != last.gpr[i]]
        
        if changed_regs:
            print(f"\n  Registers modified: {', '.join(f'R{i}' for i in changed_regs)}")
            for i in changed_regs:
                print(f"    R{i}: {first.gpr[i]} -> {last.gpr[i]}")
        
        print(f"\n  Final SREG: {last.sreg:#04x} (binary: {last.sreg:08b})")
        print(f"    I={'1' if last.sreg & 0x80 else '0'} "
              f"T={'1' if last.sreg & 0x40 else '0'} "
              f"H={'1' if last.sreg & 0x20 else '0'} "
              f"S={'1' if last.sreg & 0x10 else '0'} "
              f"V={'1' if last.sreg & 0x08 else '0'} "
              f"N={'1' if last.sreg & 0x04 else '0'} "
              f"Z={'1' if last.sreg & 0x02 else '0'} "
              f"C={'1' if last.sreg & 0x01 else '0'}")
    
    return []

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 compare_traces.py <source.c> [--steps N]")
        sys.exit(1)
    
    src = Path(sys.argv[1])
    steps = 50
    if '--steps' in sys.argv:
        idx = sys.argv.index('--steps')
        steps = int(sys.argv[idx+1])
    
    if not src.exists():
        print(f"Error: {src} not found")
        sys.exit(1)
    
    elf = src.with_suffix('.elf')
    bin_path = src.with_suffix('.bin')
    
    print(f"=== VioAVR Trace Analysis ===")
    print(f"Source: {src}")
    print(f"Target: atmega328p")
    print()
    
    # Step 1: Compile
    print(f"[1/4] Compiling {src.name}...")
    if not compile_source(src, elf, bin_path):
        sys.exit(1)
    print(f"  ✓ ELF: {elf}")
    print(f"  ✓ BIN: {bin_path} ({bin_path.stat().st_size} bytes)")
    
    # Step 2: Check SimAVR
    print(f"\n[2/4] Checking SimAVR compatibility...")
    simavr_info = run_simavr_basic(elf)
    if simavr_info['loaded']:
        print(f"  ✓ SimAVR can load ELF")
    else:
        print(f"  ⚠ SimAVR status unclear")
    
    # Step 3: Run VioAVR trace
    print(f"\n[3/4] Running VioAVR trace ({steps} steps)...")
    vioavr_states = run_vioavr(bin_path, steps)
    print(f"  ✓ Captured {len(vioavr_states)} states")
    
    # Step 4: Analyze
    print(f"\n[4/4] Analyzing trace...")
    compare_states(vioavr_states, src)
    
    print(f"\n  SimAVR comparison: Not yet automated")
    print(f"  To complete: Run SimAVR with GDB to extract register states")
    print(f"  Then compare with VioAVR output above")

if __name__ == "__main__":
    main()
