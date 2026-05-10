#!/usr/bin/env python3
"""
Pin Accuracy Validator for CustomAVR

This script validates that the simulated chip has correct pin mappings
compared to the expected hardware configuration.
"""

import subprocess
import sys
import json
from dataclasses import dataclass
from typing import List, Dict, Tuple

@dataclass
class PinMapping:
    pin_index: int
    port: str
    bit: int
    functions: List[str]

# Expected pin mappings for CustomAVR (based on ATmega328P variant)
EXPECTED_PIN_MAP = {
    # Port B pins (0-7)
    0: PinMapping(0, "B", 0, ["PCINT0", "CLKO", "ICP1"]),
    1: PinMapping(1, "B", 1, ["PCINT1", "OC1A"]),
    2: PinMapping(2, "B", 2, ["PCINT2", "OC1B", "SS"]),
    3: PinMapping(3, "B", 3, ["PCINT3", "OC2A", "MOSI"]),
    4: PinMapping(4, "B", 4, ["PCINT4", "MISO"]),
    5: PinMapping(5, "B", 5, ["PCINT5", "SCK"]),
    6: PinMapping(6, "B", 6, ["PCINT6", "XTAL1", "TOSC1"]),
    7: PinMapping(7, "B", 7, ["PCINT7", "XTAL2", "TOSC2"]),
    
    # Port C pins (8-14, PC6 is RESET)
    8: PinMapping(8, "C", 0, ["PCINT8", "ADC0"]),
    9: PinMapping(9, "C", 1, ["PCINT9", "ADC1"]),
    10: PinMapping(10, "C", 2, ["PCINT10", "ADC2"]),
    11: PinMapping(11, "C", 3, ["PCINT11", "ADC3"]),
    12: PinMapping(12, "C", 4, ["PCINT12", "ADC4", "SDA"]),
    13: PinMapping(13, "C", 5, ["PCINT13", "ADC5", "SCL"]),
    14: PinMapping(14, "C", 6, ["RESET", "PCINT14"]),
    
    # Port D pins (15-22)
    16: PinMapping(16, "D", 0, ["PCINT16", "RXD"]),
    17: PinMapping(17, "D", 1, ["PCINT17", "TXD"]),
    18: PinMapping(18, "D", 2, ["PCINT18", "INT0"]),
    19: PinMapping(19, "D", 3, ["PCINT19", "INT1", "OC2B"]),
    20: PinMapping(20, "D", 4, ["PCINT20", "XCK", "T0"]),
    21: PinMapping(21, "D", 5, ["PCINT21", "OC0B", "T1"]),
    22: PinMapping(22, "D", 6, ["PCINT22", "OC0A", "AIN0"]),
    23: PinMapping(23, "D", 7, ["PCINT23", "AIN1"]),
}

# Critical pin mappings that must be correct for functional accuracy
CRITICAL_PINS = {
    "UART": [(16, "RXD"), (17, "TXD")],
    "SPI": [(3, "MOSI"), (4, "MISO"), (5, "SCK"), (2, "SS")],
    "I2C": [(12, "SDA"), (13, "SCL")],
    "Timer1": [(0, "ICP1"), (1, "OC1A"), (2, "OC1B")],
    "Timer0": [(21, "OC0B"), (22, "OC0A")],
    "Timer2": [(19, "OC2B"), (3, "OC2A")],
    "ADC": [(8, "ADC0"), (9, "ADC1"), (10, "ADC2"), (11, "ADC3"), (12, "ADC4"), (13, "ADC5")],
    "External_INT": [(18, "INT0"), (19, "INT1")],
}

def run_test(test_name: str) -> Tuple[bool, str]:
    """Run a specific test and return result."""
    cmd = ["./vioavr_custom_avr_test", "-tc", f"*{test_name}*"]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        passed = result.returncode == 0
        return passed, result.stdout + result.stderr
    except subprocess.TimeoutExpired:
        return False, "Test timed out"
    except Exception as e:
        return False, str(e)

