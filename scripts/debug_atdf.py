import sys, json, os
from pathlib import Path
from atdf_export import parse_atdf

dfp_dir = os.environ.get("ATMEGA_DFP_DIR", "avr-pack/atmega")
atdf_path = Path(dfp_dir) / "atdf" / "ATmega328P.atdf"
data = parse_atdf(atdf_path)

# Debug CPU registers
cpu = data['peripherals'].get('CPU', data['peripherals'].get('CPU_REGISTERS', {}))
print("CPU Registers:")
for r_name, r_data in cpu.get('registers', {}).items():
    if "SP" in r_name or "SREG" in r_name:
        print(f"  {r_name}: offset={hex(r_data['offset'])}, size={r_data['size']}")

# Debug ADC registers
adc = data['peripherals'].get('ADC', {})
print("\nADC Registers:")
for r_name, r_data in adc.get('registers', {}).items():
    print(f"  {r_name}: offset={hex(r_data['offset'])}, size={r_data['size']}")
