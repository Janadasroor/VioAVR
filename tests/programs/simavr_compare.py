#!/usr/bin/env python3
"""
Compare VioAVR simulator output against SimAVR reference.

Workflow:
1. Compile C source to ELF with avr-gcc
2. Run through SimAVR with GDB, extract register traces
3. Run through VioAVR, extract register traces
4. Compare traces to find discrepancies
5. Report bugs in VioAVR

Usage: python3 simavr_compare.py <source.c>
"""

import subprocess
import sys
import re
import tempfile
from pathlib import Path
from dataclasses import dataclass, field

@dataclass
class CpuState:
    """Snapshot of CPU state at a point in time."""
    pc: int = 0
    cycles: int = 0
    gpr: list[int] = field(default_factory=lambda: [0] * 32)
    sreg: int = 0
    sp: int = 0
    
    def diff(self, other: 'CpuState') -> list[str]:
        """Return list of differences between this state and other."""
        diffs = []
        if self.pc != other.pc:
            diffs.append(f"  PC: {self.pc:#06x} vs {other.pc:#06x}")
        if self.cycles != other.cycles:
            diffs.append(f"  Cycles: {self.cycles} vs {other.cycles}")
        if self.sp != other.sp:
            diffs.append(f"  SP: {self.sp:#06x} vs {other.sp:#06x}")
        if self.sreg != other.sreg:
            diffs.append(f"  SREG: {self.sreg:#04x} vs {other.sreg:#04x}")
        for i in range(32):
            if self.gpr[i] != other.gpr[i]:
                diffs.append(f"  R{i}: {self.gpr[i]:#04x} vs {other.gpr[i]:#04x}")
        return diffs

def compile_source(src: Path, out: Path) -> bool:
    """Compile C source to ELF."""
    result = subprocess.run(
        ["avr-gcc", "-mmcu=atmega328p", "-Os", "-g", "-o", str(out), str(src)],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print(f"Compilation failed:\n{result.stderr}", file=sys.stderr)
        return False
    return True

def run_simavr(elf: Path, max_instructions: int = 100) -> list[CpuState]:
    """Run program in SimAVR via GDB and extract register states."""
    # Create GDB command script
    gdb_script = f"""
target remote | simavr -g -m atmega328p {elf}
set pagination off
set logging on
set logging file /tmp/simavr_trace.txt
set logging enabled on

# Extract initial state
info registers
# Step through instructions
"""
    for _ in range(max_instructions):
        gdb_script += "stepi\ninfo registers\n"
    
    gdb_script += "quit\n"
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.gdb', delete=False) as f:
        f.write(gdb_script)
        gdb_script_path = f.name
    
    # Alternative: Use SimAVR's built-in trace
    result = subprocess.run(
        ["simavr", "-t", "-m", "atmega328p", str(elf)],
        capture_output=True, text=True, timeout=10
    )
    
    return []  # TODO: Parse trace output

def run_vioavr(elf: Path, max_instructions: int = 100) -> list[CpuState]:
    """Run program in VioAVR simulator and extract register states."""
    # We need a VioAVR CLI tool that can dump register states
    # For now, return empty
    return []

def compare_traces(simavr_states: list[CpuState], vioavr_states: list[CpuState]):
    """Compare two execution traces and report differences."""
    max_steps = min(len(simavr_states), len(vioavr_states))
    
    print(f"\nComparing {max_steps} execution steps...")
    print("=" * 60)
    
    bugs_found = 0
    for i in range(max_steps):
        diffs = simavr_states[i].diff(vioavr_states[i])
        if diffs:
            bugs_found += 1
            print(f"\nStep {i} - MISMATCH:")
            print(f"  SimAVR:")
            print(f"    PC={simavr_states[i].pc:#06x} Cycles={simavr_states[i].cycles} SREG={simavr_states[i].sreg:#04x}")
            print(f"  VioAVR:")
            print(f"    PC={vioavr_states[i].pc:#06x} Cycles={vioavr_states[i].cycles} SREG={vioavr_states[i].sreg:#04x}")
            print("  Differences:")
            for d in diffs:
                print(d)
    
    print(f"\n{'=' * 60}")
    print(f"Total bugs found: {bugs_found} / {max_steps} steps")
    
    return bugs_found

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 simavr_compare.py <source.c>")
        sys.exit(1)
    
    src = Path(sys.argv[1])
    if not src.exists():
        print(f"Error: {src} not found")
        sys.exit(1)
    
    elf = src.with_suffix('.elf')
    
    print(f"=== SimAVR vs VioAVR Comparison ===")
    print(f"Source: {src}")
    print()
    
    # Step 1: Compile
    print("[1/3] Compiling...")
    if not compile_source(src, elf):
        sys.exit(1)
    print(f"  → {elf}")
    
    # Step 2: Run SimAVR
    print("\n[2/3] Running SimAVR...")
    simavr_states = run_simavr(elf)
    print(f"  Captured {len(simavr_states)} states")
    
    # Step 3: Run VioAVR
    print("\n[3/3] Running VioAVR...")
    vioavr_states = run_vioavr(elf)
    print(f"  Captured {len(vioavr_states)} states")
    
    # Compare
    if simavr_states and vioavr_states:
        compare_traces(simavr_states, vioavr_states)
    else:
        print("\nNote: Trace extraction not yet implemented.")
        print("This script is a framework - SimAVR and VioAVR trace extraction")
        print("need to be implemented for full comparison.")

if __name__ == "__main__":
    main()
