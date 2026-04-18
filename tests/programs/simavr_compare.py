#!/usr/bin/env python3
"""
Compare VioAVR simulator output against SimAVR reference.

Workflow:
1. Compile C source to ELF and HEX with avr-gcc
2. Run through SimAVR (via custom tracer), extract CSV
3. Run through VioAVR (via custom tracer), extract CSV
4. Compare traces to find discrepancies
"""

import csv
import io
import sys
import subprocess
import os
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
    io: dict[str, int] = field(default_factory=dict)
    
    def diff(self, other: 'CpuState') -> list[str]:
        """Return list of differences between this state and other."""
        diffs = []
        if self.pc != other.pc:
            diffs.append(f"  PC: {self.pc:#06x} vs {other.pc:#06x}")
        # Cycles might differ if wait states are different, but usually should match
        if self.cycles != other.cycles:
            diffs.append(f"  Cycles: {self.cycles} vs {other.cycles}")
        if self.sp != other.sp:
            diffs.append(f"  SP: {self.sp:#06x} vs {other.sp:#06x}")
        if self.sreg != other.sreg:
            diffs.append(f"  SREG: {self.sreg:#04x} vs {other.sreg:#04x}")
        for i in range(32):
            if self.gpr[i] != other.gpr[i]:
                diffs.append(f"  R{i}: {self.gpr[i]:#04x} vs {other.gpr[i]:#04x}")
        
        for k in self.io:
            if k in other.io and self.io[k] != other.io[k]:
                diffs.append(f"  {k}: {self.io[k]:#04x} vs {other.io[k]:#04x}")
                
        return diffs

def parse_trace_csv(csv_content: str) -> list[CpuState]:
    # Skip any non-csv lines at the beginning
    lines = csv_content.splitlines()
    csv_start = 0
    for i, line in enumerate(lines):
        if line.startswith("Cycle,PC,SREG"):
            csv_start = i
            break
            
    reader = csv.DictReader(lines[csv_start:])
    states = []
    io_keys = [
        "TCNT0", "TCNT1L", "TCNT1H", "ADCSRA", "ACSR", "UCSR0A", "UDR0",
        "PORTB", "DDRB", "PORTD", "DDRD"
    ]
    
    for row in reader:
        try:
            state = CpuState()
            state.cycles = int(row['Cycle'])
            state.pc = int(row['PC'], 16)
            state.sreg = int(row['SREG'], 16)
            state.sp = int(row['SP'], 16)
            state.gpr = [int(row[f'R{i}'], 16) for i in range(32)]
            for k in io_keys:
                if k in row:
                    state.io[k] = int(row[k], 16)
            states.append(state)
        except (ValueError, KeyError) as e:
            # Skip invalid lines
            continue
    return states

