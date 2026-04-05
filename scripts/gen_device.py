#!/usr/bin/env python3
import os, sys, re, xml.etree.ElementTree as ET
from pathlib import Path

def parse_atdf(file_path):
    device_name = file_path.stem
    tree = ET.parse(file_path)
    root = tree.getroot()
    device_info = {
        'name': device_name, 'flash_words': 0, 'sram_bytes': 0, 'eeprom_bytes': 0,
        'vectors': 0, 'vector_size': 4, 'adc': {}, 'timer0': {}, 'timer2': {},
        'timer1': {}, 'uart0': {}, 'ext_interrupt': {}, 
        'pin_change_interrupt_0': {}, 'pin_change_interrupt_1': {}, 'pin_change_interrupt_2': {}, 
        'spi': {}, 'twi': {}, 'eeprom_io': {}, 'wdt': {}, 'ports': [],
        'fuse_addr': 0, 'lockbit_addr': 0, 'sig_addr': 0,
        'flash_rww_end_word': 0
    }
    
    for mem in root.findall(".//memory-segment"):
        m, s = mem.attrib.get('type'), int(mem.attrib.get('size', '0x0'), 16)
        name = mem.attrib.get('name', '')
        external = mem.attrib.get('external', 'false')
        start = int(mem.attrib.get('start', '0x0'), 16)
        if m == 'flash' and name == 'FLASH': 
            device_info['flash_words'] = s // 2
        elif m == 'ram' and external == 'false': 
            device_info['sram_bytes'] = s
        elif m == 'eeprom': device_info['eeprom_bytes'] = s
        elif m == 'fuses': device_info['fuse_addr'] = start
        elif m == 'lockbits': device_info['lockbit_addr'] = start
        elif m == 'signatures': device_info['sig_addr'] = start
    
    v = root.findall(".//interrupt")
    if v: device_info['vectors'] = max(int(x.attrib.get('index', '0')) for x in v) + 1
    
    # RWW end calculation
    # Some ATDFs use 'prog', others use 'flash' for the address space name
    for aspace in root.findall(".//address-space"):
        as_name = aspace.attrib.get('name', '')
        if as_name in ('prog', 'flash'):
            for segment in aspace.findall("memory-segment"):
                s_name = segment.attrib.get('name', '')
                s_type = segment.attrib.get('type', '')
                if s_name == 'RWW':
                    device_info['flash_rww_end_word'] = (int(segment.attrib.get('start'), 0) + int(segment.attrib.get('size'), 0)) // 2
                elif 'BOOT' in s_name and s_type == 'flash':
                    # RWW ends where the first BOOT section starts
                    start = int(segment.attrib.get('start'), 0) // 2
                    if device_info['flash_rww_end_word'] == 0 or start < device_info['flash_rww_end_word']:
                        device_info['flash_rww_end_word'] = start
    
    # Default if no RWW/BOOT split found (assume everything is RWW or NRWW)
    if device_info['flash_rww_end_word'] == 0 and device_info['flash_words'] > 0:
        # Most megaAVRs follow this pattern: flash_words-1 if no dedicated NRWW
        pass
    reg_map = {}
    signal_map = {}

    def add_reg(name, addr, init):
        if name not in reg_map:
            reg_map[name] = {'addr': addr, 'init': init}

    for group in root.iter('register-group'):
        g_off = int(group.attrib.get('offset', '0x0'), 16)
        bias = 0x20 if group.attrib.get('address-space') == 'io' else 0
        for reg in group.findall('register'):
            name = reg.attrib['name']
            r_off = int(reg.attrib['offset'], 16)
            size = int(reg.attrib.get('size', '1'))
            init = int(reg.attrib.get('initval', '0x00'), 16)
            addr = bias + g_off + r_off
            add_reg(name, addr, init)
            if size == 2:
                add_reg(name + 'L', addr, init & 0xFF)
                add_reg(name + 'H', addr + 1, (init >> 8) & 0xFF)

    for reg in root.iter('memory-mapped-register'):
        name = reg.attrib['name']
        addr = int(reg.attrib.get('offset', '0x0'), 16)
        size = int(reg.attrib.get('size', '1'))
        init = int(reg.attrib.get('initval', '0x00'), 16)
        add_reg(name, addr, init)
        if size == 2:
            add_reg(name + 'L', addr, init & 0xFF)
            add_reg(name + 'H', addr + 1, (init >> 8) & 0xFF)

    def fr(r):
        p = re.compile(r, re.IGNORECASE)
        for n, data in reg_map.items():
            if p.fullmatch(n): return data
        return {'addr': 0, 'init': 0}

    def fv(r):
        p = re.compile(r, re.IGNORECASE)
        for i in root.findall(".//interrupt"):
            if p.search(i.attrib.get('name', '')): return int(i.attrib.get('index', '0'))
        return 0

    def fm(r):
        for m in root.findall(".//memory-segment"):
            if re.search(r, m.attrib.get('name', ''), re.I):
                return int(m.attrib.get('pagesize', '0'), 0)
        return 0

    device_info['flash_page_size'] = fm(r'FLASH')

    def parse_pad(pad):
        m = re.fullmatch(r'P([A-H])(\d)', pad or '')
        if not m: return (0, 0)
        bit = int(m.group(2))
        pin_data = fr(rf'PIN{m.group(1)}')
        return (pin_data['addr'], bit) if pin_data['addr'] else (0, bit)

    for instance in root.findall(".//instance"):
        instance_name = instance.attrib.get('name', '')
        instance_signals = {}
        for signal in instance.findall("./signals/signal"):
            group = signal.attrib.get('group', '')
            index = signal.attrib.get('index')
            key = f"{group}{index}" if index is not None else group
            instance_signals[key] = parse_pad(signal.attrib.get('pad', ''))
        signal_map[instance_name] = instance_signals

    d_adc = fr(r'ADCSRA|ADCSR')
    d_adcsrb = fr(r'ADCSRB')
    d_admux = fr(r'ADMUX')
    d_didr0 = fr(r'DIDR0')
    adc_sigs = signal_map.get('ADC', {})
    adc_pin_addrs = [adc_sigs.get(f'ADC{i}', (0, 0)) for i in range(8)]
    
    device_info['adc'] = { 
        'adcl': fr(r'ADCL')['addr'], 'adch': fr(r'ADCH')['addr'], 
        'adcsra': d_adc['addr'], 'adcsra_reset': d_adc['init'],
        'adcsrb': d_adcsrb['addr'], 'adcsrb_reset': d_adcsrb['init'],
        'admux': d_admux['addr'], 'admux_reset': d_admux['init'],
        'didr0': d_didr0['addr'],
        'pin_addrs': [a for a, b in adc_pin_addrs],
        'pin_bits': [b for a, b in adc_pin_addrs],
        'vector': fv(r'ADC') 
    }
    
    d_acsr = fr(r'ACSR')
    d_didr1 = fr(r'DIDR1')
    ac_sigs = signal_map.get('AC', {})
    ain0 = ac_sigs.get('AIN0', (0, 0))
    ain1 = ac_sigs.get('AIN1', (0, 0))
    device_info['ac'] = {
        'acsr': d_acsr['addr'], 'vector': fv(r'ANA_COMP|ANALOG_COMP'),
        'didr1': d_didr1['addr'],
        'ain0_addr': ain0[0], 'ain0_bit': ain0[1],
        'ain1_addr': ain1[0], 'ain1_bit': ain1[1]
    }
    
    tc0 = signal_map.get('TC0', {})
    d_tccra0 = fr(r'TCCR0A|TCCR0')
    d_tccrb0 = fr(r'TCCR0B')
    device_info['timer0'] = { 
        'tcnt': fr(r'TCNT0')['addr'], 'ocra': fr(r'OCR0A|OCR0')['addr'], 'ocrb': fr(r'OCR0B')['addr'], 
        'tccra': d_tccra0['addr'], 'tccra_reset': d_tccra0['init'],
        'tccrb': d_tccrb0['addr'], 'tccrb_reset': d_tccrb0['init'],
        'assr': 0, 'assr_reset': 0,
        'timsk': fr(r'TIMSK0|TIMSK')['addr'], 'tifr': fr(r'TIFR0|TIFR')['addr'], 
        'v_ovf': fv(r'TIMER0_OVF'), 'v_compa': fv(r'TIMER0_COMPA?'), 'v_compb': fv(r'TIMER0_COMPB'), 
        't_pin_addr': tc0.get('T', (0, 0))[0], 't_pin_bit': tc0.get('T', (0, 0))[1], 
        'ocra_pin_addr': tc0.get('OCA', (0, 0))[0], 'ocra_pin_bit': tc0.get('OCA', (0, 0))[1], 
        'ocrb_pin_addr': tc0.get('OCB', (0, 0))[0], 'ocrb_pin_bit': tc0.get('OCB', (0, 0))[1], 
        'tosc1_pin_addr': 0, 'tosc1_pin_bit': 0, 'tosc2_pin_addr': 0, 'tosc2_pin_bit': 0 
    }
    
    tc2 = signal_map.get('TC2', {})
    d_tccra2 = fr(r'TCCR2A|TCCR2')
    d_tccrb2 = fr(r'TCCR2B')
    d_assr2 = fr(r'ASSR')
    device_info['timer2'] = { 
        'tcnt': fr(r'TCNT2')['addr'], 'ocra': fr(r'OCR2A|OCR2')['addr'], 'ocrb': fr(r'OCR2B')['addr'], 
        'tccra': d_tccra2['addr'], 'tccra_reset': d_tccra2['init'],
        'tccrb': d_tccrb2['addr'], 'tccrb_reset': d_tccrb2['init'],
        'assr': d_assr2['addr'], 'assr_reset': d_assr2['init'],
        'timsk': fr(r'TIMSK2')['addr'], 'tifr': fr(r'TIFR2')['addr'], 
        'v_ovf': fv(r'TIMER2_OVF'), 'v_compa': fv(r'TIMER2_COMPA?'), 'v_compb': fv(r'TIMER2_COMPB'), 
        't_pin_addr': 0, 't_pin_bit': 0, 
        'ocra_pin_addr': tc2.get('OCA', (0, 0))[0], 'ocra_pin_bit': tc2.get('OCA', (0, 0))[1], 
        'ocrb_pin_addr': tc2.get('OCB', (0, 0))[0], 'ocrb_pin_bit': tc2.get('OCB', (0, 0))[1], 
        'tosc1_pin_addr': tc2.get('TOSC1', (0, 0))[0], 'tosc1_pin_bit': tc2.get('TOSC1', (0, 0))[1], 
        'tosc2_pin_addr': tc2.get('TOSC2', (0, 0))[0], 'tosc2_pin_bit': tc2.get('TOSC2', (0, 0))[1] 
    }
    
    d_tccra1 = fr(r'TCCR1A')
    d_tccrb1 = fr(r'TCCR1B')
    d_tccrc1 = fr(r'TCCR1C')
    device_info['timer1'] = { 
        'tcnt': fr(r'TCNT1|TCNT1L')['addr'], 'ocra': fr(r'OCR1A|OCR1AL')['addr'], 'ocrb': fr(r'OCR1B|OCR1BL')['addr'], 
        'icr': fr(r'ICR1|ICR1L')['addr'], 'tccra': d_tccra1['addr'], 'tccra_reset': d_tccra1['init'],
        'tccrb': d_tccrb1['addr'], 'tccrb_reset': d_tccrb1['init'],
        'tccrc': d_tccrc1['addr'], 'tccrc_reset': d_tccrc1['init'],
        'timsk': fr(r'TIMSK1|TIMSK')['addr'], 'tifr': fr(r'TIFR1|TIFR')['addr'], 
        'v_ovf': fv(r'TIMER1_OVF'), 'v_compa': fv(r'TIMER1_COMPA?'), 'v_compb': fv(r'TIMER1_COMPB'), 'v_capt': fv(r'TIMER1_CAPT') 
    }
    
    device_info['uart0'] = { 
        'udr': fr(r'UDR0|UDR')['addr'], 'ucsra': fr(r'UCSR0A|UCSRA')['addr'], 'ucsrb': fr(r'UCSR0B|UCSRB')['addr'], 'ucsrc': fr(r'UCSR0C|UCSRC')['addr'], 
        'ubrrl': fr(r'UBRR0L|UBRR')['addr'], 'ubrrh': fr(r'UBRR0H|UBRRH')['addr'],
        'ucsra_reset': fr(r'UCSR0A|UCSRA')['init'], 'ucsrb_reset': fr(r'UCSR0B|UCSRB')['init'], 'ucsrc_reset': fr(r'UCSR0C|UCSRC')['init'],
        'v_rx': fv(r'USART0?_RX'), 'v_tx': fv(r'USART0?_TX'), 'v_udre': fv(r'USART0?_UDRE') 
    }
    device_info['spi'] = { 
        'spcr': fr(r'SPCR')['addr'], 'spsr': fr(r'SPSR')['addr'], 'spdr': fr(r'SPDR')['addr'], 
        'spcr_reset': fr(r'SPCR')['init'], 'spsr_reset': fr(r'SPSR')['init'],
        'vector': fv(r'SPI_STC') 
    }
    device_info['twi'] = { 'twbr': fr(r'TWBR')['addr'], 'twsr': fr(r'TWSR')['addr'], 'twar': fr(r'TWAR')['addr'], 'twdr': fr(r'TWDR')['addr'], 'twcr': fr(r'TWCR')['addr'], 'twamr': fr(r'TWAMR')['addr'], 'vector': fv(r'TWI') }
    device_info['eeprom_io'] = { 'eecr': fr(r'EECR')['addr'], 'eedr': fr(r'EEDR')['addr'], 'eearl': fr(r'EEAR|EEARL')['addr'], 'eearh': fr(r'EEARH')['addr'], 'vector': fv(r'EE_READY') }
    device_info['wdt'] = { 'wdtcsr': fr(r'WDTCSR|WDTCR')['addr'], 'vector': fv(r'WDT') }
    device_info['ext_interrupt'] = { 'eicra': fr(r'EICRA')['addr'], 'eimsk': fr(r'EIMSK|GIMSK|GICR')['addr'], 'eifr': fr(r'EIFR|GIFR')['addr'], 'v_int0': fv(r'INT0'), 'v_int1': fv(r'INT1') }
    device_info['pin_change_interrupt_0'] = { 'pcicr': fr(r'PCICR')['addr'], 'pcifr': fr(r'PCIFR')['addr'], 'pcmsk': fr(r'PCMSK0')['addr'], 'enable_mask': 0x01, 'flag_mask': 0x01, 'vector': fv(r'PCINT0') }
    device_info['pin_change_interrupt_1'] = { 'pcicr': fr(r'PCICR')['addr'], 'pcifr': fr(r'PCIFR')['addr'], 'pcmsk': fr(r'PCMSK1')['addr'], 'enable_mask': 0x02, 'flag_mask': 0x02, 'vector': fv(r'PCINT1') }
    device_info['pin_change_interrupt_2'] = { 'pcicr': fr(r'PCICR')['addr'], 'pcifr': fr(r'PCIFR')['addr'], 'pcmsk': fr(r'PCMSK2')['addr'], 'enable_mask': 0x04, 'flag_mask': 0x04, 'vector': fv(r'PCINT2') }
    
    d_spl = fr(r'SPL')
    d_sph = fr(r'SPH')
    d_sreg = fr(r'SREG')
    device_info['core_regs'] = {
        'spl_addr': d_spl['addr'], 'spl_reset': d_spl['init'],
        'sph_addr': d_sph['addr'], 'sph_reset': d_sph['init'],
        'sreg_addr': d_sreg['addr'], 'sreg_reset': d_sreg['init']
    }
    device_info['spmcsr_addr'] = fr(r'SPMCSR|SPMCR')['addr']
    device_info['smcr_addr'] = fr(r'SMCR')['addr']
    device_info['mcusr_addr'] = fr(r'MCUSR')['addr']

    for p in "ABCDEFGH":
        p_info = {'name': f"PORT{p}", 'pin': fr(f"PIN{p}")['addr'], 'ddr': fr(f"DDR{p}")['addr'], 'port': fr(f"PORT{p}")['addr']}
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

    adc_pin_addrs = ", ".join(hx(a) for a in d['adc']['pin_addrs'])
    adc_pin_bits = ", ".join(f"{b}U" for b in d['adc']['pin_bits'])
    content = f"""#pragma once
#include "vioavr/core/device.hpp"
namespace vioavr::core::devices {{
inline constexpr DeviceDescriptor {name} {{
    .name = "{d['name']}",
    .flash_words = {d['flash_words']}U, .sram_bytes = {d['sram_bytes']}U, .eeprom_bytes = {d['eeprom_bytes']}U,
    .interrupt_vector_count = {d['vectors']}U, .interrupt_vector_size = {d['vector_size']}U, .flash_page_size = {hx(d['flash_page_size'])},
    .spl_address = {hx(d['core_regs']['spl_addr'])}, .sph_address = {hx(d['core_regs']['sph_addr'])}, .sreg_address = {hx(d['core_regs']['sreg_addr'])}, .spmcsr_address = {hx(d['spmcsr_addr'])},
    .smcr_address = {hx(d['smcr_addr'])}, .mcusr_address = {hx(d['mcusr_addr'])},
    .flash_rww_end_word = {d['flash_rww_end_word']}U,
    .spl_reset = {hx(d['core_regs']['spl_reset'])}, .sph_reset = {hx(d['core_regs']['sph_reset'])}, .sreg_reset = {hx(d['core_regs']['sreg_reset'])},
    .adc = {{ 
        .adcl_address = {hx(d['adc']['adcl'])}, .adch_address = {hx(d['adc']['adch'])}, .adcsra_address = {hx(d['adc']['adcsra'])}, .adcsrb_address = {hx(d['adc']['adcsrb'])}, .admux_address = {hx(d['adc']['admux'])}, .vector_index = {d['adc']['vector']}U, .adcsra_reset = {hx(d['adc']['adcsra_reset'])}, .adcsrb_reset = {hx(d['adc']['adcsrb_reset'])}, .admux_reset = {hx(d['adc']['admux_reset'])},
        .didr0_address = {hx(d['adc']['didr0'])},
        .adc_pin_address = {{{{ {adc_pin_addrs} }}}},
        .adc_pin_bit = {{{{ {adc_pin_bits} }}}},
        .auto_trigger_map = {{{{ AdcAutoTriggerSource::free_running, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::external_interrupt_0, AdcAutoTriggerSource::timer0_compare, AdcAutoTriggerSource::timer0_overflow, AdcAutoTriggerSource::timer1_compare_b, AdcAutoTriggerSource::timer1_overflow, AdcAutoTriggerSource::timer1_capture }}}}
    }},
    .ac = {{ 
        .acsr_address = {hx(d['ac']['acsr'])}, .didr1_address = {hx(d['ac']['didr1'])}, .vector_index = {d['ac']['vector']}U,
        .ain0_pin_address = {hx(d['ac']['ain0_addr'])}, .ain0_pin_bit = {d['ac']['ain0_bit']}U,
        .ain1_pin_address = {hx(d['ac']['ain1_addr'])}, .ain1_pin_bit = {d['ac']['ain1_bit']}U
    }},
    .timer0 = {{ .tcnt_address = {hx(d['timer0']['tcnt'])}, .ocra_address = {hx(d['timer0']['ocra'])}, .ocrb_address = {hx(d['timer0']['ocrb'])}, .tifr_address = {hx(d['timer0']['tifr'])}, .timsk_address = {hx(d['timer0']['timsk'])}, .tccra_address = {hx(d['timer0']['tccra'])}, .tccrb_address = {hx(d['timer0']['tccrb'])}, .assr_address = {hx(d['timer0']['assr'])}, .tccra_reset = {hx(d['timer0']['tccra_reset'])}, .tccrb_reset = {hx(d['timer0']['tccrb_reset'])}, .assr_reset = {hx(d['timer0']['assr_reset'])}, .compare_a_vector_index = {d['timer0']['v_compa']}U, .compare_b_vector_index = {d['timer0']['v_compb']}U, .overflow_vector_index = {d['timer0']['v_ovf']}U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U, .t0_pin_address = {hx(d['timer0']['t_pin_addr'])}, .t0_pin_bit = {d['timer0']['t_pin_bit']}U, .ocra_pin_address = {hx(d['timer0']['ocra_pin_addr'])}, .ocra_pin_bit = {d['timer0']['ocra_pin_bit']}U, .ocrb_pin_address = {hx(d['timer0']['ocrb_pin_addr'])}, .ocrb_pin_bit = {d['timer0']['ocrb_pin_bit']}U, .tosc1_pin_address = {hx(d['timer0']['tosc1_pin_addr'])}, .tosc1_pin_bit = {d['timer0']['tosc1_pin_bit']}U, .tosc2_pin_address = {hx(d['timer0']['tosc2_pin_addr'])}, .tosc2_pin_bit = {d['timer0']['tosc2_pin_bit']}U }},
    .timer2 = {{ .tcnt_address = {hx(d['timer2']['tcnt'])}, .ocra_address = {hx(d['timer2']['ocra'])}, .ocrb_address = {hx(d['timer2']['ocrb'])}, .tifr_address = {hx(d['timer2']['tifr'])}, .timsk_address = {hx(d['timer2']['timsk'])}, .tccra_address = {hx(d['timer2']['tccra'])}, .tccrb_address = {hx(d['timer2']['tccrb'])}, .assr_address = {hx(d['timer2']['assr'])}, .tccra_reset = {hx(d['timer2']['tccra_reset'])}, .tccrb_reset = {hx(d['timer2']['tccrb_reset'])}, .assr_reset = {hx(d['timer2']['assr_reset'])}, .compare_a_vector_index = {d['timer2']['v_compa']}U, .compare_b_vector_index = {d['timer2']['v_compb']}U, .overflow_vector_index = {d['timer2']['v_ovf']}U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U, .t0_pin_address = {hx(d['timer2']['t_pin_addr'])}, .t0_pin_bit = {d['timer2']['t_pin_bit']}U, .ocra_pin_address = {hx(d['timer2']['ocra_pin_addr'])}, .ocra_pin_bit = {d['timer2']['ocra_pin_bit']}U, .ocrb_pin_address = {hx(d['timer2']['ocrb_pin_addr'])}, .ocrb_pin_bit = {d['timer2']['ocrb_pin_bit']}U, .tosc1_pin_address = {hx(d['timer2']['tosc1_pin_addr'])}, .tosc1_pin_bit = {d['timer2']['tosc1_pin_bit']}U, .tosc2_pin_address = {hx(d['timer2']['tosc2_pin_addr'])}, .tosc2_pin_bit = {d['timer2']['tosc2_pin_bit']}U }},
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
        except Exception as e: print(f"FAIL {f.name}: {e}")

if __name__ == "__main__": main()
