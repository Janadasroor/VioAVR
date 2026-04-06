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

    mismatches = 0
    
    # Check ADC
    adc_meta = meta['peripherals'].get('ADC', {})
    if adc_meta:
        print("\n[ADC]")
        regs = {'adcl_address': 'ADCL', 'adcsra_address': 'ADCSRA'}
        for field, reg_name in regs.items():
            reg_info = adc_meta['registers'].get(reg_name, {})
            expected = reg_info.get('offset', -1)
            actual = find_val(rf"\.{field}\s*=\s*([^,}}]+)")
            if actual is not None and expected != -1:
                if actual != expected:
                    print(f"  ❌ ADC {reg_name} Addr Mismatch: ATDF={hex(expected)}, Header={hex(actual)}")
                    mismatches += 1
                else: print(f"  ✅ ADC {reg_name} Addr OK")

    # Check UART0
    u0_meta = meta['peripherals'].get('USART0', meta['peripherals'].get('USART', {}))
    if u0_meta:
        print("\n[UART0]")
        regs = {'udr_address': 'UDR0', 'ucsra_address': 'UCSR0A'}
        for field, reg_name in regs.items():
            reg_info = u0_meta['registers'].get(reg_name, {})
            expected = reg_info.get('offset', -1)
            # Use escaped braces for f-string
            actual = find_val(rf"\.uart0\s*=\s*\{{[^}}]*\.{field}\s*=\s*(0x[0-9a-fA-F]+)")
            if actual is not None and expected != -1:
                if actual != expected:
                    print(f"  ❌ UART0 {reg_name} Addr Mismatch: ATDF={hex(expected)}, Header={hex(actual)}")
                    mismatches += 1
                else: print(f"  ✅ UART0 {reg_name} Addr OK")

    # Check SPI
    spi_meta = meta['peripherals'].get('SPI', {})
    if spi_meta:
        print("\n[SPI]")
        reg_info = spi_meta['registers'].get('SPCR', {})
        expected = reg_info.get('offset', -1)
        actual = find_val(rf"\.spi\s*=\s*\{{[^}}]*\.spcr_address\s*=\s*(0x[0-9a-fA-F]+)")
        if actual is not None and expected != -1:
            if actual != expected:
                print(f"  ❌ SPI SPCR Addr Mismatch: ATDF={hex(expected)}, Header={hex(actual)}")
                mismatches += 1
            else: print(f"  ✅ SPI SPCR Addr OK")

    if mismatches == 0:
        print("\n✅ Register map matches ATDF metadata.")
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
