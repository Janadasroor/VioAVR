#!/usr/bin/env python3
"""
Compare VioAVR simulator output against SimAVR reference.

Usage: python3 compare.py <source.c> [--steps N]
"""

import subprocess
import sys
import os
import time
import tempfile
from pathlib import Path

def compile_c_source(src: Path, elf: Path) -> bool:
    """Compile C source to ELF with avr-gcc."""
    result = subprocess.run(
        ["avr-gcc", "-mmcu=atmega328p", "-Os", "-g", "-o", str(elf), str(src)],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print(f"Compilation failed:\n{result.stderr}", file=sys.stderr)
        return False
    return True

def extract_bin(elf: Path, bin_path: Path) -> bool:
    """Extract raw binary from ELF."""
    result = subprocess.run(
        ["avr-objcopy", "-O", "binary", str(elf), str(bin_path)],
        capture_output=True, text=True
    )
    return result.returncode == 0

def run_simavr_trace(elf: Path, max_steps: int) -> list[str]:
    """Extract register trace from SimAVR using GDB."""
    simavr = subprocess.Popen(
        ["simavr", "-g", "-m", "atmega328p", str(elf)],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    time.sleep(2)

    # Build GDB Python script
    gdb_script = f"""
import gdb
import sys

trace_runner = gdb.execute("stepi", to_string=True)

# Register names
print("step,pc,sreg,sp," + ",".join([f"r{{i}}" for i in range(32)]))

for step in range({max_steps}):
    try:
        pc = int(gdb.parse_and_eval('$pc'))
        frame = gdb.selected_frame()
        sreg = int(frame.read_register('SREG'))
        sp = int(frame.read_register('SP'))
        regs = [int(frame.read_register(f'r{{i}}')) for i in range(32)]
        
        print(f"{{step}},{{pc}},{{sreg}},{{sp}}," + ",".join(str(r) for r in regs))
        
        if step < {max_steps} - 1:
            gdb.execute('stepi', to_string=True)
    except Exception as e:
        print(f"Error at step {{step}}: {{e}}", file=sys.stderr)
        break
"""

    script_path = "/tmp/gdb_simavr.py"
    with open(script_path, "w") as f:
        f.write(gdb_script)

    try:
        result = subprocess.run(
            ["avr-gdb", "-batch", "-nx",
             "-ex", f"file {elf}",
             "-ex", "target remote localhost:1234",
             "-ex", f"source {script_path}"],
            capture_output=True, text=True, timeout=60
        )
        
        lines = []
        for line in result.stdout.split('\n'):
            if line.strip() and line.strip()[0].isdigit():
                lines.append(line.strip())
        
        return lines
        
    except subprocess.TimeoutExpired:
        print("Error: GDB timed out", file=sys.stderr)
        return []
    finally:
        simavr.terminate()
        simavr.wait(timeout=5)
        if os.path.exists(script_path):
            os.unlink(script_path)

def run_vioavr_trace(bin_path: Path, max_steps: int) -> list[str]:
    """Extract register trace from VioAVR."""
    trace_runner = "/home/jnd/cpp_projects/VioAVR/tests/programs/trace_runner"
    if not os.path.exists(trace_runner):
        print(f"Error: {trace_runner} not found", file=sys.stderr)
        return []

    result = subprocess.run(
        [trace_runner, str(bin_path), "--steps", str(max_steps)],
        capture_output=True, text=True, timeout=60
    )

    lines = []
    for line in result.stdout.split('\n'):
        if line.strip() and line.strip()[0].isdigit():
            lines.append(line.strip())
    
    return lines

def parse_csv_line(line: str) -> dict:
    """Parse a CSV trace line into a dict."""
    parts = line.split(',')
    if len(parts) < 36:
        return {}
    return {
        'step': int(parts[0]),
        'pc': int(parts[1]),
        'sreg': int(parts[2]),
        'sp': int(parts[3]),
        'regs': [int(parts[4+i]) for i in range(32)]
    }

def compare_traces(simavr_lines: list[str], vioavr_lines: list[str]):
    """Compare two traces and report differences."""
    max_steps = min(len(simavr_lines), len(vioavr_lines))
    if max_steps == 0:
        print("No trace data to compare")
        return 0

    bugs = 0
    for i in range(max_steps):
        sim = parse_csv_line(simavr_lines[i])
        vio = parse_csv_line(vioavr_lines[i])
        
        if not sim or not vio:
            continue

        if sim['pc'] != vio['pc']:
            bugs += 1
            print(f"Step {i}: PC mismatch - SimAVR={sim['pc']:#06x} vs VioAVR={vio['pc']:#06x}")
        
        if sim['sreg'] != vio['sreg']:
            bugs += 1
            print(f"Step {i}: SREG mismatch - SimAVR={sim['sreg']:#04x} vs VioAVR={vio['sreg']:#04x}")
        
        if sim['sp'] != vio['sp']:
            bugs += 1
            print(f"Step {i}: SP mismatch - SimAVR={sim['sp']:#06x} vs VioAVR={vio['sp']:#06x}")

        for r in range(32):
            if sim['regs'][r] != vio['regs'][r]:
                bugs += 1
                print(f"Step {i}: R{r} mismatch - SimAVR={sim['regs'][r]:#04x} vs VioAVR={vio['regs'][r]:#04x}")

    print(f"\n{'=' * 60}")
    print(f"Compared {max_steps} steps")
    print(f"Bugs found: {bugs}")
    
    if bugs == 0:
        print("All register states match! VioAVR agrees with SimAVR.")
    else:
        print(f"Found {bugs} discrepancies in {max_steps} steps")
    
    return bugs

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 compare.py <source.c> [--steps N]")
        sys.exit(1)

    src = Path(sys.argv[1])
    if not src.exists():
        print(f"Error: {src} not found")
        sys.exit(1)

    steps = 50
    if '--steps' in sys.argv:
        idx = sys.argv.index('--steps')
        steps = int(sys.argv[idx + 1])

    elf = src.with_suffix('.elf')
    bin_path = src.with_suffix('.bin')

    print(f"=== SimAVR vs VioAVR Comparison ===")
    print(f"Source: {src}")
    print(f"Steps:  {steps}")
    print()

    # Step 1: Compile
    print("[1/5] Compiling...")
    if not compile_c_source(src, elf):
        sys.exit(1)
    print(f"  ELF: {elf}")

    # Step 2: Extract binary
    print("\n[2/5] Extracting binary...")
    if not extract_bin(elf, bin_path):
        print("  Failed to extract binary", file=sys.stderr)
        sys.exit(1)
    print(f"  BIN: {bin_path}")

    # Step 3: Run SimAVR
    print(f"\n[3/5] Running SimAVR ({steps} steps)...")
    simavr_lines = run_simavr_trace(elf, steps)
    print(f"  Captured {len(simavr_lines)} states")

    # Step 4: Run VioAVR
    print(f"\n[4/5] Running VioAVR ({steps} steps)...")
    vioavr_lines = run_vioavr_trace(bin_path, steps)
    print(f"  Captured {len(vioavr_lines)} states")

    # Step 5: Compare
    print(f"\n[5/5] Comparing traces...")
    bugs = compare_traces(simavr_lines, vioavr_lines)
    
    sys.exit(1 if bugs > 0 else 0)

if __name__ == "__main__":
    main()
