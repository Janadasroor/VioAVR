#!/usr/bin/env python3
"""
Compare VioAVR trace against SimAVR reference trace.

Usage: python3 compare_traces.py simavr_trace.csv vioavr_trace.csv

Outputs: list of discrepancies between the two traces.
"""

import sys
import csv
import io

def load_trace(filepath: str) -> list[dict]:
    """Load CSV trace file."""
    traces = []
    with open(filepath) as f:
        reader = csv.DictReader(f)
        for row in reader:
            traces.append({k: int(v) for k, v in row.items()})
    return traces

def compare_traces(simavr: list[dict], vioavr: list[dict]) -> list[str]:
    """Compare two traces and return list of discrepancies."""
    diffs = []
    max_steps = min(len(simavr), len(vioavr))
    
    for i in range(max_steps):
        sa = simavr[i]
        va = vioavr[i]
        
        step_diffs = []
        
        # Compare PC
        if sa.get('pc', -1) != va.get('pc', -2):
            step_diffs.append(f"  PC: SimAVR={sa.get('pc')} VioAVR={va.get('pc')}")
        
        # Compare SREG
        if sa.get('sreg', -1) != va.get('sreg', -2):
            step_diffs.append(f"  SREG: SimAVR={sa.get('sreg'):#04x} VioAVR={va.get('sreg'):#04x}")
        
        # Compare SP
        if sa.get('sp', -1) != va.get('sp', -2):
            step_diffs.append(f"  SP: SimAVR={sa.get('sp')} VioAVR={va.get('sp')}")
        
        # Compare registers
        for j in range(32):
            sv = sa.get(f'r{j}', -1)
            vv = va.get(f'r{j}', -2)
            if sv != vv:
                step_diffs.append(f"  R{j}: SimAVR={sv} VioAVR={vv}")
        
        if step_diffs:
            diffs.append(f"Step {i}:")
            diffs.extend(step_diffs)
    
    return diffs

def main():
    if len(sys.argv) < 3:
        print("Usage: python3 compare_traces.py simavr.csv vioavr.csv")
        sys.exit(1)
    
    simavr_file = sys.argv[1]
    vioavr_file = sys.argv[2]
    
    print(f"Loading SimAVR trace from {simavr_file}")
    simavr = load_trace(simavr_file)
    print(f"  {len(simavr)} states")
    
    print(f"Loading VioAVR trace from {vioavr_file}")
    vioavr = load_trace(vioavr_file)
    print(f"  {len(vioavr)} states")
    
    print(f"\nComparing {min(len(simavr), len(vioavr))} steps...")
    diffs = compare_traces(simavr, vioavr)
    
    if diffs:
        print(f"\nFound {len(diffs) // 2} discrepancies:")
        for d in diffs:
            print(d)
        sys.exit(1)
    else:
        print("\n✓ All states match between SimAVR and VioAVR!")
        sys.exit(0)

if __name__ == "__main__":
    main()
