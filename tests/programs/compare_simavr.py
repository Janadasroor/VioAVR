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
import re
from pathlib import Path

def compile_c_source(src: Path, elf: Path) -> bool:
    result = subprocess.run(
        ["avr-gcc", "-mmcu=atmega328p", "-Os", "-g", "-o", str(elf), str(src)],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print(f"Compilation failed:\n{result.stderr}", file=sys.stderr)
        return False
    return True

def extract_bin(elf: Path, bin_path: Path) -> bool:
    result = subprocess.run(
        ["avr-objcopy", "-O", "binary", str(elf), str(bin_path)],
        capture_output=True, text=True
    )
    return result.returncode == 0

def run_simavr_trace(elf: Path, max_steps: int) -> list[str]:
    """Extract register trace from SimAVR using GDB."""
    
    # Create GDB script
    gdb_commands = ["set confirm off", "set pagination off"]
    for i in range(max_steps):
        if i > 0:
            gdb_commands.append("stepi")
        gdb_commands.append("info registers")
        gdb_commands.append("echo ---END_STEP---\n")
    gdb_commands.append("quit")
    
    script_path = "/tmp/simavr_gdb.cmd"
    with open(script_path, "w") as f:
        f.write("\n".join(gdb_commands))
    
    try:
        simavr = subprocess.Popen(
            ["simavr", "-g", "-m", "atmega328p", str(elf)],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        time.sleep(3)
        
        result = subprocess.run(
            ["avr-gdb", "-batch",
             "-ex", "target remote localhost:1234",
             "-x", script_path],
            capture_output=True, text=True, timeout=60
        )
        
        traces = []
        current_step = 0
        regs = {}
        
        for line in result.stdout.split('\n'):
            line = line.strip()
            
            match = re.match(r'^(r\d+)\s+0x([0-9a-fA-F]+)', line)
            if match:
                regs[match.group(1)] = int(match.group(2), 16)
            
            match = re.match(r'^SREG\s+0x([0-9a-fA-F]+)', line)
            if match:
                regs['SREG'] = int(match.group(1), 16)
            
            match = re.match(r'^SP\s+0x([0-9a-fA-F]+)', line)
            if match:
                regs['SP'] = int(match.group(1), 16)
            
            match = re.match(r'^pc\s+0x([0-9a-fA-F]+)', line)
            if match:
                regs['pc'] = int(match.group(1), 16)
            
            if '---END_STEP---' in line:
                if 'pc' in regs and 'SREG' in regs and 'SP' in regs:
                    trace = {
                        'step': current_step,
                        'pc': regs['pc'],
                        'sreg': regs['SREG'],
                        'sp': regs['SP'],
                        'regs': [regs.get(f'r{i}', 0) for i in range(32)]
                    }
                    traces.append(trace)
                current_step += 1
                regs = {}
        
        return traces
        
    except subprocess.TimeoutExpired:
        print("Error: GDB timed out", file=sys.stderr)
        return []
    finally:
        simavr.terminate()
        try:
            simavr.wait(timeout=5)
        except:
            pass
        os.unlink(script_path)

def run_vioavr_trace(bin_path: Path, max_steps: int) -> list[str]:
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
        line = line.strip()
        if line and line[0].isdigit():
            lines.append(line)
    
    return lines

def parse_csv_line(line: str) -> dict:
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

def compare_traces(simavr_lines: list[str], vioavr_lines: list[str]) -> int:
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
    print(f"Compared {max_steps} steps (SimAVR={len(simavr_lines)}, VioAVR={len(vioavr_lines)})")
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
    
    steps = 20
    if '--steps' in sys.argv:
        idx = sys.argv.index('--steps')
        steps = int(sys.argv[idx + 1])
    
    elf = src.with_suffix('.elf')
    bin_path = src.with_suffix('.bin')
    
    print(f"=== SimAVR vs VioAVR Comparison ===")
    print(f"Source: {src}")
    print(f"Steps:  {steps}")
    print()
    
    print("[1/5] Compiling...")
    if not compile_c_source(src, elf):
        sys.exit(1)
    print(f"  ELF: {elf}")
    
    print("\n[2/5] Extracting binary...")
    if not extract_bin(elf, bin_path):
        print("  Failed to extract binary", file=sys.stderr)
        sys.exit(1)
    print(f"  BIN: {bin_path}")
    
    print(f"\n[3/5] Running SimAVR ({steps} steps)...")
    simavr_lines = []
    simavr_traces = run_simavr_trace(elf, steps)
    for t in simavr_traces:
        line = f"{t['step']},{t['pc']},{t['sreg']},{t['sp']}"
        for r in t['regs']:
            line += f",{r}"
        simavr_lines.append(line)
    print(f"  Captured {len(simavr_lines)} states")
    
    print(f"\n[4/5] Running VioAVR ({steps} steps)...")
    vioavr_lines = run_vioavr_trace(bin_path, steps)
    print(f"  Captured {len(vioavr_lines)} states")
    
    print(f"\n[5/5] Comparing traces...")
    bugs = compare_traces(simavr_lines, vioavr_lines)
    
    sys.exit(1 if bugs > 0 else 0)

if __name__ == "__main__":
    main()
