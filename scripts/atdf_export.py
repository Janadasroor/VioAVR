#!/usr/bin/env python3
import os, sys, json, xml.etree.ElementTree as ET
from pathlib import Path
import argparse

def parse_atdf(file_path):
    tree = ET.parse(file_path)
    root = tree.getroot()
    
    # 1. Build module definitions (from <modules> section)
    # This section contains the actual register and bitfield definitions.
    module_defs = {}
    
    # First, collect all value-groups for enums
    value_groups = {}
    for vg in root.findall(".//value-group"):
        vg_name = vg.attrib.get('name')
        value_groups[vg_name] = {}
        for val in vg.findall("value"):
            value_groups[vg_name][val.attrib.get('name')] = {
                'value': int(val.attrib.get('value', '0'), 16),
                'caption': val.attrib.get('caption')
            }

    modules_sect = root.find("modules")
    if modules_sect is not None:
        for module in modules_sect.findall("module"):
            mod_name = module.attrib.get('name')
            module_defs[mod_name] = {
                'caption': module.attrib.get('caption'),
                'register_groups': {}
            }
            for reg_group in module.findall("register-group"):
                rg_name = reg_group.attrib.get('name')
                rg_data = {
                    'caption': reg_group.attrib.get('caption'),
                    'registers': {}
                }
                for reg in reg_group.findall("register"):
                    reg_name = reg.attrib.get('name')
                    r_data = {
                        'caption': reg.attrib.get('caption'),
                        'offset': int(reg.attrib.get('offset', '0'), 16),
                        'size': int(reg.attrib.get('size', '1')),
                        'initval': int(reg.attrib.get('initval', '0'), 16),
                        'mask': int(reg.attrib.get('mask', '0xFF'), 16),
                        'bitfields': {}
                    }
                    for bf in reg.findall("bitfield"):
                        bf_name = bf.attrib.get('name')
                        bf_data = {
                            'mask': int(bf.attrib.get('mask', '0'), 16),
                            'caption': bf.attrib.get('caption'),
                            'values': {}
                        }
                        # Link to value-group if it exists
                        vg_ref = bf.attrib.get('values')
                        if vg_ref and vg_ref in value_groups:
                            bf_data['values'] = value_groups[vg_ref]
                        
                        r_data['bitfields'][bf_name] = bf_data
                    rg_data['registers'][reg_name] = r_data
                module_defs[mod_name]['register_groups'][rg_name] = rg_data

    # 2. Extract Device Info
    device = root.find(".//device")
    if device is None:
        # Fallback for some ATDF formats
        device = root
    device_name = device.attrib.get('name')
    
    data = {
        'name': device_name,
        'memories': {},
        'interrupts': [],
        'peripherals': {},
        'configuration': {
            'fuses': {},
            'lockbits': {}
        }
    }
    
    # 3. Extract Memories
    for space in root.findall(".//address-space"):
        space_name = space.attrib.get('name')
        data['memories'][space_name] = []
        for segment in space.findall("memory-segment"):
            data['memories'][space_name].append({
                'name': segment.attrib.get('name'),
                'start': int(segment.attrib.get('start', '0'), 16),
                'size': int(segment.attrib.get('size', '0'), 16),
                'type': segment.attrib.get('type'),
                'pagesize': int(segment.attrib.get('pagesize', '0'), 16)
            })

    # 4. Extract Interrupts
    for intr in root.findall(".//interrupt"):
        data['interrupts'].append({
            'name': intr.attrib.get('name'),
            'index': int(intr.attrib.get('index', '0')),
            'caption': intr.attrib.get('caption')
        })
    data['interrupts'].sort(key=lambda x: x['index'])

    # 5. Extract Peripherals (from <peripherals> section)
    periphs_sect = root.find(".//peripherals")
    if periphs_sect is not None:
        for module in periphs_sect.findall("module"):
            mod_name = module.attrib.get('name')
            for instance in module.findall("instance"):
                inst_name = instance.attrib.get('name')
                periph_data = {
                    'module': mod_name,
                    'registers': {},
                    'signals': []
                }
                
                # Signals
                for sig in instance.findall(".//signal"):
                    periph_data['signals'].append({
                        'group': sig.attrib.get('group'),
                        'pad': sig.attrib.get('pad'),
                        'index': sig.attrib.get('index'),
                        'function': sig.attrib.get('function')
                    })
                
                # Link registers from module definitions
                for rg_ref in instance.findall("register-group"):
                    rg_name = rg_ref.attrib.get('name-in-module') or rg_ref.attrib.get('name')
                    rg_offset = int(rg_ref.attrib.get('offset', '0'), 16)
                    
                    if mod_name in module_defs and rg_name in module_defs[mod_name]['register_groups']:
                        base_rg = module_defs[mod_name]['register_groups'][rg_name]
                        for reg_name, reg_def in base_rg['registers'].items():
                            # Clone and adjust offset
                            import copy
                            r_data = copy.deepcopy(reg_def)
                            r_data['offset'] += rg_offset
                            periph_data['registers'][reg_name] = r_data
                
                data['peripherals'][inst_name] = periph_data

    # 6. Extract Configuration (Fuses, Lockbits)
    for mod_name in ['FUSE', 'LOCKBIT']:
        if mod_name in module_defs:
            target_key = 'fuses' if mod_name == 'FUSE' else 'lockbits'
            for rg_name, rg_data in module_defs[mod_name]['register_groups'].items():
                for reg_name, reg_def in rg_data['registers'].items():
                    data['configuration'][target_key][reg_name] = reg_def

    # 7. Extract Properties (Signatures, etc.)
    data['properties'] = {}
    for prop in root.findall(".//property"):
        name = prop.attrib.get('name')
        value = prop.attrib.get('value')
        if name and value:
            data['properties'][name] = value

    return data

def main():
    parser = argparse.ArgumentParser(description='Export ATDF metadata to JSON')
    parser.add_argument('--mcu', help='MCU name (e.g. ATmega328P)', required=True)
    parser.add_argument('--out', help='Output JSON file')
    args = parser.parse_args()

    atdf_path = Path(f"atmega/atdf/{args.mcu}.atdf")
    if not atdf_path.exists():
        print(f"Error: ATDF file not found for {args.mcu}")
        sys.exit(1)

    data = parse_atdf(atdf_path)
    
    out_path = args.out if args.out else f"{args.mcu}_metadata.json"
    with open(out_path, 'w') as f:
        json.dump(data, f, indent=2)
    print(f"Exported metadata to {out_path}")

if __name__ == "__main__":
    main()
