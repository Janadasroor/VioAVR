#!/usr/bin/env python3
"""
Batch fix all cpu.run(N) calls in test files.

Strategy:
1. Add step_to() helper to each file
2. Replace cpu.run(N) with step_to(cpu, TARGET) where TARGET is derived from context
3. For patterns like "cpu.run(N); cpu.step()" → "step_to(cpu, N); cpu.step()" (execute instruction N)
4. For patterns like "cpu.run(N);" alone → "step_to(cpu, N+1);" (execute through instruction N-1)
"""

import re
import sys
from pathlib import Path

TESTS_DIR = Path("tests/core")

HELPER = """
void step_to(AvrCpu& cpu, u32 target_pc) {
    while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) {
        cpu.step();
    }
}
"""

def fix_file(filepath: Path) -> int:
    content = filepath.read_text()
    
    # Skip if already has step_to
    if 'void step_to(' in content:
        return 0
    
    changes = 0
    
    # Pattern 1: cpu.run(N); // COMMENT\n        cpu.step(); // COMMENT
    # → step_to(cpu, N); cpu.step();
    # This means: skip to instruction N, then execute it
    p1 = r'cpu\.run\((\d+)\);\s*//[^\n]*\n(\s+)cpu\.step\(\);'
    def repl1(m):
        nonlocal changes
        changes += 1
        n = int(m.group(1))
        indent = m.group(2)
        return f'step_to(cpu, {n}U);\n{indent}cpu.step();'
    content = re.sub(p1, repl1, content)
    
    # Pattern 2: cpu.run(N); // COMMENT\n        auto s = cpu.snapshot();
    # → step_to(cpu, N); (the snapshot captures state AFTER N instructions)
    # But since run(N) executes N instructions, and we want state after,
    # we need step_to(cpu, N) which executes through instruction N-1
    # Actually the test expects state after executing N instructions.
    # step_to(cpu, N) gets PC to N, meaning instructions 0..N-1 have executed. That's correct.
    p2 = r'cpu\.run\((\d+)\);\s*//[^\n]*\n(\s+)auto s = cpu\.snapshot\(\);'
    def repl2(m):
        nonlocal changes
        changes += 1
        n = int(m.group(1))
        indent = m.group(2)
        return f'step_to(cpu, {n}U);\n{indent}auto s = cpu.snapshot();'
    content = re.sub(p2, repl2, content)
    
    # Pattern 3: cpu.run(N); cpu.step(); // no comment
    p3 = r'cpu\.run\((\d+)\);\s*cpu\.step\(\);'
    def repl3(m):
        nonlocal changes
        changes += 1
        n = int(m.group(1))
        return f'step_to(cpu, {n}U); cpu.step();'
    content = re.sub(p3, repl3, content)
    
    # Pattern 4: cpu.run(N); // Reach COMMENT (no step follows, but followed by CHECK)
    # These execute N instructions and check state. step_to(cpu, N) is equivalent.
    p4 = r'cpu\.run\((\d+)\);\s*// Reach[^\n]*\n(\s+)(CHECK|auto )'
    def repl4(m):
        nonlocal changes
        changes += 1
        n = int(m.group(1))
        indent = m.group(2)
        rest = m.group(3)
        return f'step_to(cpu, {n}U);\n{indent}{rest}'
    content = re.sub(p4, repl4, content)
    
    if changes > 0:
        # Add helper before closing namespace
        content = content.replace('\n} // namespace\n', HELPER + '\n} // namespace\n')
        filepath.write_text(content)
    
    return changes

def main():
    files = sorted(TESTS_DIR.glob("cpu_*.cpp"))
    total = 0
    for f in files:
        c = fix_file(f)
        if c > 0:
            print(f"  {f.name}: {c} changes")
            total += c
    print(f"\nTotal: {total} changes across {len(files)} files")

if __name__ == "__main__":
    main()
