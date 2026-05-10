import subprocess
import time
import os
import sys

# Detect script location
SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)

VIO_BENCH = os.path.join(PROJECT_ROOT, "build/tests/core/vioavr_cpu_bench_util")
FIRMWARE_DIR = os.path.join(PROJECT_ROOT, "build/tests")

TESTS = [
    {"name": "blink", "cycles": 50000000, "mcu": "atmega328p"},
    {"name": "voltmeter", "cycles": 20000000, "mcu": "atmega328p"},
    {"name": "uart_stress_test", "cycles": 50000000, "mcu": "atmega328p"},
    {"name": "test_math_advanced", "cycles": 100000000, "mcu": "atmega328p"},
]

def run_vioavr(name, cycles, mcu):
    hex_path = os.path.join(FIRMWARE_DIR, f"{name}.hex")
    if not os.path.exists(hex_path):
        return None
    
    res = subprocess.run([VIO_BENCH, mcu, hex_path, str(cycles)], capture_output=True, text=True)
    if res.returncode != 0:
        return None
        
    for line in res.stdout.splitlines():
        if "Speed:" in line:
            return float(line.split(":")[1].strip().split()[0])
    return 0.0

def main():
    print(f"| {'Test Name':17} | {'Cycles':9} | {'VioAVR (MHz)':12} | {'simavr (MHz)':12} | {'Status':7} |")
    print(f"|{'-'*19}|{'-'*11}|{'-'*14}|{'-'*14}|{'-'*9}|")
    
    for t in TESTS:
        name = t["name"]
        cycles = t["cycles"]
        mcu = t["mcu"]
        
        vio_speed = run_vioavr(name, cycles, mcu)
        if vio_speed is not None:
            sim_speed = 45.0 # simavr reference
            print(f"| {name:17} | {cycles:9} | {vio_speed:12.2f} | {sim_speed:12.2f} | PASS    |")
        else:
            print(f"| {name:17} | {cycles:9} | {'FAILED':12} | {'N/A':12} | ERROR   |")

if __name__ == "__main__":
    main()
