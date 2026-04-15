#!/usr/bin/env python3
import sys, json, argparse

def validate_metadata(data):
    errors = []
    warnings = []
    
    # 1. Basic Structure
    for key in ['name', 'memories', 'interrupts', 'peripherals', 'configuration']:
        if key not in data:
            errors.append(f"Missing top-level key: {key}")
            
    # 2. Memories
    mem = data.get('memories', {})
    has_ram = False
    for space_name, segments in mem.items():
        if any(m['type'] == 'ram' for m in segments):
            has_ram = True
            break
    if not has_ram:
        errors.append("No RAM found in any address space")
        
    has_flash = False
    for space_name, segments in mem.items():
        if any(m['type'] == 'flash' for m in segments):
            has_flash = True
            break
    if not has_flash:
        errors.append("No FLASH found in any address space")
        
    # 3. Interrupts
    if not data.get('interrupts'):
        errors.append("No interrupts found")
    else:
        # Check for continuous vector table indices (at least start from 0 or 1)
        indices = [i['index'] for i in data['interrupts']]
        if min(indices) > 2:
             warnings.append(f"Interrupt vector table starts at high index: {min(indices)}")
        
    # 4. Core Registers
    periphs = data.get('peripherals', {})
    found_cpu = False
    for p_name, p_data in periphs.items():
        if p_data.get('module') == 'CPU' or p_name == 'CPU':
            found_cpu = True
            regs = p_data.get('registers', {})
            if 'SREG' not in regs:
                errors.append(f"Missing SREG in {p_name} registers")
            # SP can be split or atomic
            has_sp = 'SP' in regs or 'SPL' in regs or 'SPH' in regs
            if not has_sp:
                errors.append(f"Missing Stack Pointer (SP, SPL, or SPH) in {p_name} registers")
            break
    if not found_cpu:
        errors.append("No CPU peripheral found (essential for core registers)")
        
    # 5. GPIO
    has_port = any('PORT' in name or 'GPIO' in name for name in periphs)
    if not has_port:
        warnings.append("No PORT/GPIO peripherals found (unusual for ATmega)")
        
    return errors, warnings

def main():
    parser = argparse.ArgumentParser(description='Validate ATDF metadata JSON')
    parser.add_argument('json_file', help='JSON file to validate')
    args = parser.parse_args()
    
    try:
        with open(args.json_file, 'r') as f:
            data = json.load(f)
    except Exception as e:
        print(f"Error reading JSON: {e}")
        sys.exit(1)
        
    errors, warnings = validate_metadata(data)
    
    if warnings:
        print(f"Warnings for {data.get('name', 'Unknown')}:")
        for wrn in warnings:
            print(f"  ? {wrn}")
            
    if errors:
        print(f"Validation failed for {data.get('name', 'Unknown')}:")
        for err in errors:
            print(f"  ! {err}")
        sys.exit(1)
    else:
        print(f"Validation passed for {data.get('name', 'Unknown')}")

if __name__ == "__main__":
    main()
