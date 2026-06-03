#!/usr/bin/env python3
"""Check NACK test: verify PA0 toggled HIGH after NACK + STOP."""
import sys

log = sys.argv[1] if len(sys.argv) > 1 else "sim_matrix.log"
with open(log) as f:
    lines = [l.strip() for l in f if l.strip() and not l.startswith("Index")]

pa0_vals = []
for l in lines:
    parts = l.split()
    if len(parts) >= 3:
        try:
            pa0 = float(parts[2])
            pa0_vals.append(pa0)
        except ValueError:
            continue

if not pa0_vals:
    print("FAIL: no PA0 data found")
    sys.exit(1)

peak = max(pa0_vals)
trough = min(pa0_vals)
print(f"PA0: min={trough:.4f}V max={peak:.4f}V ({len(pa0_vals)} samples)")

if peak > 4.0 and trough < 0.5:
    print(f"PASS: PA0 toggled (NACK detected, STOP sent)")
    sys.exit(0)
elif peak > 4.0:
    print(f"FAIL: PA0 stuck high (no toggle)")
    sys.exit(1)
else:
    print(f"FAIL: PA0 stuck low (NACK/STOP not processed)")
    sys.exit(1)
