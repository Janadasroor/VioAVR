#!/usr/bin/env python3
"""Check repeated START: verify PA0 went HIGH (rx == tx)."""
import sys

log = sys.argv[1] if len(sys.argv) > 1 else "sim_matrix.log"
with open(log) as f:
    lines = [l.strip() for l in f if l.strip() and not l.startswith("Index")]

pa0_vals = []
for l in lines:
    parts = l.split()
    if len(parts) >= 2:
        try:
            pa0 = float(parts[2])
            pa0_vals.append(pa0)
        except ValueError:
            continue

if not pa0_vals:
    print("FAIL: no PA0 data found")
    sys.exit(1)

peak = max(pa0_vals)
print(f"PA0 peak: {peak:.4f}V ({len(pa0_vals)} samples)")

if peak > 4.0:
    print("PASS: PA0 went HIGH (repeated START round-trip OK)")
    sys.exit(0)
else:
    print("FAIL: PA0 never went HIGH (repeated START failed)")
    sys.exit(1)
