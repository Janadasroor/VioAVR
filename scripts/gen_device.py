#!/usr/bin/env python3
import os, sys, re, xml.etree.ElementTree as ET
from pathlib import Path

def parse_atdf(file_path):
    device_name = file_path.stem
    tree = ET.parse(file_path)
    root = tree.getroot()
    device_info = {'name': device_name, 'flash_words': 0, 'sram_bytes': 0, 'eeprom_bytes': 0, 'vectors': 0, 'vector_size': 2, 'adc': {}, 'timer0': {}, 'timer2': {}, 'timer1': {}, 'uart0': {}, 'ext_interrupt': {}, 'pin_change_interrupt_0': {}, 'pin_change_interrupt_1': {}, 'pin_change_interrupt_2': {}, 'spi': {}, 'twi': {}, 'eeprom_io': {}, 'wdt': {}, 'ports': []}
    
    for mem in root.findall(".//memory-segment"):
        m, s = mem.attrib.get('type'), int(mem.attrib.get('size', '0x0'), 16)
        name = mem.attrib.get('name', '')
        external = mem.attrib.get('external', 'false')
        if m == 'flash' and name == 'FLASH': 
            device_info['flash_words'] = s // 2
        elif m == 'ram' and external == 'false': 
            device_info['sram_bytes'] = s
        elif m == 'eeprom': device_info['eeprom_bytes'] = s
    
    v = root.findall(".//interrupt")
    if v: device_info['vectors'] = max(int(x.attrib.get('index', '0')) for x in v) + 1
    
    reg_map = {}
    for group in root.iter('register-group'):
        g_off = int(group.attrib.get('offset', '0x0'), 16)
        bias = 0x20 if group.attrib.get('address-space') == 'io' else 0
        for reg in group.findall('register'):
            name = reg.attrib['name']
            r_off = int(reg.attrib['offset'], 16)
            size = int(reg.attrib.get('size', '1'))
            addr = bias + g_off + r_off
            reg_map[name] = addr
            if size == 2:
                if name + 'L' not in reg_map: reg_map[name + 'L'] = addr
                if name + 'H' not in reg_map: reg_map[name + 'H'] = addr + 1

    for reg in root.iter('memory-mapped-register'):
        name = reg.attrib['name']
        addr = int(reg.attrib.get('offset', '0x0'), 16)
        size = int(reg.attrib.get('size', '1'))
        reg_map[name] = addr
        if size == 2:
            if name + 'L' not in reg_map: reg_map[name + 'L'] = addr
            if name + 'H' not in reg_map: reg_map[name + 'H'] = addr + 1

    def fr(r):
        p = re.compile(r, re.IGNORECASE)
        for n, a in reg_map.items():
            if p.fullmatch(n): return a
        return 0
    def fv(r):
        p = re.compile(r, re.IGNORECASE)
        for i in root.findall(".//interrupt"):
            if p.search(i.attrib.get('name', '')): return int(i.attrib.get('index', '0'))
        return 0

    device_info['adc'] = { 'adcl': fr(r'ADCL'), 'adch': fr(r'ADCH'), 'adcsra': fr(r'ADCSRA|ADCSR'), 'adcsrb': fr(r'ADCSRB'), 'admux': fr(r'ADMUX'), 'vector': fv(r'ADC') }
    device_info['timer0'] = { 'tcnt': fr(r'TCNT0'), 'ocra': fr(r'OCR0A|OCR0'), 'ocrb': fr(r'OCR0B'), 'tccra': fr(r'TCCR0A|TCCR0'), 'tccrb': fr(r'TCCR0B'), 'assr': 0, 'timsk': fr(r'TIMSK0|TIMSK'), 'tifr': fr(r'TIFR0|TIFR'), 'v_ovf': fv(r'TIMER0_OVF'), 'v_compa': fv(r'TIMER0_COMPA?'), 'v_compb': fv(r'TIMER0_COMPB') }
    device_info['timer2'] = { 'tcnt': fr(r'TCNT2'), 'ocra': fr(r'OCR2A|OCR2'), 'ocrb': fr(r'OCR2B'), 'tccra': fr(r'TCCR2A|TCCR2'), 'tccrb': fr(r'TCCR2B'), 'assr': fr(r'ASSR'), 'timsk': fr(r'TIMSK2'), 'tifr': fr(r'TIFR2'), 'v_ovf': fv(r'TIMER2_OVF'), 'v_compa': fv(r'TIMER2_COMPA?'), 'v_compb': fv(r'TIMER2_COMPB') }
    device_info['timer1'] = { 'tcnt': fr(r'TCNT1|TCNT1L'), 'ocra': fr(r'OCR1A|OCR1AL'), 'ocrb': fr(r'OCR1B|OCR1BL'), 'icr': fr(r'ICR1|ICR1L'), 'tccra': fr(r'TCCR1A'), 'tccrb': fr(r'TCCR1B'), 'tccrc': fr(r'TCCR1C'), 'timsk': fr(r'TIMSK1|TIMSK'), 'tifr': fr(r'TIFR1|TIFR'), 'v_ovf': fv(r'TIMER1_OVF'), 'v_compa': fv(r'TIMER1_COMPA?'), 'v_compb': fv(r'TIMER1_COMPB'), 'v_capt': fv(r'TIMER1_CAPT') }
    device_info['uart0'] = { 'udr': fr(r'UDR0|UDR'), 'ucsra': fr(r'UCSR0A|UCSRA'), 'ucsrb': fr(r'UCSR0B|UCSRB'), 'ucsrc': fr(r'UCSR0C|UCSRC'), 'v_rx': fv(r'USART0?_RX'), 'v_tx': fv(r'USART0?_TX'), 'v_udre': fv(r'USART0?_UDRE') }
    device_info['spi'] = { 'spcr': fr(r'SPCR'), 'spsr': fr(r'SPSR'), 'spdr': fr(r'SPDR'), 'vector': fv(r'SPI_STC') }
    device_info['twi'] = { 'twbr': fr(r'TWBR'), 'twsr': fr(r'TWSR'), 'twar': fr(r'TWAR'), 'twdr': fr(r'TWDR'), 'twcr': fr(r'TWCR'), 'twamr': fr(r'TWAMR'), 'vector': fv(r'TWI') }
    device_info['eeprom_io'] = { 'eecr': fr(r'EECR'), 'eedr': fr(r'EEDR'), 'eearl': fr(r'EEAR|EEARL'), 'eearh': fr(r'EEARH'), 'vector': fv(r'EE_READY') }
    device_info['wdt'] = { 'wdtcsr': fr(r'WDTCSR|WDTCR'), 'vector': fv(r'WDT') }
    device_info['ext_interrupt'] = { 'eicra': fr(r'EICRA'), 'eimsk': fr(r'EIMSK|GIMSK|GICR'), 'eifr': fr(r'EIFR|GIFR'), 'vector': fv(r'INT0') }
    device_info['pin_change_interrupt_0'] = { 'pcicr': fr(r'PCICR'), 'pcifr': fr(r'PCIFR'), 'pcmsk': fr(r'PCMSK0'), 'enable_mask': 0x01, 'flag_mask': 0x01, 'vector': fv(r'PCINT0') }
    device_info['pin_change_interrupt_1'] = { 'pcicr': fr(r'PCICR'), 'pcifr': fr(r'PCIFR'), 'pcmsk': fr(r'PCMSK1'), 'enable_mask': 0x02, 'flag_mask': 0x02, 'vector': fv(r'PCINT1') }
    device_info['pin_change_interrupt_2'] = { 'pcicr': fr(r'PCICR'), 'pcifr': fr(r'PCIFR'), 'pcmsk': fr(r'PCMSK2'), 'enable_mask': 0x04, 'flag_mask': 0x04, 'vector': fv(r'PCINT2') }
    
    for p in "ABCDEFGH":
        p_info = {'name': f"PORT{p}", 'pin': fr(f"PIN{p}"), 'ddr': fr(f"DDR{p}"), 'port': fr(f"PORT{p}")}
        if p_info['pin'] or p_info['ddr'] or p_info['port']: device_info['ports'].append(p_info)
    return device_info

