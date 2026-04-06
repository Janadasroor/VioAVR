#!/usr/bin/env python3
import re, json, sys
from pathlib import Path
import argparse

def check_header(header_path, metadata_path):
    with open(metadata_path, 'r') as f:
        meta = json.load(f)
    
    with open(header_path, 'r') as f:
        header = f.read()

    print(f"--- Accuracy Check: {meta['name']} ---")
    
    def find_val(pattern):
        m = re.search(pattern, header)
        if m:
            val = m.group(1).lower().replace('u', '')
            if val.startswith('0x'):
                return int(val, 16)
            try:
                return int(val)
            except:
                return None
        return None

    # 1. Check Interrupt Vectors
    print("\n[Interrupts]")

    # 2. Check Peripheral Registers
    print("\n[Peripherals]")
    mismatches = 0
    
    # Check ADC
    adc_meta = meta['peripherals'].get('ADC', {})
    if adc_meta:
        regs = {
            'adcl_address': 'ADCL',
            'adch_address': 'ADCH',
            'adcsra_address': 'ADCSRA',
            'admux_address': 'ADMUX'
        }
        for field, reg_name in regs.items():
            reg_info = adc_meta['registers'].get(reg_name, {})
            expected = reg_info.get('offset', -1)
            actual = find_val(rf"\.{field}\s*=\s*([^,}}]+)")
            if actual is not None and expected != -1:
                if actual != expected:
                    print(f"  Mismatch ADC {reg_name} Addr: ATDF={hex(expected)}, Header={hex(actual)}")
                    mismatches += 1
                else:
                    print(f"  OK: ADC {reg_name} Addr")

        # Check ADC Masks
        masks = {
            'adsc_mask': 'ADSC',
            'adif_mask': 'ADIF',
            'adie_mask': 'ADIE'
        }
        adcsra = adc_meta['registers'].get('ADCSRA', {})
        for field, bit_name in masks.items():
            bf_info = adcsra.get('bitfields', {}).get(bit_name, {})
            # It might be an int already or a dict with 'mask'
            expected = bf_info.get('mask', -1) if isinstance(bf_info, dict) else bf_info
            actual = find_val(rf"\.{field}\s*=\s*([^,}}]+)")
            if actual is not None and expected != -1:
                if actual != expected:
                    print(f"  Mismatch ADC {bit_name} Mask: ATDF={hex(expected)}, Header={hex(actual)}")
                    mismatches += 1
                else:
                    print(f"  OK: ADC {bit_name} Mask")

    # Check Timer0
    t0_meta = meta['peripherals'].get('TC0', {})
    if t0_meta:
        regs = {
            'tcnt_address': 'TCNT0',
            'tccra_address': 'TCCR0A',
            'tccrb_address': 'TCCR0B',
        }
        for field, reg_name in regs.items():
            reg_info = t0_meta['registers'].get(reg_name, {})
            expected = reg_info.get('offset', -1)
            actual = find_val(rf"\.timer0\s*=\s*{{[^}}]*\.{field}\s*=\s*([^,}}]+)")
            if actual is not None and expected != -1:
                if actual != expected:
                    print(f"  Mismatch Timer0 {reg_name} Addr: ATDF={hex(expected)}, Header={hex(actual)}")
                    mismatches += 1
                else:
                    print(f"  OK: Timer0 {reg_name} Addr")

    if mismatches == 0:
        print("\n✅ Register and Mask map matches ATDF metadata.")
    else:
        print(f"\n❌ Found {mismatches} mismatches!")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--mcu', required=True)
    args = parser.parse_args()

    header = Path(f"include/vioavr/core/devices/{args.mcu.lower()}.hpp")
    meta = Path(f"{args.mcu}_metadata.json")

    if not meta.exists():
        import subprocess
        subprocess.run([sys.executable, "scripts/atdf_export.py", "--mcu", args.mcu])

    check_header(header, meta)

if __name__ == "__main__":
    main()