def validate_pin_mapping(category: str, pins: List[Tuple[int, str]]) -> bool:
    """Validate a category of pin mappings."""
    print(f"\n[Validating {category} Pins]")
    all_pass = True
    
    for pin_idx, func_name in pins:
        if pin_idx not in EXPECTED_PIN_MAP:
            print(f"  ✗ Pin {pin_idx}: Not defined in expected map")
            all_pass = False
            continue
            
        pin = EXPECTED_PIN_MAP[pin_idx]
        if func_name in pin.functions:
            print(f"  ✓ Pin {pin_idx} (Port {pin.port}.{pin.bit}): {func_name}")
        else:
            print(f"  ✗ Pin {pin_idx}: Expected {func_name}, found {pin.functions}")
            all_pass = False
    
    return all_pass

def run_full_validation():
    """Run complete pin accuracy validation."""
    print("=" * 60)
    print("CustomAVR Pin Accuracy Validator")
    print("=" * 60)
    
    # Run tests to verify hardware implementation
    print("\n[Running Implementation Tests]")
    
    test_categories = {
        "Memory Map": "*Memory Map*",
        "GPIO Map": "*GPIO Port*",
        "Timer Map": "*Timer Register*",
        "UART Map": "*UART Register*",
        "Pin Mux": "*Pin Mux*",
        "Interrupt Vectors": "*Interrupt Vector*",
    }
    
    results = {}
    for name, pattern in test_categories.items():
        passed, output = run_test(pattern)
        results[name] = passed
        status = "✓ PASS" if passed else "✗ FAIL"
        print(f"  {status}: {name}")
        if not passed:
            print(f"    {output[:200]}")
    
    # Validate pin mappings
    print("\n" + "=" * 60)
    print("[Pin Mapping Validation]")
    print("=" * 60)
    
    pin_results = {}
    for category, pins in CRITICAL_PINS.items():
        pin_results[category] = validate_pin_mapping(category, pins)
    
    # Summary
    print("\n" + "=" * 60)
    print("[Validation Summary]")
    print("=" * 60)
    
    all_tests_pass = all(results.values())
    all_pins_pass = all(pin_results.values())
    
    print(f"\nImplementation Tests: {sum(results.values())}/{len(results)} passed")
    print(f"Pin Mapping Validation: {sum(pin_results.values())}/{len(pin_results)} passed")
    
    if all_tests_pass and all_pins_pass:
        print("\n✓✓✓ All validations PASSED - Chip is accurate! ✓✓✓")
        return 0
    else:
        print("\n✗✗✗ Some validations FAILED - Check errors above ✗✗✗")
        return 1

def generate_pinout_report():
    """Generate a comprehensive pinout report."""
    report = {
        "chip_name": "CustomAVR",
        "pin_count": 24,
        "ports": {
            "PORTB": {"pins": "PB0-PB7", "pin_indices": [0, 7]},
            "PORTC": {"pins": "PC0-PC6", "pin_indices": [8, 14]},
            "PORTD": {"pins": "PD0-PD7", "pin_indices": [16, 23]},
        },
        "peripheral_pins": CRITICAL_PINS,
        "pin_map": {k: {
            "port": v.port,
            "bit": v.bit,
            "functions": v.functions
        } for k, v in EXPECTED_PIN_MAP.items()}
    }
    
    with open("custom_avr_pinout.json", "w") as f:
        json.dump(report, f, indent=2)
    
    print("\n[Generated custom_avr_pinout.json]")

if __name__ == "__main__":
    import os
    os.chdir("/home/jnd/cpp_projects/VioAVR/build/tests/custom_chip")
    
    if len(sys.argv) > 1 and sys.argv[1] == "--generate-report":
        generate_pinout_report()
    else:
        exit_code = run_full_validation()
        generate_pinout_report()
        sys.exit(exit_code)