def generate_header(d, output_dir):
    name = d['name'].lower()
    header_path = output_dir / f"{name}.hpp"
    def hx(v): return f"{hex(v).upper().replace('X', 'x')}U"
    port_entries = []
    for i in range(8):
        if i < len(d['ports']):
            p = d['ports'][i]
            port_entries.append(f'{{ "{p["name"]}", {hx(p["pin"])}, {hx(p["ddr"])}, {hx(p["port"])} }}')
        else: port_entries.append('{ "", 0x00U, 0x00U, 0x00U }')
    ports_str = ",\n        ".join(port_entries)
    content = f"""#pragma once
#include "vioavr/core/device.hpp"
namespace vioavr::core::devices {{
inline constexpr DeviceDescriptor {name} {{
    .name = "{d['name']}",
    .flash_words = {d['flash_words']}U, .sram_bytes = {d['sram_bytes']}U, .eeprom_bytes = {d['eeprom_bytes']}U,
    .interrupt_vector_count = {d['vectors']}U, .interrupt_vector_size = {d['vector_size']}U,
    .adc = {{ .adcl_address = {hx(d['adc']['adcl'])}, .adch_address = {hx(d['adc']['adch'])}, .adcsra_address = {hx(d['adc']['adcsra'])}, .adcsrb_address = {hx(d['adc']['adcsrb'])}, .admux_address = {hx(d['adc']['admux'])}, .vector_index = {d['adc']['vector']}U }},
    .timer0 = {{ .tcnt_address = {hx(d['timer0']['tcnt'])}, .ocra_address = {hx(d['timer0']['ocra'])}, .ocrb_address = {hx(d['timer0']['ocrb'])}, .tifr_address = {hx(d['timer0']['tifr'])}, .timsk_address = {hx(d['timer0']['timsk'])}, .tccra_address = {hx(d['timer0']['tccra'])}, .tccrb_address = {hx(d['timer0']['tccrb'])}, .assr_address = {hx(d['timer0']['assr'])}, .compare_a_vector_index = {d['timer0']['v_compa']}U, .compare_b_vector_index = {d['timer0']['v_compb']}U, .overflow_vector_index = {d['timer0']['v_ovf']}U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U }},
    .timer2 = {{ .tcnt_address = {hx(d['timer2']['tcnt'])}, .ocra_address = {hx(d['timer2']['ocra'])}, .ocrb_address = {hx(d['timer2']['ocrb'])}, .tifr_address = {hx(d['timer2']['tifr'])}, .timsk_address = {hx(d['timer2']['timsk'])}, .tccra_address = {hx(d['timer2']['tccra'])}, .tccrb_address = {hx(d['timer2']['tccrb'])}, .assr_address = {hx(d['timer2']['assr'])}, .compare_a_vector_index = {d['timer2']['v_compa']}U, .compare_b_vector_index = {d['timer2']['v_compb']}U, .overflow_vector_index = {d['timer2']['v_ovf']}U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U }},
    .timer1 = {{ .tcnt_address = {hx(d['timer1']['tcnt'])}, .ocra_address = {hx(d['timer1']['ocra'])}, .ocrb_address = {hx(d['timer1']['ocrb'])}, .icr_address = {hx(d['timer1']['icr'])}, .tifr_address = {hx(d['timer1']['tifr'])}, .timsk_address = {hx(d['timer1']['timsk'])}, .tccra_address = {hx(d['timer1']['tccra'])}, .tccrb_address = {hx(d['timer1']['tccrb'])}, .tccrc_address = {hx(d['timer1']['tccrc'])}, .capture_vector_index = {d['timer1']['v_capt']}U, .compare_a_vector_index = {d['timer1']['v_compa']}U, .compare_b_vector_index = {d['timer1']['v_compb']}U, .overflow_vector_index = {d['timer1']['v_ovf']}U, .capture_enable_mask = 0x20U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U }},
    .ext_interrupt = {{ .eicra_address = {hx(d['ext_interrupt']['eicra'])}, .eimsk_address = {hx(d['ext_interrupt']['eimsk'])}, .eifr_address = {hx(d['ext_interrupt']['eifr'])}, .int0_vector_index = {d['ext_interrupt']['vector']}U }},
    .uart0 = {{ .udr_address = {hx(d['uart0']['udr'])}, .ucsra_address = {hx(d['uart0']['ucsra'])}, .ucsrb_address = {hx(d['uart0']['ucsrb'])}, .ucsrc_address = {hx(d['uart0']['ucsrc'])}, .rx_vector_index = {d['uart0']['v_rx']}U, .udre_vector_index = {d['uart0']['v_udre']}U, .tx_vector_index = {d['uart0']['v_tx']}U }},
    .pin_change_interrupt_0 = {{ .pcicr_address = {hx(d['pin_change_interrupt_0']['pcicr'])}, .pcifr_address = {hx(d['pin_change_interrupt_0']['pcifr'])}, .pcmsk_address = {hx(d['pin_change_interrupt_0']['pcmsk'])}, .pcicr_enable_mask = {hx(d['pin_change_interrupt_0']['enable_mask'])}, .pcifr_flag_mask = {hx(d['pin_change_interrupt_0']['flag_mask'])}, .vector_index = {d['pin_change_interrupt_0']['vector']}U }},
    .pin_change_interrupt_1 = {{ .pcicr_address = {hx(d['pin_change_interrupt_1']['pcicr'])}, .pcifr_address = {hx(d['pin_change_interrupt_1']['pcifr'])}, .pcmsk_address = {hx(d['pin_change_interrupt_1']['pcmsk'])}, .pcicr_enable_mask = {hx(d['pin_change_interrupt_1']['enable_mask'])}, .pcifr_flag_mask = {hx(d['pin_change_interrupt_1']['flag_mask'])}, .vector_index = {d['pin_change_interrupt_1']['vector']}U }},
    .pin_change_interrupt_2 = {{ .pcicr_address = {hx(d['pin_change_interrupt_2']['pcicr'])}, .pcifr_address = {hx(d['pin_change_interrupt_2']['pcifr'])}, .pcmsk_address = {hx(d['pin_change_interrupt_2']['pcmsk'])}, .pcicr_enable_mask = {hx(d['pin_change_interrupt_2']['enable_mask'])}, .pcifr_flag_mask = {hx(d['pin_change_interrupt_2']['flag_mask'])}, .vector_index = {d['pin_change_interrupt_2']['vector']}U }},
    .spi = {{ .spcr_address = {hx(d['spi']['spcr'])}, .spsr_address = {hx(d['spi']['spsr'])}, .spdr_address = {hx(d['spi']['spdr'])}, .vector_index = {d['spi']['vector']}U }},
    .twi = {{ .twbr_address = {hx(d['twi']['twbr'])}, .twsr_address = {hx(d['twi']['twsr'])}, .twar_address = {hx(d['twi']['twar'])}, .twdr_address = {hx(d['twi']['twdr'])}, .twcr_address = {hx(d['twi']['twcr'])}, .twamr_address = {hx(d['twi']['twamr'])}, .vector_index = {d['twi']['vector']}U }},
    .eeprom = {{ .eecr_address = {hx(d['eeprom_io']['eecr'])}, .eedr_address = {hx(d['eeprom_io']['eedr'])}, .eearl_address = {hx(d['eeprom_io']['eearl'])}, .eearh_address = {hx(d['eeprom_io']['eearh'])}, .vector_index = {d['eeprom_io']['vector']}U }},
    .wdt = {{ .wdtcsr_address = {hx(d['wdt']['wdtcsr'])}, .vector_index = {d['wdt']['vector']}U }},
    .ports = {{{{
        {ports_str}
    }}}},
}};
}} // namespace vioavr::core::devices
"""
    with open(header_path, 'w') as f: f.write(content)

def main():
    ad, od = Path("atmega/atdf"), Path("include/vioavr/core/devices")
    if not ad.exists(): sys.exit(1)
    od.mkdir(parents=True, exist_ok=True)
    files = list(ad.glob("*.atdf"))
    for f in files:
        try: generate_header(parse_atdf(f), od)
        except Exception as e: print(f"FAIL {f.name}: {e}")

if __name__ == "__main__": main()