def compile_source(src: Path, elf: Path, hex_file: Path) -> bool:
    """Compile C source to ELF and HEX."""
    result = subprocess.run(
        ["avr-gcc", "-mmcu=atmega328p", "-Os", "-o", str(elf), str(src)],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print(f"Compilation failed:\n{result.stderr}", file=sys.stderr)
        return False
        
    subprocess.run(["avr-objcopy", "-O", "ihex", str(elf), str(hex_file)])
    return True

def run_simavr(elf: Path, cycles: int = 1000) -> list[CpuState]:
    """Run program in SimAVR and extract register states."""
    tracer = Path("./build/tests/simavr_tracer")
    if not tracer.exists():
        print(f"Error: {tracer} not found. Build it first.")
        return []
        
    # Try to ensure LD_LIBRARY_PATH includes simavr if not already there
    env = os.environ.copy()
    simavr_path = "/home/jnd/cpp_projects/simavr/simavr/obj-x86_64-linux-gnu"
    if simavr_path not in env.get("LD_LIBRARY_PATH", ""):
        env["LD_LIBRARY_PATH"] = env.get("LD_LIBRARY_PATH", "") + ":" + simavr_path

    try:
        result = subprocess.run(
            [str(tracer), str(elf), str(cycles)],
            capture_output=True, text=True, env=env, check=True
        )
    except subprocess.CalledProcessError as e:
        print(f"Error: simavr_tracer failed with exit code {e.returncode}")
        print(f"Stderr: {e.stderr}")
        return []
        
    with open("build/simavr_trace.csv", "w") as f:
        f.write(result.stdout)
    return parse_trace_csv(result.stdout)

def run_vioavr(hex_file: Path, cycles: int = 1000) -> list[CpuState]:
    """Run program in VioAVR simulator and extract register states."""
    tracer = Path("./build/tests/core/vioavr_cpu_trace_util")
    if not tracer.exists():
        print(f"Error: {tracer} not found. Rebuild with make.")
        return []
        
    result = subprocess.run(
        [str(tracer), str(hex_file), str(cycles)],
        capture_output=True, text=True
    )
    with open("build/vioavr_trace.csv", "w") as f:
        f.write(result.stdout)
    return parse_trace_csv(result.stdout)

def compare_traces(simavr_states: list[CpuState], vioavr_states: list[CpuState]):
    """Compare two execution traces and report differences."""
    # Find alignment (some sims might start at different cycle offsets)
    # We'll align by PC=0 and Cycle=0 if possible
    
    max_steps = min(len(simavr_states), len(vioavr_states))
    if max_steps == 0:
        print("Error: One of the traces is empty!")
        return 0
        
    print(f"\nComparing {max_steps} execution cycles...")
    print("=" * 60)
    
    bugs_found = 0
    last_mismatch_step = -1
    
    for i in range(max_steps):
        s1 = simavr_states[i]
        s2 = vioavr_states[i]
        
        diffs = s1.diff(s2)
        if diffs:
            bugs_found += 1
            if bugs_found <= 5: # Limit output
                print(f"\nCycle {s1.cycles} [Step {i}] - MISMATCH:")
                print(f"  PC: Sim={s1.pc:#06x} Vio={s2.pc:#06x}")
                for d in diffs:
                    print(d)
            elif bugs_found == 6:
                print("\n... and more mismatches ...")
            
            last_mismatch_step = i
            # Once we diverge, cycles might drift, so we stop or try to resync
            # For now, we report the first 5 and then total
            
    print(f"\n{'=' * 60}")
    if bugs_found == 0:
        print("SUCCESS: Traces match exactly!")
    else:
        print(f"FAILED: Found {bugs_found} mismatched cycles out of {max_steps}")
    
    return bugs_found

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 simavr_compare.py <source.c> [cycles]")
        sys.exit(1)
    
    src = Path(sys.argv[1])
    cycles = int(sys.argv[2]) if len(sys.argv) > 2 else 1000
    
    if not src.exists():
        print(f"Error: {src} not found")
        sys.exit(1)
    
    elf = src.with_suffix('.elf')
    hex_file = src.with_suffix('.hex')
    
    print(f"=== SimAVR vs VioAVR Comparison ===")
    print(f"Source: {src}")
    print(f"Cycles: {cycles}")
    
    # Step 1: Compile
    if not compile_source(src, elf, hex_file):
        sys.exit(1)
    
    # Step 2: Run SimAVR
    simavr_states = run_simavr(elf, cycles)
    print(f"  SimAVR: Captured {len(simavr_states)} states")
    
    # Step 3: Run VioAVR
    vioavr_states = run_vioavr(hex_file, cycles)
    print(f"  VioAVR: Captured {len(vioavr_states)} states")
    
    # Compare
    if simavr_states and vioavr_states:
        bugs = compare_traces(simavr_states, vioavr_states)
        if bugs > 0:
            sys.exit(1)
    else:
        print("\nFailed to extract traces from one or both simulators.")
        sys.exit(1)

if __name__ == "__main__":
    main()
