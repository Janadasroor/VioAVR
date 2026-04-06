#!/usr/bin/env python3
import os, sys, re, xml.etree.ElementTree as ET
from pathlib import Path

def parse_atdf(file_path):
    device_name = file_path.stem
    tree = ET.parse(file_path)
    root = tree.getroot()
    
    reg_map = {}
    
    device_info = {
        'name': device_name, 'flash_words': 0, 'sram_bytes': 0, 'eeprom_bytes': 0,
        'vectors': 0, 'vector_size': 4, 'adc': {}, 'timer0': {}, 'timer2': {},
        'timer1': {}, 'uart0': {}, 'ext_interrupt': {}, 
        'pin_change_interrupt_0': {}, 'pin_change_interrupt_1': {}, 'pin_change_interrupt_2': {}, 
        'spi': {}, 'twi': {}, 'eeprom_io': {}, 'wdt': {}, 'ports': [],
        'fuse_addr': 0, 'lockbit_addr': 0, 'sig_addr': 0,
        'flash_rww_end_word': 0, 'flash_page_size': 0
    }
    
    for mem in root.findall(".//memory-segment"):
        m, s = mem.attrib.get('type'), int(mem.attrib.get('size', '0x0'), 16)
        name = mem.attrib.get('name', '')
        external = mem.attrib.get('external', 'false')
        start = int(mem.attrib.get('start', '0x0'), 16)
        pagesize = int(mem.attrib.get('pagesize', '0x0'), 16)
        if m == 'flash' and name == 'FLASH': 
            device_info['flash_words'] = s // 2
            device_info['flash_page_size'] = pagesize
        elif m == 'ram' and external == 'false': 
            device_info['sram_bytes'] = s
        elif m == 'eeprom': device_info['eeprom_bytes'] = s
        elif m == 'fuses': device_info['fuse_addr'] = start
        elif m == 'lockbits': device_info['lockbit_addr'] = start
        elif m == 'signatures': device_info['sig_addr'] = start
    
    v = root.findall(".//interrupt")
    if v: device_info['vectors'] = max(int(x.attrib.get('index', '0')) for x in v) + 1
    
    device_info['vector_size'] = 4 if device_info['flash_words'] > 8192 else 2

    for aspace in root.findall(".//address-space"):
        as_name = aspace.attrib.get('name', '')
        if as_name in ('prog', 'flash'):
            for segment in aspace.findall("memory-segment"):
                s_name = segment.attrib.get('name', '')
                s_type = segment.attrib.get('type', '')
                if s_name == 'RWW':
                    device_info['flash_rww_end_word'] = (int(segment.attrib.get('start'), 0) + int(segment.attrib.get('size'), 0)) // 2
                elif 'BOOT' in s_name and s_type == 'flash':
                    start = int(segment.attrib.get('start'), 0) // 2
                    if device_info['flash_rww_end_word'] == 0 or start < device_info['flash_rww_end_word']:
                        device_info['flash_rww_end_word'] = start
    
    def add_reg(name, addr, init, bitfields):
        if name not in reg_map:
            reg_map[name] = {'addr': addr, 'init': init, 'bitfields': bitfields}

    for group in root.iter('register-group'):
        g_off = int(group.attrib.get('offset', '0x0'), 16)
        bias = 0x20 if group.attrib.get('address-space') == 'io' else 0
        for reg in group.findall('register'):
            name = reg.attrib['name']
            r_off = int(reg.attrib['offset'], 16)
            size = int(reg.attrib.get('size', '1'))
            init = int(reg.attrib.get('initval', '0x00'), 16)
            addr = bias + g_off + r_off
            bfs = {b.attrib['name']: int(b.attrib['mask'], 16) for b in reg.findall('bitfield')}
            add_reg(name, addr, init, bfs)
            if size == 2:
                add_reg(name + 'L', addr, init & 0xFF, bfs)
                add_reg(name + 'H', addr + 1, (init >> 8) & 0xFF, bfs)

    for reg in root.iter('memory-mapped-register'):
        name = reg.attrib['name']
        addr = int(reg.attrib.get('offset', '0x0'), 16)
        size = int(reg.attrib.get('size', '1'))
        init = int(reg.attrib.get('initval', '0x00'), 16)
        bfs = {b.attrib['name']: int(b.attrib['mask'], 16) for b in reg.findall('bitfield')}
        add_reg(name, addr, init, bfs)
        if size == 2:
            add_reg(name + 'L', addr, init & 0xFF, bfs)
            add_reg(name + 'H', addr + 1, (init >> 8) & 0xFF, bfs)

    def fr(r):
        p = re.compile(r, re.IGNORECASE)
        for n, data in reg_map.items():
            if p.fullmatch(n) or p.fullmatch(n.split('.')[-1]): 
                return data
        return None
    
    def fr_addr(n):
        if not n: return 0
        p = re.compile(f"PORT{n[1]}", re.IGNORECASE)
        for reg_n, data in reg_map.items():
            if p.fullmatch(reg_n) or p.fullmatch(reg_n.split('.')[-1]): return data['addr']
        return 0

    def fb(reg_name_re, bit_name_re, default=0):
        r = fr(reg_name_re)
        if not r: return default
        p = re.compile(bit_name_re, re.IGNORECASE)
        for bn, mask in r['bitfields'].items():
            if p.fullmatch(bn) or p.search(bn): return mask
        return default

    # nested signal map: instance_name -> group_name -> (port_addr, bit)
    signal_map = {}
    for inst in root.findall(".//instance"):
        iname = inst.attrib.get('name')
        if iname not in signal_map: signal_map[iname] = {}
        for sig in inst.findall(".//signal"):
            pad = sig.attrib.get('pad', '')
            group = sig.attrib.get('group', '')
            index = sig.attrib.get('index', '')
            key = group + (index if index else "")
            if pad:
                signal_map[iname][key] = (fr_addr(pad), int(pad[2:] if pad.startswith('P') and len(pad) > 2 and pad[2:].isdigit() else '0'))

    def fv(r):
        p = re.compile(r, re.IGNORECASE)
        for i in root.findall(".//interrupt"):
            if p.search(i.attrib.get('name', '')): return int(i.attrib.get('index', '0'))
        return 0

    adc_s = signal_map.get('ADC', {})
    pa, pb = [], []
    for i in range(8):
        pad = adc_s.get(f'ADC{i}', (0, 0))
        pa.append(pad[0])
        pb.append(pad[1])
    
    device_info['adc'] = {
        'adcl': (fr(r'ADCL') or {'addr':0})['addr'], 
        'adch': (fr(r'ADCH') or {'addr':0})['addr'], 
        'adcsra': (fr(r'ADCSRA') or {'addr':0})['addr'], 
        'adcsrb': (fr(r'ADCSRB') or {'addr':0})['addr'], 
        'admux': (fr(r'ADMUX') or {'addr':0})['addr'], 
        'vector': fv(r'ADC'), 
        'adcsra_reset': (fr(r'ADCSRA') or {'init':0})['init'], 
        'adcsrb_reset': (fr(r'ADCSRB') or {'init':0})['init'], 
        'admux_reset': (fr(r'ADMUX') or {'init':0})['init'],
        'didr0': (fr(r'DIDR0') or {'addr':0})['addr'], 'pin_addrs': pa, 'pin_bits': pb,
        'adsc': fb(r'ADCSRA', r'ADSC', 0x40), 'adif': fb(r'ADCSRA', r'ADIF', 0x10), 'adie': fb(r'ADCSRA', r'ADIE', 0x08)
    }
    
    ac_s = signal_map.get('AC', {})
    ain0 = ac_s.get('AIN0', (0, 0))
    ain1 = ac_s.get('AIN1', (0, 0))
    device_info['ac'] = {
        'acsr': (fr(r'ACSR') or {'addr':0})['addr'], 
        'didr1': (fr(r'DIDR1') or {'addr':0})['addr'], 
        'vector': fv(r'ANALOG_COMP'),
        'ain0_addr': ain0[0], 'ain0_bit': ain0[1],
        'ain1_addr': ain1[0], 'ain1_bit': ain1[1],
        'aci': fb(r'ACSR', r'ACI', 0x10), 'acie': fb(r'ACSR', r'ACIE', 0x08)
    }
    
    tc0_s = signal_map.get('TC0', {})
    device_info['timer0'] = { 
        'tcnt': (fr(r'TCNT0') or {'addr':0})['addr'], 
        'ocra': (fr(r'OCR0A|OCR0') or {'addr':0})['addr'], 
        'ocrb': (fr(r'OCR0B') or {'addr':0})['addr'], 
        'tccra': (fr(r'TCCR0A|TCCR0') or {'addr':0})['addr'], 
        'tccra_reset': (fr(r'TCCR0A|TCCR0') or {'init':0})['init'],
        'tccrb': (fr(r'TCCR0B') or {'addr':0})['addr'], 
        'tccrb_reset': (fr(r'TCCR0B') or {'init':0})['init'],
        'assr': 0, 'assr_reset': 0,
        'timsk': (fr(r'TIMSK0|TIMSK') or {'addr':0})['addr'], 
        'tifr': (fr(r'TIFR0|TIFR') or {'addr':0})['addr'], 
        'v_ovf': fv(r'TIMER0_OVF'), 'v_compa': fv(r'TIMER0_COMPA?'), 'v_compb': fv(r'TIMER0_COMPB'), 
        't_pin_addr': tc0_s.get('T', (0, 0))[0], 't_pin_bit': tc0_s.get('T', (0, 0))[1], 
        'ocra_pin_addr': tc0_s.get('OCA', (0, 0))[0], 'ocra_pin_bit': tc0_s.get('OCA', (0, 0))[1], 
        'ocrb_pin_addr': tc0_s.get('OCB', (0, 0))[0], 'ocrb_pin_bit': tc0_s.get('OCB', (0, 0))[1],
        'wgm0': fb(r'TCCR0A', r'WGM00', 0x01) | fb(r'TCCR0A', r'WGM01', 0x02),
        'wgm2': fb(r'TCCR0B', r'WGM02', 0x08),
        'cs': fb(r'TCCR0B', r'CS0.', 0x07)
    }
    
    tc2_s = signal_map.get('TC2', {})
    cpu_s = signal_map.get('CPU', {})
    device_info['timer2'] = { 
        'tcnt': (fr(r'TCNT2') or {'addr':0})['addr'], 
        'ocra': (fr(r'OCR2A|OCR2') or {'addr':0})['addr'], 
        'ocrb': (fr(r'OCR2B') or {'addr':0})['addr'], 
        'tccra': (fr(r'TCCR2A|TCCR2') or {'addr':0})['addr'], 
        'tccra_reset': (fr(r'TCCR2A|TCCR2') or {'init':0})['init'],
        'tccrb': (fr(r'TCCR2B') or {'addr':0})['addr'], 
        'tccrb_reset': (fr(r'TCCR2B') or {'init':0})['init'],
        'assr': (fr(r'ASSR') or {'addr':0})['addr'], 
        'assr_reset': (fr(r'ASSR') or {'init':0})['init'],
        'timsk': (fr(r'TIMSK2') or {'addr':0})['addr'], 
        'tifr': (fr(r'TIFR2') or {'addr':0})['addr'], 
        'v_ovf': fv(r'TIMER2_OVF'), 'v_compa': fv(r'TIMER2_COMPA?'), 'v_compb': fv(r'TIMER2_COMPB'), 
        'ocra_pin_addr': tc2_s.get('OCA', (0, 0))[0], 'ocra_pin_bit': tc2_s.get('OCA', (0, 0))[1], 
        'ocrb_pin_addr': tc2_s.get('OCB', (0, 0))[0], 'ocrb_pin_bit': tc2_s.get('OCB', (0, 0))[1], 
        'tosc1_pin_addr': tc2_s.get('TOSC1', cpu_s.get('XTAL1', (0, 0)))[0], 
        'tosc1_pin_bit': tc2_s.get('TOSC1', cpu_s.get('XTAL1', (0, 0)))[1], 
        'tosc2_pin_addr': tc2_s.get('TOSC2', cpu_s.get('XTAL2', (0, 0)))[0], 
        'tosc2_pin_bit': tc2_s.get('TOSC2', cpu_s.get('XTAL2', (0, 0)))[1],
        'wgm0': fb(r'TCCR2A', r'WGM20', 0x01) | fb(r'TCCR2A', r'WGM21', 0x02),
        'wgm2': fb(r'TCCR2B', r'WGM22', 0x08),
        'cs': fb(r'TCCR2B', r'CS2.', 0x07),
        'as2': fb(r'ASSR', r'AS2', 0x20),
        'tcn2ub': fb(r'ASSR', r'TCN2UB', 0x10)
    }
    
    device_info['timer1'] = { 
        'tcnt': (fr(r'TCNT1L?|TCNT1') or {'addr':0})['addr'], 
        'ocra': (fr(r'OCR1AL?|OCR1A') or {'addr':0})['addr'], 
        'ocrb': (fr(r'OCR1BL?|OCR1B') or {'addr':0})['addr'], 
        'icr': (fr(r'ICR1L?|ICR1') or {'addr':0})['addr'], 
        'tccra': (fr(r'TCCR1A') or {'addr':0})['addr'], 
        'tccra_reset': (fr(r'TCCR1A') or {'init':0})['init'],
        'tccrb': (fr(r'TCCR1B') or {'addr':0})['addr'], 
        'tccrb_reset': (fr(r'TCCR1B') or {'init':0})['init'],
        'tccrc': (fr(r'TCCR1C') or {'addr':0})['addr'], 
        'tccrc_reset': (fr(r'TCCR1C') or {'init':0})['init'],
        'timsk': (fr(r'TIMSK1|TIMSK') or {'addr':0})['addr'], 
        'tifr': (fr(r'TIFR1|TIFR') or {'addr':0})['addr'], 
        'v_ovf': fv(r'TIMER1_OVF'), 'v_compa': fv(r'TIMER1_COMPA?'), 'v_compb': fv(r'TIMER1_COMPB'), 'v_capt': fv(r'TIMER1_CAPT') 
    }
    
    device_info['uart0'] = { 
        'udr': (fr(r'UDR0|UDR') or {'addr':0})['addr'], 
        'ucsra': (fr(r'UCSR0A|UCSRA') or {'addr':0})['addr'], 
        'ucsrb': (fr(r'UCSR0B|UCSRB') or {'addr':0})['addr'], 
        'ucsrc': (fr(r'UCSR0C|UCSRC') or {'addr':0})['addr'], 
        'ubrrl': (fr(r'UBRR0L|UBRR') or {'addr':0})['addr'], 
        'ubrrh': (fr(r'UBRR0H|UBRRH') or {'addr':0})['addr'],
        'ucsra_reset': (fr(r'UCSR0A|UCSRA') or {'init':0})['init'], 
        'ucsrb_reset': (fr(r'UCSR0B|UCSRB') or {'init':0})['init'], 
        'ucsrc_reset': (fr(r'UCSR0C|UCSRC') or {'init':0})['init'],
        'v_rx': fv(r'USART0?_RX'), 'v_tx': fv(r'USART0?_TX'), 'v_udre': fv(r'USART0?_UDRE') 
    }
    device_info['spi'] = { 
        'spcr': (fr(r'SPCR') or {'addr':0})['addr'], 
        'spsr': (fr(r'SPSR') or {'addr':0})['addr'], 
        'spdr': (fr(r'SPDR') or {'addr':0})['addr'], 
        'spcr_reset': (fr(r'SPCR') or {'init':0})['init'], 
        'spsr_reset': (fr(r'SPSR') or {'init':0})['init'],
        'vector': fv(r'SPI_STC') 
    }
    device_info['twi'] = { 
        'twbr': (fr(r'TWBR') or {'addr':0})['addr'], 
        'twsr': (fr(r'TWSR') or {'addr':0})['addr'], 
        'twar': (fr(r'TWAR') or {'addr':0})['addr'], 
        'twdr': (fr(r'TWDR') or {'addr':0})['addr'], 
        'twcr': (fr(r'TWCR') or {'addr':0})['addr'], 
        'twamr': (fr(r'TWAMR') or {'addr':0})['addr'], 'vector': fv(r'TWI') 
    }
    device_info['eeprom_io'] = { 
        'eecr': (fr(r'EECR') or {'addr':0})['addr'], 
        'eedr': (fr(r'EEDR') or {'addr':0})['addr'], 
        'eearl': (fr(r'EEAR|EEARL') or {'addr':0})['addr'], 
        'eearh': (fr(r'EEARH') or {'addr':0})['addr'], 'vector': fv(r'EE_READY') 
    }
    device_info['wdt'] = { 'wdtcsr': (fr(r'WDTCSR|WDTCR') or {'addr':0})['addr'], 'vector': fv(r'WDT') }
    device_info['ext_interrupt'] = { 
        'eicra': (fr(r'EICRA') or {'addr':0})['addr'], 
        'eimsk': (fr(r'EIMSK|GIMSK|GICR') or {'addr':0})['addr'], 
        'eifr': (fr(r'EIFR|GIFR') or {'addr':0})['addr'], 'v_int0': fv(r'INT0'), 'v_int1': fv(r'INT1') 
    }
    device_info['pin_change_interrupt_0'] = { 'pcicr': (fr(r'PCICR') or {'addr':0})['addr'], 'pcifr': (fr(r'PCIFR') or {'addr':0})['addr'], 'pcmsk': (fr(r'PCMSK0') or {'addr':0})['addr'], 'enable_mask': 0x01, 'flag_mask': 0x01, 'vector': fv(r'PCINT0') }
    device_info['pin_change_interrupt_1'] = { 'pcicr': (fr(r'PCICR') or {'addr':0})['addr'], 'pcifr': (fr(r'PCIFR') or {'addr':0})['addr'], 'pcmsk': (fr(r'PCMSK1') or {'addr':0})['addr'], 'enable_mask': 0x02, 'flag_mask': 0x02, 'vector': fv(r'PCINT1') }
    device_info['pin_change_interrupt_2'] = { 'pcicr': (fr(r'PCICR') or {'addr':0})['addr'], 'pcifr': (fr(r'PCIFR') or {'addr':0})['addr'], 'pcmsk': (fr(r'PCMSK2') or {'addr':0})['addr'], 'enable_mask': 0x04, 'flag_mask': 0x04, 'vector': fv(r'PCINT2') }
    
    device_info['core_regs'] = {
        'spl_addr': (fr(r'SPL') or {'addr':0})['addr'], 'spl_reset': (fr(r'SPL') or {'init':0})['init'],
        'sph_addr': (fr(r'SPH') or {'addr':0})['addr'], 'sph_reset': (fr(r'SPH') or {'init':0})['init'],
        'sreg_addr': (fr(r'SREG') or {'addr':0})['addr'], 'sreg_reset': (fr(r'SREG') or {'init':0})['init']
    }
    device_info['spmcsr_addr'] = (fr(r'SPMCSR|SPMCR') or {'addr':0})['addr']
    device_info['smcr_addr'] = (fr(r'SMCR') or {'addr':0})['addr']
    device_info['mcusr_addr'] = (fr(r'MCUSR') or {'addr':0})['addr']
    device_info['prr_addr'] = (fr(r'PRR0|PRR') or {'addr':0})['addr']

    for p in "ABCDEFGH":
        p_info = {'name': f"PORT{p}", 'pin': (fr(f"PIN{p}") or {'addr':0})['addr'], 'ddr': (fr(f"DDR{p}") or {'addr':0})['addr'], 'port': (fr(f"PORT{p}") or {'addr':0})['addr']}
        if p_info['pin'] or p_info['ddr'] or p_info['port']: device_info['ports'].append(p_info)
    return device_info

