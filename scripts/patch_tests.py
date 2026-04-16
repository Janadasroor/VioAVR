#!/usr/bin/env python3
import os
import re

def patch_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    # Pass 1: Handle old property names (e.g. .adc -> .adcs[0])
    replacements = {
        r'\.adc\b': '.adcs[0]',
        r'\.ac\b': '.acs[0]',
        r'\.spi\b': '.spis[0]',
        r'\.twi\b': '.twis[0]',
        r'\.eeproms\[0\]\b': '.eeproms[0]', # No change needed but good to have
        r'\.eeprom\b': '.eeproms[0]',
        r'\.wdt\b': '.wdts[0]',
        r'\.uart0\b': '.uarts[0]',
        r'\.uart\b': '.uarts[0]',
        r'\.timer0\b': '.timers8[0]',
        r'\.timer1\b': '.timers16[0]',
        r'\.timer2\b': '.timers8[1]',
        r'\.ext_interrupt\b': '.ext_interrupts[0]',
        r'\.pcint([0-9])\b': r'.pcints[\1]',
        r'\.pcint\b': '.pcints[0]',
        r'\.pin_change_interrupt_([0-9])\b': r'.pcints[\1]',
        r'\.int([0-9])_vector_index\b': r'.vector_indices[\1]',
    }

    # Pass 1.5: Fix Includes
    includes = {
        r'#include "vioavr/core/uart0.hpp"': '#include "vioavr/core/uart.hpp"',
        r'#include "vioavr/core/timer0.hpp"': '#include "vioavr/core/timer8.hpp"',
        r'#include "vioavr/core/timer2.hpp"': '#include "vioavr/core/timer8.hpp"',
        r'#include "vioavr/core/ext_interrupts.hpp"': '#include "vioavr/core/ext_interrupt.hpp"',
        r'#include "vioavr/core/pin_change_interrupts.hpp"': '#include "vioavr/core/pin_change_interrupt.hpp"',
    }

    modified = False
    new_content = content
    for pattern, replacement in replacements.items():
        new_content = re.sub(pattern, replacement, new_content)
    
    # Pass 1.7: Type Renames
    types = {
        r'\bUart[0-9]\b': 'Uart',
        r'\bExtInterrupts\b': 'ExtInterrupt',
        r'\bPinChangeInterrupts\b': 'PinChangeInterrupt',
    }

    for pattern, replacement in includes.items():
        new_content = re.sub(pattern, replacement, new_content)
    
    for pattern, replacement in types.items():
        new_content = re.sub(pattern, replacement, new_content)
    
    if new_content != content:
        content = new_content
        modified = True

    # Pass 2: Constructor patching (e.g. Timer8 t{"T", device} -> Timer8 t{"T", device.timers8[0]})
    # This is for when the device itself was passed instead of a descriptor.
    patterns = {
        'Timer8': ('timers8', '0'),
        'Timer16': ('timers16', '0'),
        'Spi': ('spis', '0'),
        'Twi': ('twis', '0'),
        'Adc': ('adcs', '0'),
        'AnalogComparator': ('acs', '0'),
        'Uart': ('uarts', '0'),
        'Wdt': ('wdts', '0'),
        'Eeprom': ('eeproms', '0'),
    }

    for p_type, (p_field, default_idx) in patterns.items():
        # Matches: Type var {name, device ...
        # Captured groups: 1=Type, 2=Var, 3=Name, 4=Device, 5=Suffix (comma or brace)
        regex = rf'\b({p_type})\b\s+([a-zA-Z0-9_]+)\s*\{{([^,]+),\s*([a-zA-Z0-9_]+)(\s*[,}}])'
        
        def replace_fn(match):
            t, v, name, device, suffix = match.groups()
            
            # Avoid double patching property access
            if device in ['atmega328', 'atmega8', 'device', 'atmega2560']:
                idx = default_idx
                if p_type == "Timer8" and "TIMER2" in name: idx = "1"
                return f"{t} {v} {{{name}, {device}.{p_field}[{idx}]{suffix}"
            return match.group(0)

        new_content = re.sub(regex, replace_fn, content)
        if new_content != content:
            content = new_content
            modified = True

    if modified:
        with open(filepath, 'w') as f:
            f.write(content)
        print(f"Patched {filepath}")

def main():
    test_dir = 'tests/core'
    for root, dirs, files in os.walk(test_dir):
        for file in files:
            if file.endswith('.cpp'):
                patch_file(os.path.join(root, file))

if __name__ == "__main__":
    main()
