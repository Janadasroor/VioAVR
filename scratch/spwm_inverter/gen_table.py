#!/usr/bin/env python3
"""Generate SPWM sine table for AVR firmware."""

import math

TABLE_LEN = 400
PWM_TOP = 799
MOD_INDEX = 0.8

vals = []
for i in range(TABLE_LEN):
    angle = 2.0 * math.pi * i / TABLE_LEN
    s = math.sin(angle)
    if s < 0:
        s = -s
    v = int(PWM_TOP * MOD_INDEX * s + 0.5)
    vals.append(v)

# Print as C array
print("// Auto-generated sine lookup table")
print(f"// {TABLE_LEN} entries, max={max(vals)}, min={min(vals)}")
print(f"#define SPWM_TABLE_LEN {TABLE_LEN}")
print(f"#define SPWM_PWM_TOP {PWM_TOP}")
print(f"#define SPWM_MOD_INDEX {MOD_INDEX}f")
print()
print("static const uint16_t sine_table[SPWM_TABLE_LEN] PROGMEM = {")
for i in range(0, TABLE_LEN, 10):
    chunk = ", ".join(f"{v:3d}" for v in vals[i:i+10])
    print(f"    {chunk},")
print("};")