def generate_header(d, output_dir):
    if not d.get('core_regs') or d['core_regs']['sreg_addr'] == 0:
        return
    
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

    adc_pin_addrs = ", ".join(hx(a) for a in d['adc']['pin_addrs'])
    adc_pin_bits = ", ".join(f"{b}U" for b in d['adc']['pin_bits'])
    
    t0 = d['timer0']
    t2 = d['timer2']
    
    content = f"""#pragma once
#include "vioavr/core/device.hpp"
namespace vioavr::core::devices {{
inline constexpr DeviceDescriptor {name} {{
    .name = "{d['name']}",
    .flash_words = {d['flash_words']}U, .sram_bytes = {d['sram_bytes']}U, .eeprom_bytes = {d['eeprom_bytes']}U,
    .interrupt_vector_count = {d['vectors']}U, .interrupt_vector_size = {d['vector_size']}U, .flash_page_size = {hx(d['flash_page_size'])},
    .spl_address = {hx(d['core_regs']['spl_addr'])}, .sph_address = {hx(d['core_regs']['sph_addr'])}, .sreg_address = {hx(d['core_regs']['sreg_addr'])}, .spmcsr_address = {hx(d['spmcsr_addr'])},
    .prr_address = {hx(d['prr_addr'])}, .smcr_address = {hx(d['smcr_addr'])}, .mcusr_address = {hx(d['mcusr_addr'])},
    .flash_rww_end_word = {d['flash_rww_end_word']}U,
    .spl_reset = {hx(d['core_regs']['spl_reset'])}, .sph_reset = {hx(d['core_regs']['sph_reset'])}, .sreg_reset = {hx(d['core_regs']['sreg_reset'])},
    .adc = {{ 
        .adcl_address = {hx(d['adc']['adcl'])}, .adch_address = {hx(d['adc']['adch'])}, .adcsra_address = {hx(d['adc']['adcsra'])}, .adcsrb_address = {hx(d['adc']['adcsrb'])}, .admux_address = {hx(d['adc']['admux'])}, .vector_index = {d['adc']['vector']}U, .adcsra_reset = {hx(d['adc']['adcsra_reset'])}, .adcsrb_reset = {hx(d['adc']['adcsrb_reset'])}, .admux_reset = {hx(d['adc']['admux_reset'])},
        .didr0_address = {hx(d['adc']['didr0'])},
        .adc_pin_address = {{{{ {adc_pin_addrs} }}}},
        .adc_pin_bit = {{{{ {adc_pin_bits} }}}},
        .auto_trigger_map = {{{{ AdcAutoTriggerSource::free_running, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::external_interrupt_0, AdcAutoTriggerSource::timer0_compare, AdcAutoTriggerSource::timer0_overflow, AdcAutoTriggerSource::timer1_compare_b, AdcAutoTriggerSource::timer1_overflow, AdcAutoTriggerSource::timer1_capture }}}},
        .adsc_mask = {hx(d['adc']['adsc'])}, .adif_mask = {hx(d['adc']['adif'])}, .adie_mask = {hx(d['adc']['adie'])}
    }},
    .ac = {{ 
        .acsr_address = {hx(d['ac']['acsr'])}, .didr1_address = {hx(d['ac']['didr1'])}, .vector_index = {d['ac']['vector']}U,
        .ain0_pin_address = {hx(d['ac']['ain0_addr'])}, .ain0_pin_bit = {d['ac']['ain0_bit']}U,
        .ain1_pin_address = {hx(d['ac']['ain1_addr'])}, .ain1_pin_bit = {d['ac']['ain1_bit']}U,
        .aci_mask = {hx(d['ac']['aci'])}, .acie_mask = {hx(d['ac']['acie'])}
    }},
    .timer0 = {{ .tcnt_address = {hx(t0['tcnt'])}, .ocra_address = {hx(t0['ocra'])}, .ocrb_address = {hx(t0['ocrb'])}, .tifr_address = {hx(t0['tifr'])}, .timsk_address = {hx(t0['timsk'])}, .tccra_address = {hx(t0['tccra'])}, .tccrb_address = {hx(t0['tccrb'])}, .assr_address = {hx(t0['assr'])}, .tccra_reset = {hx(t0['tccra_reset'])}, .tccrb_reset = {hx(t0['tccrb_reset'])}, .assr_reset = {hx(t0['assr_reset'])}, .compare_a_vector_index = {t0['v_compa']}U, .compare_b_vector_index = {t0['v_compb']}U, .overflow_vector_index = {t0['v_ovf']}U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U, .t0_pin_address = {hx(t0['t_pin_addr'])}, .t0_pin_bit = {t0['t_pin_bit']}U, .ocra_pin_address = {hx(t0['ocra_pin_addr'])}, .ocra_pin_bit = {t0['ocra_pin_bit']}U, .ocrb_pin_address = {hx(t0['ocrb_pin_addr'])}, .ocrb_pin_bit = {t0['ocrb_pin_bit']}U, .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0U, .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0U, .wgm0_mask = {hx(t0['wgm0'])}, .wgm2_mask = {hx(t0['wgm2'])}, .cs_mask = {hx(t0['cs'])} }},
    .timer2 = {{ .tcnt_address = {hx(t2['tcnt'])}, .ocra_address = {hx(t2['ocra'])}, .ocrb_address = {hx(t2['ocrb'])}, .tifr_address = {hx(t2['tifr'])}, .timsk_address = {hx(t2['timsk'])}, .tccra_address = {hx(t2['tccra'])}, .tccrb_address = {hx(t2['tccrb'])}, .assr_address = {hx(t2['assr'])}, .tccra_reset = {hx(t2['tccra_reset'])}, .tccrb_reset = {hx(t2['tccrb_reset'])}, .assr_reset = {hx(t2['assr_reset'])}, .compare_a_vector_index = {t2['v_compa']}U, .compare_b_vector_index = {t2['v_compb']}U, .overflow_vector_index = {t2['v_ovf']}U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U, .ocra_pin_address = {hx(t2['ocra_pin_addr'])}, .ocra_pin_bit = {t2['ocra_pin_bit']}U, .ocrb_pin_address = {hx(t2['ocrb_pin_addr'])}, .ocrb_pin_bit = {t2['ocrb_pin_bit']}U, .tosc1_pin_address = {hx(t2['tosc1_pin_addr'])}, .tosc1_pin_bit = {t2['tosc1_pin_bit']}U, .tosc2_pin_address = {hx(t2['tosc2_pin_addr'])}, .tosc2_pin_bit = {t2['tosc2_pin_bit']}U, .wgm0_mask = {hx(t2['wgm0'])}, .wgm2_mask = {hx(t2['wgm2'])}, .cs_mask = {hx(t2['cs'])}, .as2_mask = {hx(t2['as2'])}, .tcn2ub_mask = {hx(t2['tcn2ub'])} }},
    .timer1 = {{ .tcnt_address = {hx(d['timer1']['tcnt'])}, .ocra_address = {hx(d['timer1']['ocra'])}, .ocrb_address = {hx(d['timer1']['ocrb'])}, .icr_address = {hx(d['timer1']['icr'])}, .tifr_address = {hx(d['timer1']['tifr'])}, .timsk_address = {hx(d['timer1']['timsk'])}, .tccra_address = {hx(d['timer1']['tccra'])}, .tccrb_address = {hx(d['timer1']['tccrb'])}, .tccrc_address = {hx(d['timer1']['tccrc'])}, .tccra_reset = {hx(d['timer1']['tccra_reset'])}, .tccrb_reset = {hx(d['timer1']['tccrb_reset'])}, .tccrc_reset = {hx(d['timer1']['tccrc_reset'])}, .capture_vector_index = {d['timer1']['v_capt']}U, .compare_a_vector_index = {d['timer1']['v_compa']}U, .compare_b_vector_index = {d['timer1']['v_compb']}U, .overflow_vector_index = {d['timer1']['v_ovf']}U, .capture_enable_mask = 0x20U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U }},
    .ext_interrupt = {{ .eicra_address = {hx(d['ext_interrupt']['eicra'])}, .eimsk_address = {hx(d['ext_interrupt']['eimsk'])}, .eifr_address = {hx(d['ext_interrupt']['eifr'])}, .int0_vector_index = {d['ext_interrupt']['v_int0']}U, .int1_vector_index = {d['ext_interrupt']['v_int1']}U }},
    .uart0 = {{ .udr_address = {hx(d['uart0']['udr'])}, .ucsra_address = {hx(d['uart0']['ucsra'])}, .ucsrb_address = {hx(d['uart0']['ucsrb'])}, .ucsrc_address = {hx(d['uart0']['ucsrc'])}, .ubrrl_address = {hx(d['uart0']['ubrrl'])}, .ubrrh_address = {hx(d['uart0']['ubrrh'])}, .ucsra_reset = {hx(d['uart0']['ucsra_reset'])}, .ucsrb_reset = {hx(d['uart0']['ucsrb_reset'])}, .ucsrc_reset = {hx(d['uart0']['ucsrc_reset'])}, .rx_vector_index = {d['uart0']['v_rx']}U, .udre_vector_index = {d['uart0']['v_udre']}U, .tx_vector_index = {d['uart0']['v_tx']}U }},
    .pin_change_interrupt_0 = {{ .pcicr_address = {hx(d['pin_change_interrupt_0']['pcicr'])}, .pcifr_address = {hx(d['pin_change_interrupt_0']['pcifr'])}, .pcmsk_address = {hx(d['pin_change_interrupt_0']['pcmsk'])}, .pcicr_enable_mask = {hx(d['pin_change_interrupt_0']['enable_mask'])}, .pcifr_flag_mask = {hx(d['pin_change_interrupt_0']['flag_mask'])}, .vector_index = {d['pin_change_interrupt_0']['vector']}U }},
    .pin_change_interrupt_1 = {{ .pcicr_address = {hx(d['pin_change_interrupt_1']['pcicr'])}, .pcifr_address = {hx(d['pin_change_interrupt_1']['pcifr'])}, .pcmsk_address = {hx(d['pin_change_interrupt_1']['pcmsk'])}, .pcicr_enable_mask = {hx(d['pin_change_interrupt_1']['enable_mask'])}, .pcifr_flag_mask = {hx(d['pin_change_interrupt_1']['flag_mask'])}, .vector_index = {d['pin_change_interrupt_1']['vector']}U }},
    .pin_change_interrupt_2 = {{ .pcicr_address = {hx(d['pin_change_interrupt_2']['pcicr'])}, .pcifr_address = {hx(d['pin_change_interrupt_2']['pcifr'])}, .pcmsk_address = {hx(d['pin_change_interrupt_2']['pcmsk'])}, .pcicr_enable_mask = {hx(d['pin_change_interrupt_2']['enable_mask'])}, .pcifr_flag_mask = {hx(d['pin_change_interrupt_2']['flag_mask'])}, .vector_index = {d['pin_change_interrupt_2']['vector']}U }},
    .spi = {{ .spcr_address = {hx(d['spi']['spcr'])}, .spsr_address = {hx(d['spi']['spsr'])}, .spdr_address = {hx(d['spi']['spdr'])}, .spcr_reset = {hx(d['spi']['spcr_reset'])}, .spsr_reset = {hx(d['spi']['spsr_reset'])}, .vector_index = {d['spi']['vector']}U }},
    .twi = {{ .twbr_address = {hx(d['twi']['twbr'])}, .twsr_address = {hx(d['twi']['twsr'])}, .twar_address = {hx(d['twi']['twar'])}, .twdr_address = {hx(d['twi']['twdr'])}, .twcr_address = {hx(d['twi']['twcr'])}, .twamr_address = {hx(d['twi']['twamr'])}, .vector_index = {d['twi']['vector']}U }},
    .eeprom = {{ .eecr_address = {hx(d['eeprom_io']['eecr'])}, .eedr_address = {hx(d['eeprom_io']['eedr'])}, .eearl_address = {hx(d['eeprom_io']['eearl'])}, .eearh_address = {hx(d['eeprom_io']['eearh'])}, .vector_index = {d['eeprom_io']['vector']}U }},
    .wdt = {{ .wdtcsr_address = {hx(d['wdt']['wdtcsr'])}, .vector_index = {d['wdt']['vector']}U }},
    .fuse_address = {hx(d['fuse_addr'])}, .lockbit_address = {hx(d['lockbit_addr'])}, .signature_address = {hx(d['sig_addr'])},
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
        except Exception as e: pass

if __name__ == "__main__": main()
