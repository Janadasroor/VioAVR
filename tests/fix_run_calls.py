#!/usr/bin/env python3
"""
Fix cpu.run(N) calls in test files by replacing with step_to() helper.

Pattern: cpu.run(N) is used to skip to specific instruction positions.
Fix: Replace with step_to(cpu, TARGET_PC) which is explicit and reliable.

Usage: python3 fix_run_calls.py [--dry-run] [file1.cpp file2.cpp ...]
"""

import re
import sys
from pathlib import Path

HELPER = """
void step_to(AvrCpu& cpu, u32 target_pc) {
    while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) {
        cpu.step();
    }
}
"""

def has_step_to(content: str) -> bool:
    return 'void step_to(' in content

def add_helper(content: str) -> str:
    if has_step_to(content):
        return content
    # Add before closing namespace brace
    pattern = r'(\n\} // namespace\n)'
    return re.sub(pattern, HELPER + r'\1', content)

def fix_run_calls(content: str, dry_run: bool = False) -> tuple[str, int]:
    """Replace cpu.run(N) patterns with step_to().
    
    Heuristic: cpu.run(N) where N is small (< 200) is likely trying to reach
    a specific PC position. Replace with step_to(cpu, TARGET_PC) where
    TARGET_PC needs to be determined from context.
    
    For complex patterns, we add the step_to helper and leave a TODO comment.
    """
    changes = 0
    
    # Pattern 1: cpu.run(N); // Reach COMMENT\n        cpu.step();
    # Replace with: step_to(cpu, TARGET); cpu.step();
    # where TARGET = N (assuming 1 cycle per instruction)
    
    # For tests that do cpu.run(N); cpu.step(); - the intent is:
    # "run N instructions, then execute the next one"
    # This is equivalent to: step_to(cpu, N); cpu.step();
    
    # But many tests use cpu.run(N) incorrectly. The safest fix is to
    # add the helper and mark problematic runs for manual review.
    
    # Simple heuristic: if cpu.run(N) is followed by cpu.step(), 
    # the test wants to be AT instruction N, then execute it.
    # So: run(N) + step() = execute instruction N
    # Replace with: step_to(cpu, N); cpu.step();
    
    pattern = r'cpu\.run\((\d+)\);\s*//.*\n(\s+)cpu\.step\(\)'
    
    def replacer(m):
        nonlocal changes
        changes += 1
        n = m.group(1)
        indent = m.group(2)
        return f'step_to(cpu, {n}U);\n{indent}cpu.step()'
    
    content = re.sub(pattern, replacer, content)
    
    # Pattern 2: cpu.run(N); // COMMENT (no following step)
    # This executes N instructions and stops. Equivalent to step_to(cpu, N).
    # But only if followed by a CHECK that inspects state AFTER N instructions.
    # Safe to replace: run(N) → step_to(cpu, N) when no step() follows.
    
    # This is trickier to detect. Let's focus on Pattern 1 for now.
    
    return content, changes

def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("files", nargs="*", help="Test files to fix")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--all", action="store_true", help="Fix all test files")
    
    args = parser.parse_args()
    
    if args.all:
        files = list(Path("tests/core").glob("cpu_*.cpp"))
    elif args.files:
        files = [Path(f) for f in args.files]
    else:
        print("Error: specify --all or file paths", file=sys.stderr)
        sys.exit(1)
    
    total_changes = 0
    for f in files:
        if not f.exists():
            continue
        
        content = f.read_text()
        
        # Skip if already has step_to
        if has_step_to(content):
            continue
        
        fixed, changes = fix_run_calls(content, args.dry_run)
        
        if changes > 0:
            fixed = add_helper(fixed)
            if not args.dry_run:
                f.write_text(fixed)
            print(f"{'[DRY] ' if args.dry_run else ''}Fixed {changes} run() calls in {f.name}")
            total_changes += changes
    
    print(f"\nTotal: {total_changes} run() calls fixed across {len(files)} files")

if __name__ == "__main__":
    main()
