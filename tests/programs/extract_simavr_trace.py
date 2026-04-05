#!/usr/bin/env python3
"""
Extract register trace from SimAVR via GDB 'info registers'.
"""

import subprocess
import sys
import time
import re
import os
from pathlib import Path

def extract_trace(elf: Path, max_steps: int = 20) -> list[dict]:
    """Extract trace using GDB's info registers command."""
    
    # Create GDB script
    gdb_commands = ["set confirm off", "set pagination off"]
    
    for i in range(max_steps):
        if i > 0:
            gdb_commands.append("stepi")
        gdb_commands.append("info registers")
        gdb_commands.append("echo ---END_STEP---\\n")
    
    gdb_commands.append("quit")
    
    # Write to temp file
    script_path = "/tmp/simavr_gdb.cmd"
    with open(script_path, "w") as f:
        f.write("\n".join(gdb_commands))
    
    try:
        # Start SimAVR
        simavr = subprocess.Popen(
            ["simavr", "-g", "-m", "atmega328p", str(elf)],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        time.sleep(3)
        
        # Run GDB
        result = subprocess.run(
            ["avr-gdb", "-batch",
             "-ex", f"target remote localhost:1234",
             "-x", script_path],
            capture_output=True, text=True, timeout=60
        )
        
        # Parse output
        traces = []
        current_step = 0
        regs = {}
        
        for line in result.stdout.split('\n'):
            line = line.strip()
            
            # Match register lines: r0  0x0  0
            match = re.match(r'^(r\d+)\s+0x([0-9a-fA-F]+)', line)
            if match:
                reg_name = match.group(1)
                reg_value = int(match.group(2), 16)
                regs[reg_name] = reg_value
            
            # Match SREG
            match = re.match(r'^SREG\s+0x([0-9a-fA-F]+)', line)
            if match:
                regs['SREG'] = int(match.group(1), 16)
            
            # Match SP
            match = re.match(r'^SP\s+0x([0-9a-fA-F]+)', line)
            if match:
                regs['SP'] = int(match.group(1), 16)
            
            # Match PC
            match = re.match(r'^pc\s+0x([0-9a-fA-F]+)', line)
            if match:
                regs['pc'] = int(match.group(1), 16)
            
            # End of step marker
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
        if os.path.exists(script_path):
            os.unlink(script_path)

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 extract_simavr_trace.py <elf_file> [--steps N]")
        sys.exit(1)
    
    elf = Path(sys.argv[1])
    if not elf.exists():
        print(f"Error: {elf} not found")
        sys.exit(1)
    
    steps = 20
    if '--steps' in sys.argv:
        idx = sys.argv.index('--steps')
        steps = int(sys.argv[idx + 1])
    
    print(f"Extracting SimAVR trace from {elf} ({steps} steps)...", file=sys.stderr)
    traces = extract_trace(elf, steps)
    
    if not traces:
        print("Error: No trace extracted", file=sys.stderr)
        sys.exit(1)
    
    # Output CSV
    print("step,pc,sreg,sp", end="")
    for i in range(32):
        print(f",r{i}", end="")
    print()
    
    for t in traces:
        print(f"{t['step']},{t['pc']},{t['sreg']},{t['sp']}", end="")
        for r in t['regs']:
            print(f",{r}", end="")
        print()
    
    print(f"\nExtracted {len(traces)} states", file=sys.stderr)

if __name__ == "__main__":
    main()
