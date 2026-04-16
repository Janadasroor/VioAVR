import json
import os

with open('ATmega2560_metadata.json', 'r') as f:
    data = json.load(f)

periphs = data.get('peripherals', {})
port_map = {}
for p_name, p_data in periphs.items():
    if p_data.get('module') == 'PORT':
        for r_name, r_data in p_data.get('registers', {}).items():
            if (r_name.startswith('PORT') and len(r_name) >= 5) or \
               (r_name.startswith('PIN') and len(r_name) >= 4):
                char = r_name[4] if r_name.startswith('PORT') else r_name[3]
                if char not in port_map: port_map[char] = {}
                reg_type = 'PORT' if r_name.startswith('PORT') else 'PIN'
                port_map[char][reg_type] = r_data['offset']
                print(f"Mapped {r_name} ({r_data['offset']}) to {char}[{reg_type}]")

print("-" * 20)
print(f"Port D: {port_map.get('D')}")
