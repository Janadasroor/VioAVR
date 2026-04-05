#!/usr/bin/env python3
"""
Compare VioAVR output against SimAVR reference.

This creates a C test program that:
1. Runs in SimAVR via GDB to extract expected register states
2. Runs in VioAVR via a trace-dumping CLI
3. Compares the results

Usage: python3 compare_simavr.py <source.c> [--steps N]
"""

import subprocess
import sys
import re
import os
from pathlib import Path
from dataclasses import dataclass, field

@dataclass
class CpuState:
    pc: int = 0
    gpr: list[int] = field(default_factory=lambda: [0]*32)
    sreg: int = 0
    sp: int = 0
    
    def __getitem__(self, key):
        if key == 'pc': return self.pc
        if key == 'sreg': return self.sreg
        if key == 'sp': return self.sp
        if isinstance(key, int): return self.gpr[key]
        return None
    
    def diff(self, other: 'CpuState', max_regs=32) -> list[tuple[str, int, int]]:
        diffs = []
        if self.pc != other.pc:
            diffs.append(('PC', self.pc, other.pc))
        if self.sp != other.sp:
            diffs.append(('SP', self.sp, other.sp))
        if self.sreg != other.sreg:
            diffs.append(('SREG', self.sreg, other.sreg))
        for i in range(max_regs):
            if self.gpr[i] != other.gpr[i]:
                diffs.append((f'R{i}', self.gpr[i], other.gpr[i]))
        return diffs

def compile_c(src: Path, elf: Path) -> bool:
    result = subprocess.run(
        ['avr-gcc', '-mmcu=atmega328p', '-Os', '-g', '-o', str(elf), str(src)],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print(f"Compile error: {result.stderr}", file=sys.stderr)
        return False
    return True

def get_simavr_disassembly(elf: Path) -> list[tuple[int, str, str]]:
    """Get disassembly from avr-objdump."""
    result = subprocess.run(
        ['avr-objdump', '-d', str(elf)],
        capture_output=True, text=True
    )
    instructions = []
    for line in result.stdout.split('\n'):
        m = re.match(r'\s+([0-9a-f]+):\s+([0-9a-f]{2})\s+([0-9a-f]{2})\s+(\S+)', line)
        if m:
            addr = int(m.group(1), 16) // 2
            opcode = int(m.group(2), 16) | (int(m.group(3), 16) << 8)
            mnemonic = m.group(4)
            instructions.append((addr, f'{opcode:#06x}', mnemonic))
    return instructions

def get_vioavr_trace(elf: Path, max_steps: int = 50) -> list[dict]:
    """Run VioAVR with debug logging to extract trace."""
    # We need to create a test runner that loads the ELF and dumps state
    # For now, use the existing test infrastructure
    return []

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 compare_simavr.py <source.c> [--steps N]")
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
    
    print(f"=== SimAVR Reference Comparison ===")
    print(f"Source: {src}")
    print(f"Target: atmega328p")
    print()
    
    # Step 1: Compile
    print(f"[1/4] Compiling {src.name}...")
    if not compile_c(src, elf):
        sys.exit(1)
    print(f"  ✓ {elf}")
    
    # Step 2: Get disassembly
    print(f"\n[2/4] Disassembling...")
    instructions = get_simavr_disassembly(elf)
    print(f"  Found {len(instructions)} instructions")
    for addr, opcode, mnemonic in instructions[:10]:
        print(f"    PC={addr:3d}: {opcode}  {mnemonic}")
    if len(instructions) > 10:
        print(f"    ... and {len(instructions)-10} more")
    
    # Step 3: Check if we can run SimAVR
    print(f"\n[3/4] Checking SimAVR...")
    result = subprocess.run(
        ['timeout', '2', 'simavr', '-m', 'atmega328p', str(elf)],
        capture_output=True, text=True
    )
    if 'Loaded' in result.stdout:
        print(f"  ✓ SimAVR can load ELF")
    else:
        print(f"  ✗ SimAVR failed: {result.stderr[:100]}")
    
    # Step 4: Report what we need
    print(f"\n[4/4] Next Steps")
    print(f"  To complete comparison, we need:")
    print(f"  1. VioAVR CLI tool that loads ELF and dumps register states")
    print(f"  2. SimAVR GDB script to extract reference states")
    print(f"  3. Comparison logic to diff the two traces")
    print(f"\n  Current approach:")
    print(f"  - We have {len(instructions)} instructions to verify")
    print(f"  - Each instruction needs PC, GPR, SREG, SP state after execution")
    print(f"  - Compare VioAVR output against SimAVR output")
    print(f"  - Report mismatches as bugs to fix")

if __name__ == "__main__":
    main()
