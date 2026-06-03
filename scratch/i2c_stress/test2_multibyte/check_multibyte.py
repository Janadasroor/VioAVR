#!/usr/bin/env python3
"""Check multi-byte burst: verify DAC voltage matches XOR checksum of 16 bytes."""
import sys

log = sys.argv[1] if len(sys.argv) > 1 else "sim_matrix.log"
with open(log) as f:
    lines = [l.strip() for l in f if l.strip() and not l.startswith("Index")]

dac_vals = []
for l in lines:
    parts = l.split()
    if len(parts) >= 2:
        try:
            dac = float(parts[2])
            dac_vals.append(dac)
        except ValueError:
            continue

if not dac_vals:
    print("FAIL: no DAC data found")
    sys.exit(1)

end_region = dac_vals[-min(1000, len(dac_vals)):]
stable = max(end_region) - min(end_region)
avg_dac = sum(end_region) / len(end_region)

# XOR checksum of tx_data[16]
tx_data = [0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
           0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10]
checksum = 0
for b in tx_data:
    checksum ^= b

expected_v = checksum * 5.0 / 255.0
print(f"DAC: avg={avg_dac:.4f}V expected={expected_v:.4f}V "
      f"(checksum=0x{checksum:02X}) "
      f"stable={stable:.4f}V")

if abs(avg_dac - expected_v) < 0.3:
    print(f"PASS: DAC matches XOR checksum")
    sys.exit(0)
else:
    print(f"FAIL: DAC mismatch")
    sys.exit(1)
