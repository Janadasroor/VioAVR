#!/usr/bin/env python3
"""Fix cpu.run(N) calls in test files with step_to() helper."""
import re
from pathlib import Path

TESTS = Path("tests/core")

HELPER = """
void step_to(AvrCpu& cpu, u32 target_pc) {
    while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) {
        cpu.step();
    }
}
"""

def fix(filepath: Path) -> int:
    content = filepath.read_text()
    if 'void step_to(' in content:
        return 0
    
    original = content
    changes = 0
    
    # Pattern: cpu.run(N); // comment\n        cpu.step();
    # Replace: step_to(cpu, N); cpu.step();
    def repl_run_step(m):
        nonlocal changes
        changes += 1
        return f'step_to(cpu, {m.group(1)}U);\n{m.group(2)}cpu.step();'
    
    content = re.sub(
        r'cpu\.run\((\d+)\);\s*//[^\n]*\n(\s+)cpu\.step\(\);',
        repl_run_step, content
    )
    
    # Pattern: cpu.run(N); cpu.step(); // no comment
    def repl_rs(m):
        nonlocal changes
        changes += 1
        return f'step_to(cpu, {m.group(1)}U); cpu.step();'
    content = re.sub(r'cpu\.run\((\d+)\);\s*cpu\.step\(\);', repl_rs, content)
    
    # Pattern: cpu.run(N); // comment\n        auto s =
    def repl_ra(m):
        nonlocal changes
        changes += 1
        return f'step_to(cpu, {m.group(1)}U);\n{m.group(2)}auto s ='
    content = re.sub(
        r'cpu\.run\((\d+)\);\s*//[^\n]*\n(\s+)auto s =',
        repl_ra, content
    )
    
    # Pattern: cpu.run(N); // Run until halt
    def repl_rh(m):
        nonlocal changes
        changes += 1
        return f'step_to(cpu, {m.group(1)}U);'
    content = re.sub(
        r'cpu\.run\((\d+)\);\s*// Run until halt',
        repl_rh, content
    )
    
    # Pattern: cpu.run(N); // Run to halt
    content = re.sub(
        r'cpu\.run\((\d+)\);\s*// Run to halt',
        lambda m: f'step_to(cpu, {m.group(1)}U);',
        content
    )
    if 'step_to(cpu,' in content and 'cpu.run(' not in content.split('step_to')[0].split('\n')[-1]:
        pass  # counted above if replaced
    
    if changes > 0:
        # Add helper before } // namespace
        content = content.replace('\n} // namespace\n', HELPER + '\n} // namespace\n')
        filepath.write_text(content)
    
    return changes

total = 0
for f in sorted(TESTS.glob("cpu_*.cpp")):
    c = fix(f)
    if c:
        print(f"  {f.name}: {c}")
        total += c
print(f"\nTotal: {total} fixes")
