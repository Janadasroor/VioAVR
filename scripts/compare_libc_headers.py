#!/usr/bin/env python3
import re, sys
from pathlib import Path
import argparse

def parse_libc_header(header_path):
    with open(header_path, 'r') as f:
        content = f.read()
    
    regs = {}
    # Match #define NAME _SFR_IO8(0xXX) or _SFR_MEM8(0xXX) or _SFR_IO16...
    # We use a more generic pattern to catch the address
    pattern = re.compile(r'#define\s+(\w+)\s+_SFR_(IO|MEM)(8|16)\((0x[0-9a-fA-F]+)\)')
    
    for name, space, size, addr in pattern.findall(content):
        val = int(addr, 16)
        if space == 'IO':
            val += 0x20
        regs[name] = val
        
    # Handle includes (important for shared headers like iomxx4.h)
    include_pattern = re.compile(r'#include\s+"(avr/)?(io\w+\.h)"')
    for _, incl in include_pattern.findall(content):
        incl_path = header_path.parent / incl
        if incl_path.exists():
            regs.update(parse_libc_header(incl_path))

    return regs

def compare(mcu):
    # Mapping for common shared headers in avr-libc
    shared_maps = {
        'atmega324p': 'iomxx4.h',
        'atmega644p': 'iomxx4.h',
        'atmega1284p': 'iom1284p.h',
        'atmega328p': 'iom328p.h',
        'atmega168p': 'iom168p.h',
        'atmega2560': 'iom2560.h'
    }
    
    m_name = mcu.lower()
    libc_base = Path("avr-libc/include/avr")
    
    libc_file = libc_base / shared_maps.get(m_name, f"io{m_name.replace('atmega', 'm')}.h")
    
    if not libc_file.exists():
        libc_file = libc_base / f"io{m_name}.h"
        
    if not libc_file.exists():
        print(f"Error: avr-libc header not found for {mcu}")
        return

    vio_file = Path(f"include/vioavr/core/devices/{m_name}.hpp")
    if not vio_file.exists():
        print(f"Error: VioAVR header not found for {mcu}")
        return

    print(f"Comparing {mcu}:")
    print(f"  Libc: {libc_file.name}")
    print(f"  Vio:  {vio_file.name}")

    libc_regs = parse_libc_header(libc_file)
    with open(vio_file, 'r') as f:
        vio_content = f.read()

    mismatches = 0
    # More flexible regex for finding addresses in our descriptors
    def find_in_vio(field_regex):
        match = re.search(field_regex, vio_content)
        if match:
            return int(match.group(1), 16)
        return None

    to_check = [
        ('PORTB', r'\{\s*"PORTB",\s*0x[0-9a-fA-F]+U,\s*0x[0-9a-fA-F]+U,\s*(0x[0-9a-fA-F]+)U'),
        ('DDRB',  r'\{\s*"PORTB",\s*0x[0-9a-fA-F]+U,\s*(0x[0-9a-fA-F]+)U'),
        ('PINB',  r'\{\s*"PORTB",\s*(0x[0-9a-fA-F]+)U'),
        ('ADCSRA', r'\.adcsra_address\s*=\s*(0x[0-9a-fA-F]+)U'),
        ('ADMUX',  r'\.admux_address\s*=\s*(0x[0-9a-fA-F]+)U'),
        ('TCNT0',  r'\.timer0\s*=\s*\{[^}]*\.tcnt_address\s*=\s*(0x[0-9a-fA-F]+)U'),
        ('TCNT1',  r'\.timer1\s*=\s*\{[^}]*\.tcnt_address\s*=\s*(0x[0-9a-fA-F]+)U'),
        ('SREG',   r'\.sreg_address\s*=\s*(0x[0-9a-fA-F]+)U'),
        ('SMCR',   r'\.smcr_address\s*=\s*(0x[0-9a-fA-F]+)U'),
    ]

    for name, pattern in to_check:
        if name not in libc_regs: 
            # Some chips don't have all peripherals
            continue
        expected = libc_regs[name]
        actual = find_in_vio(pattern)
        
        if actual is not None:
            if actual != expected:
                print(f"  ❌ {name} Mismatch! Libc={hex(expected)}, Vio={hex(actual)}")
                mismatches += 1
            else:
                print(f"  ✅ {name} OK ({hex(actual)})")
        else:
            # Only warn if it's a critical core register
            if name in ['SREG', 'SPL', 'PORTB']:
                print(f"  ⚠️  {name} not found in VioAVR descriptor")

    if mismatches == 0:
        print("\n🏆 Gold Standard achieved for basic register map!")
    else:
        print(f"\n🛠 Found {mismatches} issues to fix.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--mcu', required=True)
    args = parser.parse_args()
    compare(args.mcu)
