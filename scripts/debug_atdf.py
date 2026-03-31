#!/usr/bin/env python3
import xml.etree.ElementTree as ET

def debug_atdf(file_path):
    tree = ET.parse(file_path)
    root = tree.getroot()
    
    for module in root.findall(".//module"):
        m_name = module.attrib.get('name')
        for instance in module.findall("instance"):
            i_name = instance.attrib.get('name')
            for rg in instance.findall("register-group"):
                print(f"Mod: {m_name}, Inst: {i_name}, RG: {rg.attrib.get('name')}, NIM: {rg.attrib.get('name-in-module')}, Off: {rg.attrib.get('offset')}")

if __name__ == "__main__":
    debug_atdf("atmega/atdf/ATmega128.atdf")
