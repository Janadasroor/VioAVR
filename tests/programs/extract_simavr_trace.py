#!/usr/bin/env python3
"""
Extract register trace from SimAVR via GDB Python API.

Usage: python3 extract_simavr_trace.py <elf_file> [--steps N] [--port P]

Outputs CSV: step,pc,sreg,sp,r0,...,r31
"""

import subprocess
import sys
import time
import os
from pathlib import Path

def extract_trace(elf: Path, max_steps: int = 20, port: int = 1234) -> list[dict]:
    """Extract trace using GDB Python API."""
    
    elf_abs = str(elf.absolute())
    
    # Start SimAVR
    simavr = subprocess.Popen(
        ['simavr', '-g', '-m', 'atmega328p', elf_abs],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    time.sleep(2)
    
    # Build GDB Python script
    gdb_python = f'''
import gdb

# Register names for read_register
reg_names = ['SREG', 'SP'] + [f'r{{i}}' for i in range(32)]

print("step,pc,sreg,sp," + ",".join([f"r{{i}}" for i in range(32)]))

for step in range({max_steps + 1}):
    try:
        pc = int(gdb.parse_and_eval('$pc').cast(gdb.lookup_type('long')))
        frame = gdb.selected_frame()
        sreg = int(frame.read_register('SREG'))
        sp = int(frame.read_register('SP').cast(gdb.lookup_type('long')))
        regs = [int(frame.read_register(f'r{{i}}')) for i in range(32)]
        
        print(f"{{step}},{{pc}},{{sreg}},{{sp}}," + ",".join(str(r) for r in regs))
        
        if step < {max_steps}:
            gdb.execute('stepi', to_string=True)
    except Exception as e:
        print(f"Error at step {{step}}: {{e}}", file=sys.stderr)
        break
'''
    
    # Write temp script
    script_path = '/tmp/gdb_simavr_extract.py'
    with open(script_path, 'w') as f:
        f.write(gdb_python)
    
    try:
        result = subprocess.run(
            ['avr-gdb', '-batch', '-nx',
             '-ex', f'file {elf_abs}',
             '-ex', f'target remote localhost:{port}',
             '-ex', f'source {script_path}',
             '-ex', 'quit'],
            capture_output=True, text=True, timeout=120
        )
        
        # Parse CSV output
        traces = []
        for line in result.stdout.split('\n'):
            if line.strip() and line.strip()[0].isdigit():
                parts = line.strip().split(',')
                if len(parts) >= 36:
                    traces.append({
                        'step': int(parts[0]),
                        'pc': int(parts[1]),
                        'sreg': int(parts[2]),
                        'sp': int(parts[3]),
                        'regs': [int(x) for x in parts[4:36]]
                    })
        
        return traces
    
    except subprocess.TimeoutExpired:
        print("Error: GDB timed out", file=sys.stderr)
        return []
    finally:
        simavr.terminate()
        simavr.wait(timeout=5)
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
