#!/usr/bin/env python3
import json, sys
from pathlib import Path
import argparse

def generate_audit_md(metadata_path, mcu_name):
    with open(metadata_path, 'r') as f:
        data = json.load(f)

    md = f"# Hardware Accuracy Audit: {mcu_name}\n\n"
    md += "## 1. Memory Configuration\n\n"
    md += "| Segment | Start | Size | Type | Status |\n"
    md += "| --- | --- | --- | --- | --- |\n"
    
    # We look at 'data' and 'prog' spaces
    for space, segments in data['memories'].items():
        for seg in segments:
            md += f"| {seg['name']} | {hex(seg['start'])} | {hex(seg['size'])} | {seg['type']} | [ ] |\n"
    
    md += "\n## 2. Interrupt Vector Table\n\n"
    md += "| Index | Name | Caption | Status |\n"
    md += "| --- | --- | --- | --- |\n"
    for intr in data['interrupts']:
        md += f"| {intr['index']} | {intr['name']} | {intr['caption']} | [ ] |\n"

    md += "\n## 3. Peripheral Register Map\n\n"
    for inst_name, periph in data['peripherals'].items():
        if not periph['registers']: continue
        md += f"### {inst_name} ({periph['module']})\n\n"
        md += "| Register | Offset | Reset | Mask | Bits | Status |\n"
        md += "| --- | --- | --- | --- | --- | --- |\n"
        for reg_name, reg in periph['registers'].items():
            bits = ", ".join(reg['bitfields'].keys())
            md += f"| {reg_name} | {hex(reg['offset'])} | {hex(reg['initval'])} | {hex(reg['mask'])} | {bits} | [ ] |\n"
        md += "\n"

    return md

def main():
    parser = argparse.ArgumentParser(description='Generate Audit Markdown from Metadata')
    parser.add_argument('--mcu', help='MCU name', required=True)
    parser.add_argument('--meta', help='Metadata JSON file', required=True)
    args = parser.parse_args()

    md_content = generate_audit_md(args.meta, args.mcu)
    out_path = Path(f"docs/ACCURACY_{args.mcu}.md")
    with open(out_path, 'w') as f:
        f.write(md_content)
    print(f"Generated audit report template: {out_path}")

if __name__ == "__main__":
    main()
