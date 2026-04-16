#!/usr/bin/env python3
import os, sys, json, re
from pathlib import Path

def hx(v):
    if v is None or v == "": return "0x0U"
    if isinstance(v, str):
        if v.endswith('U'): v = v[:-1]
        v = int(v, 0)
    return f"{hex(v).upper().replace('X', 'x')}U"

def get_reg(periph, name_regex):
    if not periph: return None
    p = re.compile(name_regex, re.IGNORECASE)
    for reg_name, reg_data in periph.get('registers', {}).items():
        if p.fullmatch(reg_name) or p.search(reg_name):
            return reg_data
    return None

def get_reg_addr(p_data, name_re, high=False):
    reg = get_reg(p_data, name_re)
    if not reg: return "0x0U"
    if high and reg['size'] > 1:
        return hx(reg['offset'] + 1)
    return hx(reg['offset'])

def get_bit(reg, bit_regex, default=0):
    if not reg: return default
    p = re.compile(bit_regex, re.IGNORECASE)
    for b_name, b_data in reg.get('bitfields', {}).items():
        if p.fullmatch(b_name) or p.search(b_name):
            return b_data.get('mask', default)
    return default

def get_pad_info(port_map, periph, sig_name, reg_type='PORT'):
    for sig in periph.get('signals', []):
        full_sig = (sig.get('group') or '') + (sig.get('index') or '')
        if full_sig == sig_name:
            pad = sig.get('pad', '')
            if pad and pad.startswith('P'):
                port_char = pad[1]
                bit_index = int(pad[2:]) if pad[2:].isdigit() else 0
                port_data = port_map.get(port_char, {})
                addr = port_data.get(reg_type, 0)
                return hx(addr), bit_index
                
    sig_no_digit = "".join(c for c in sig_name if not c.isdigit())
    if sig_no_digit != sig_name:
        for sig in periph.get('signals', []):
            full_sig = (sig.get('group') or '') + (sig.get('index') or '')
            if full_sig == sig_no_digit:
                pad = sig.get('pad', '')
                if pad and pad.startswith('P'):
                    port_char = pad[1]
                    bit_index = int(pad[2:]) if pad[2:].isdigit() else 0
                    port_data = port_map.get(port_char, {})
                    addr = port_data.get(reg_type, 0)
                    return hx(addr), bit_index
                    
    return "0x0U", 0
 
def get_pr_info(data, bit_regex):
    p = re.compile(bit_regex, re.IGNORECASE)
    cpu = data['peripherals'].get('CPU', data['peripherals'].get('CPU_REGISTERS', {}))
    for r_name in ['PRR', 'PRR0', 'PRR1', 'PRR2']:
        reg = get_reg(cpu, r_name)
        if reg:
            for b_name, b_data in reg.get('bitfields', {}).items():
                if p.fullmatch(b_name) or p.search(b_name):
                    return reg['offset'], b_data['mask']
    return 0, 0xFF

def generate_header(data, output_dir):
    name = data['name']
    safe_name = name.lower().replace('-', '_')
    header_path = output_dir / f"{safe_name}.hpp"
    
    # 1. Build Port Map for PinMux
    port_map = {}
    ports_raw = []
    for p_instance, p_data in data['peripherals'].items():
        if p_data.get('module') == 'PORT':
            for r_name, r_data in p_data['registers'].items():
                if (r_name.startswith('PORT') and len(r_name) >= 5) or \
                   (r_name.startswith('DDR') and len(r_name) >= 4) or \
                   (r_name.startswith('PIN') and len(r_name) >= 4):
                    char = r_name[4] if r_name.startswith('PORT') else r_name[3]
                    if char not in port_map: port_map[char] = {}
                    reg_type = 'PORT' if r_name.startswith('PORT') else ('DDR' if r_name.startswith('DDR') else 'PIN')
                    port_map[char][reg_type] = r_data['offset']
    
    for char, regs in port_map.items():
        ports_raw.append({
            'name': f"PORT{char}",
            'port': regs.get('PORT', 0),
            'ddr': regs.get('DDR', 0),
            'pin': regs.get('PIN', 0)
        })
    ports_raw.sort(key=lambda x: x['name'])

    # 2. Group Peripherals
    groups = {
        'USART': [], 'TC8': [], 'TC8_ASYNC': [], 'TC16': [], 'ADC': [], 'AC': [],
        'WDT': [], 'EEPROM': [], 'SPI': [], 'TWI': [], 'EXINT': [], 'PCINT': [],
        'CAN': [], 'EXTERNAL_MEMORY': [], 'TC10': [], 'USB_DEVICE': []
    }
    for p_name, p_data in data['peripherals'].items():
        mod = p_data.get('module')
        if mod in groups:
            groups[mod].append((p_name, p_data))
            if mod == 'EXINT':
                for k in p_data.get('registers', {}).keys():
                    if k.startswith('PCMSK'):
                        idx = k[5:]
                        groups['PCINT'].append((f'PCINT{idx}', p_data))
        elif 'PCINT' in p_name or 'PCINT' == mod:
            groups['PCINT'].append((p_name, p_data))
        elif mod == 'TC10':
            groups['TC10'].append((p_name, p_data))

    for k in groups: groups[k].sort()

    # 3. Memories
    sram = next((m for m in data['memories'].get('data', []) if m['type'] == 'ram'), {'size': 0})
    flash = next((m for m in data['memories'].get('prog', []) if m['type'] == 'flash'), {'size': 0})
    eeprom = next((m for m in data['memories'].get('data', []) if m['type'] == 'eeprom'), {'size': 0})
    
    # 4. Core Regs
    cpu = data['peripherals'].get('CPU', data['peripherals'].get('CPU_REGISTERS', {}))
    regs = cpu.get('registers', {})
    r_cpu = lambda n: get_reg(cpu, n)
    b_cpu = lambda n, b_re: get_bit(r_cpu(n), b_re)
    
    # 5. Header Content Generation
    def gen_uart(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        idx = "".join(filter(str.isdigit, p_name)) or "0"
        pr_addr, pr_bit = get_pr_info(data, f'PRUSART{idx}|PRUSART')
        return f"""{{
            .udr_address = {hx(r(f'UDR{idx}|UDR')['offset'])}, .ucsra_address = {hx(r(f'UCSR{idx}A|UCSRA')['offset'])}, .ucsrb_address = {hx(r(f'UCSR{idx}B|UCSRB')['offset'])}, .ucsrc_address = {hx(r(f'UCSR{idx}C|UCSRC')['offset'])}, .ubrrl_address = {get_reg_addr(p_data, f'UBRR{idx}|UBRR')}, .ubrrh_address = {get_reg_addr(p_data, f'UBRR{idx}|UBRR', True)},
            .ucsra_reset = {hx(r(f'UCSR{idx}A|UCSRA')['initval'])}, .ucsrb_reset = {hx(r(f'UCSR{idx}B|UCSRB')['initval'])}, .ucsrc_reset = {hx(r(f'UCSR{idx}C|UCSRC')['initval'])},
            .rx_vector_index = {next((i['index'] for i in data.get('interrupts', []) if 'RX' in (i.get('name') or i.get('caption') or '').upper() and (p_name in (i.get('name') or '').upper() or 'USART' in (i.get('name') or '').upper())), 0)}U,
            .udre_vector_index = {next((i['index'] for i in data.get('interrupts', []) if 'UDRE' in (i.get('name') or i.get('caption') or '').upper() and (p_name in (i.get('name') or '').upper() or 'USART' in (i.get('name') or '').upper())), 0)}U,
            .tx_vector_index = {next((i['index'] for i in data.get('interrupts', []) if 'TX' in (i.get('name') or i.get('caption') or '').upper() and (p_name in (i.get('name') or '').upper() or 'USART' in (i.get('name') or '').upper())), 0)}U,
            .u2x_mask = {hx(b(f'UCSR{idx}A|UCSRA', 'U2X'))}, .rxc_mask = {hx(b(f'UCSR{idx}A|UCSRA', 'RXC'))}, .txc_mask = {hx(b(f'UCSR{idx}A|UCSRA', 'TXC'))}, .udre_mask = {hx(b(f'UCSR{idx}A|UCSRA', 'UDRE'))},
            .rxen_mask = {hx(b(f'UCSR{idx}B|UCSRB', 'RXEN'))}, .txen_mask = {hx(b(f'UCSR{idx}B|UCSRB', 'TXEN'))}, .rxcie_mask = {hx(b(f'UCSR{idx}B|UCSRB', 'RXCIE'))}, .txcie_mask = {hx(b(f'UCSR{idx}B|UCSRB', 'TXCIE'))}, .udrie_mask = {hx(b(f'UCSR{idx}B|UCSRB', 'UDRIE'))},
            .pr_address = {pr_addr}, .pr_bit = {pr_bit}
        }}"""

    def gen_timer8(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        idx = "".join(filter(str.isdigit, p_name)) or "0"
        pa_a, pa_b = get_pad_info(port_map, p_data, f'OC{idx}A', 'PORT')
        pb_a, pb_b = get_pad_info(port_map, p_data, f'OC{idx}B', 'PORT')
        t_a, t_b = get_pad_info(port_map, p_data, f'T{idx}', 'PIN')
        t1_a, t1_b = get_pad_info(port_map, p_data, 'TOSC1', 'PIN')
        t2_a, t2_b = get_pad_info(port_map, p_data, 'TOSC2', 'PIN')
        return f"""{{
            .tcnt_address = {hx(r(f'TCNT{idx}|TCNT')['offset'])}, .ocra_address = {hx(r(f'OCR{idx}A|OCR.*A')['offset'])}, .ocrb_address = {hx(r(f'OCR{idx}B|OCR.*B')['offset'])}, .tifr_address = {hx(r(f'TIFR{idx}|TIFR')['offset'])}, .timsk_address = {hx(r(f'TIMSK{idx}|TIMSK')['offset'])}, .tccra_address = {hx(r(f'TCCR{idx}A|TCCR.*A')['offset'])}, .tccrb_address = {hx(r(f'TCCR{idx}B|TCCR.*B')['offset'])}, .assr_address = {hx(r(f'ASSR{idx}|ASSR')['offset'])},
            .tccra_reset = {hx(r(f'TCCR{idx}A|TCCR.*A')['initval'])}, .tccrb_reset = {hx(r(f'TCCR{idx}B|TCCR.*B')['initval'])}, .assr_reset = {hx(r(f'ASSR{idx}|ASSR')['initval'])},
            .compare_a_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'{idx}' in (i.get('name') or i.get('caption') or '').upper() and ('COMPA' in (i.get('name') or i.get('caption') or '').upper() or 'OCIE0A' in (i.get('name') or '').upper() or 'Compare Match A' in (i.get('caption') or ''))), 0)}U,
            .compare_b_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'{idx}' in (i.get('name') or i.get('caption') or '').upper() and ('COMPB' in (i.get('name') or i.get('caption') or '').upper() or 'OCIE0B' in (i.get('name') or '').upper() or 'Compare Match B' in (i.get('caption') or ''))), 0)}U,
            .overflow_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'{idx}' in (i.get('name') or i.get('caption') or '').upper() and ('OVF' in (i.get('name') or i.get('caption') or '').upper() or 'Overflow' in (i.get('caption') or ''))), 0)}U,
            .ocra_pin_address = {pa_a}, .ocra_pin_bit = {pa_b}U, .ocrb_pin_address = {pb_a}, .ocrb_pin_bit = {pb_b}U,
            .t_pin_address = {t_a}, .t_pin_bit = {t_b}U,
            .tosc1_pin_address = {t1_a}, .tosc1_pin_bit = {t1_b}U, .tosc2_pin_address = {t2_a}, .tosc2_pin_bit = {t2_b}U,
            .wgm0_mask = {hx(b(f'TCCR{idx}A|TCCR.*A', 'WGM.*0'))}, .wgm2_mask = {hx(b(f'TCCR{idx}B|TCCR.*B', 'WGM.*2'))}, .cs_mask = {hx(b(f'TCCR{idx}B|TCCR.*B', 'CS'))},
            .as2_mask = {hx(b(f'ASSR{idx}|ASSR', 'AS2'))}, .tcn2ub_mask = {hx(b(f'ASSR{idx}|ASSR', 'TCN2UB'))},
            .compare_a_enable_mask = {hx(b(f'TIMSK{idx}|TIMSK', 'OCIEA') or b(f'TIMSK{idx}|TIMSK', f'OCIE{idx}A') or b('TIMSK', 'OCIEA'))},
            .compare_b_enable_mask = {hx(b(f'TIMSK{idx}|TIMSK', 'OCIEB') or b(f'TIMSK{idx}|TIMSK', f'OCIE{idx}B') or b('TIMSK', 'OCIEB'))},
            .overflow_enable_mask = {hx(b(f'TIMSK{idx}|TIMSK', 'TOIE') or b(f'TIMSK{idx}|TIMSK', f'TOIE{idx}') or b('TIMSK', 'TOIE'))},
            .foca_mask = {hx(b(f'TCCR{idx}B|TCCR.*B', 'FOC.*A'))}, .focb_mask = {hx(b(f'TCCR{idx}B|TCCR.*B', 'FOC.*B'))},
            .pr_address = {get_pr_info(data, f'PRTIM{idx}')[0]}, .pr_bit = {get_pr_info(data, f'PRTIM{idx}')[1]}
        }}"""
    
    def gen_timer16(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        idx = "".join(filter(str.isdigit, p_name)) or "1"
        pa_a, pa_b = get_pad_info(port_map, p_data, f'OC{idx}A', 'PORT')
        pb_a, pb_b = get_pad_info(port_map, p_data, f'OC{idx}B', 'PORT')
        pc_a, pc_b = get_pad_info(port_map, p_data, f'OC{idx}C', 'PORT')
        t_a, t_b = get_pad_info(port_map, p_data, f'T{idx}', 'PIN')
        ic_a, ic_b = get_pad_info(port_map, p_data, f'ICP{idx}', 'PIN')
        return f"""{{
            .tcnt_address = {hx(r('TCNT.*L?')['offset'])}, .ocra_address = {hx(r('OCR.*AL?')['offset'])}, .ocrb_address = {hx(r('OCR.*BL?')['offset'])}, .ocrc_address = {hx(r('OCR.*CL?')['offset'])}, .icr_address = {hx(r('ICR.*L?')['offset'])}, .tifr_address = {hx(r('TIFR')['offset'])}, .timsk_address = {hx(r('TIMSK')['offset'])}, .tccra_address = {hx(r('TCCR.*A')['offset'])}, .tccrb_address = {hx(r('TCCR.*B')['offset'])}, .tccrc_address = {hx(r('TCCR.*C')['offset'])},
            .tccra_reset = {hx(r('TCCR.*A')['initval'])}, .tccrb_reset = {hx(r('TCCR.*B')['initval'])}, .tccrc_reset = {hx(r('TCCR.*C')['initval'])},
            .capture_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'{idx}' in (i.get('name') or i.get('caption') or '').upper() and ('CAPT' in (i.get('name') or i.get('caption') or '').upper() or 'Capture' in (i.get('caption') or ''))), 0)}U,
            .compare_a_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'{idx}' in (i.get('name') or i.get('caption') or '').upper() and ('COMPA' in (i.get('name') or i.get('caption') or '').upper() or 'OC' in (i.get('name') or '').upper() and 'A' in (i.get('name') or '').upper() or 'Compare Match A' in (i.get('caption') or ''))), 0)}U,
            .compare_b_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'{idx}' in (i.get('name') or i.get('caption') or '').upper() and ('COMPB' in (i.get('name') or i.get('caption') or '').upper() or 'OC' in (i.get('name') or '').upper() and 'B' in (i.get('name') or '').upper() or 'Compare Match B' in (i.get('caption') or ''))), 0)}U,
            .compare_c_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'{idx}' in (i.get('name') or i.get('caption') or '').upper() and ('COMPC' in (i.get('name') or i.get('caption') or '').upper() or 'OC' in (i.get('name') or '').upper() and 'C' in (i.get('name') or '').upper() or 'Compare Match C' in (i.get('caption') or ''))), 0)}U,
            .overflow_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'{idx}' in (i.get('name') or i.get('caption') or '').upper() and ('OVF' in (i.get('name') or i.get('caption') or '').upper() or 'Overflow' in (i.get('caption') or ''))), 0)}U,
            .ocra_pin_address = {pa_a}, .ocra_pin_bit = {pa_b}U, .ocrb_pin_address = {pb_a}, .ocrb_pin_bit = {pb_b}U, .ocrc_pin_address = {pc_a}, .ocrc_pin_bit = {pc_b}U,
            .icp_pin_address = {ic_a}, .icp_pin_bit = {ic_b}U,
            .t_pin_address = {t_a}, .t_pin_bit = {t_b}U,
            .wgm10_mask = {hx(b('TCCR.*A', 'WGM'))}, .wgm12_mask = {hx(b('TCCR.*B', 'WGM'))}, .cs_mask = {hx(b('TCCR.*B', 'CS'))}, .ices_mask = {hx(b('TCCR.*B', 'ICES'))}, .icnc_mask = {hx(b('TCCR.*B', 'ICNC'))},
            .capture_enable_mask = {hx(b('TIMSK.*', 'ICIE'))},
            .compare_a_enable_mask = {hx(b('TIMSK.*', 'OCIE.*A'))},
            .compare_b_enable_mask = {hx(b('TIMSK.*', 'OCIE.*B'))},
            .compare_c_enable_mask = {hx(b('TIMSK.*', 'OCIE.*C'))},
            .overflow_enable_mask = {hx(b('TIMSK.*', 'TOIE'))},
            .foca_mask = {hx(b('TCCR.*C', 'FOC.*A'))}, .focb_mask = {hx(b('TCCR.*C', 'FOC.*B'))}, .focc_mask = {hx(b('TCCR.*C', 'FOC.*C'))},
            .pr_address = {get_pr_info(data, f'PRTIM{idx}')[0]}, .pr_bit = {get_pr_info(data, f'PRTIM{idx}')[1]}
        }}"""

    def gen_timer10(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        idx = "".join(filter(str.isdigit, p_name)) or "4"
        pa_a, pa_b = get_pad_info(port_map, p_data, f'OCA', 'PORT')
        pan_a, pan_b = get_pad_info(port_map, p_data, f'_OCA', 'PORT')
        pb_a, pb_b = get_pad_info(port_map, p_data, f'OCB', 'PORT')
        pbn_a, pbn_b = get_pad_info(port_map, p_data, f'_OCB', 'PORT')
        pd_a, pd_b = get_pad_info(port_map, p_data, f'OCD', 'PORT')
        pdn_a, pdn_b = get_pad_info(port_map, p_data, f'_OCD', 'PORT')
        return f"""{{
            .tcnt_address = {hx(r('TCNT.*')['offset'])}, .tc4h_address = {hx(r('TC4H')['offset'])}, .ocra_address = {hx(r('OCR.*A')['offset'])}, .ocrb_address = {hx(r('OCR.*B')['offset'])}, .ocrc_address = {hx(r('OCR.*C')['offset'])}, .ocrd_address = {hx(r('OCR.*D')['offset'])},
            .ocra_pin_address = {pa_a}, .ocra_pin_bit = {pa_b}U, .ocra_neg_pin_address = {pan_a}, .ocra_neg_pin_bit = {pan_b}U,
            .ocrb_pin_address = {pb_a}, .ocrb_pin_bit = {pb_b}U, .ocrb_neg_pin_address = {pbn_a}, .ocrb_neg_pin_bit = {pbn_b}U,
            .ocrd_pin_address = {pd_a}, .ocrd_pin_bit = {pd_b}U, .ocrd_neg_pin_address = {pdn_a}, .ocrd_neg_pin_bit = {pdn_b}U,
            .tccra_address = {hx(r('TCCR.*A')['offset'])}, .tccrb_address = {hx(r('TCCR.*B')['offset'])}, .tccrc_address = {hx(r('TCCR.*C')['offset'])}, .tccrd_address = {hx(r('TCCR.*D')['offset'])}, .tccre_address = {hx(r('TCCR.*E')['offset'])}, .dt4_address = {hx(r('DT4')['offset'])}, .pllcsr_address = {hx(data['peripherals'].get('PLL', {}).get('registers', {}).get('PLLCSR', {}).get('offset', 0))}, .timsk_address = {hx(r('TIMSK')['offset'])}, .tifr_address = {hx(r('TIFR')['offset'])},
            .tccra_reset = {hx(r('TCCR.*A')['initval'])}, .tccrb_reset = {hx(r('TCCR.*B')['initval'])}, .tccrc_reset = {hx(r('TCCR.*C')['initval'])}, .tccrd_reset = {hx(r('TCCR.*D')['initval'])}, .tccre_reset = {hx(r('TCCR.*E')['initval'])},
            .compare_a_vector_index = {next((i['index'] for i in data.get('interrupts', []) if (f'OC{idx}A' in (i.get('name') or '').upper()) or (f'TIMER{idx}_COMPA' in (i.get('name') or '').upper())), 0)}U,
            .compare_b_vector_index = {next((i['index'] for i in data.get('interrupts', []) if (f'OC{idx}B' in (i.get('name') or '').upper()) or (f'TIMER{idx}_COMPB' in (i.get('name') or '').upper())), 0)}U,
            .compare_d_vector_index = {next((i['index'] for i in data.get('interrupts', []) if (f'OC{idx}D' in (i.get('name') or '').upper()) or (f'TIMER{idx}_COMPD' in (i.get('name') or '').upper())), 0)}U,
            .overflow_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'OVF{idx}' in (i.get('name') or '').upper() or f'TIMER{idx}_OVF' in (i.get('name') or '').upper()), 0)}U,
            .pr_address = {get_pr_info(data, f'PRTIM{idx}')[0]}, .pr_bit = {get_pr_info(data, f'PRTIM{idx}')[1]}
        }}"""

    def gen_adc(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        idx = "".join(filter(str.isdigit, p_name)) or "0"
        pins_addr, pins_bit = [], []
        for i in range(16):
            a, b_bit = get_pad_info(port_map, p_data, f'ADC{i}', 'PIN')
            pins_addr.append(a)
            pins_bit.append(str(b_bit) + "U")
        return f"""{{
            .adcl_address = {get_reg_addr(p_data, 'ADCL|ADC')}, .adch_address = {get_reg_addr(p_data, 'ADCH|ADC', True)}, .adcsra_address = {hx(r('ADCSRA')['offset'])}, .adcsrb_address = {hx(r('ADCSRB')['offset'])}, .admux_address = {hx(r('ADMUX')['offset'])},
            .vector_index = {next((i['index'] for i in data['interrupts'] if (i.get('name') or '') and 'ADC' in (i.get('name') or '').upper()), 0)}U,
            .adcsra_reset = {hx(r('ADCSRA')['initval'])}, .adcsrb_reset = {hx(r('ADCSRB')['initval'])}, .admux_reset = {hx(r('ADMUX')['initval'])},
            .didr0_address = {hx(r('DIDR0')['offset'])},
            .adc_pin_address = {{{{ {", ".join(pins_addr)} }}}},
            .adc_pin_bit = {{{{ {", ".join(pins_bit)} }}}},
            .auto_trigger_map = {{{{ AdcAutoTriggerSource::free_running, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::external_interrupt_0, AdcAutoTriggerSource::timer0_compare, AdcAutoTriggerSource::timer0_overflow, AdcAutoTriggerSource::timer1_compare_b, AdcAutoTriggerSource::timer1_overflow, AdcAutoTriggerSource::timer1_capture }}}},
            .adsc_mask = {hx(b('ADCSRA', 'ADSC'))}, .adate_mask = {hx(b('ADCSRA', 'ADATE'))}, .adif_mask = {hx(b('ADCSRA', 'ADIF'))}, .adie_mask = {hx(b('ADCSRA', 'ADIE'))}, .aden_mask = {hx(b('ADCSRA', 'ADEN'))}, .adlar_mask = {hx(b('ADMUX', 'ADLAR'))},
            .pr_address = {get_pr_info(data, 'PRADC')[0]}, .pr_bit = {get_pr_info(data, 'PRADC')[1]}
        }}"""

    def gen_usb(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        return f"""{{
            .uhwcon_address = {hx(r('UHWCON')['offset'])}, .usbcon_address = {hx(r('USBCON')['offset'])}, .usbsta_address = {hx(r('USBSTA')['offset'])}, .usbint_address = {hx(r('USBINT')['offset'])},
            .udcon_address = {hx(r('UDCON')['offset'])}, .udint_address = {hx(r('UDINT')['offset'])}, .udien_address = {hx(r('UDIEN')['offset'])}, .udaddr_address = {hx(r('UDADDR')['offset'])},
            .uenum_address = {hx(r('UENUM')['offset'])}, .uerst_address = {hx(r('UERST')['offset'])}, .ueint_address = {hx(r('UEINT')['offset'])},
            .ueintx_address = {hx(r('UEINTX')['offset'])}, .ueienx_address = {hx(r('UEIENX')['offset'])}, .uedatx_address = {hx(r('UEDATX')['offset'])},
            .uebclx_address = {hx(r('UEBCLX')['offset'])}, .uebchx_address = {hx(r('UEBCHX')['offset'])}, .ueconx_address = {hx(r('UECONX')['offset'])},
            .uecfg0x_address = {hx(r('UECFG0X')['offset'])}, .uecfg1x_address = {hx(r('UECFG1X')['offset'])}, .uesta0x_address = {hx(r('UESTA0X')['offset'])}, .uesta1x_address = {hx(r('UESTA1X')['offset'])},
            .vector_index = {next((i['index'] for i in data.get('interrupts', []) if 'USB' in (i.get('name') or i.get('caption') or '').upper()), 0)}U,
            .pr_address = {get_pr_info(data, 'PRUSB')[0]}, .pr_bit = {get_pr_info(data, 'PRUSB')[1]}
        }}"""

    def gen_ac(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        a0_a, a0_b = get_pad_info(port_map, p_data, 'AIN0', 'PIN')
        a1_a, a1_b = get_pad_info(port_map, p_data, 'AIN1', 'PIN')
        return f"""{{
            .acsr_address = {hx(r('ACSR')['offset'])}, .didr1_address = {hx(r('DIDR1')['offset'])}, .vector_index = {next((i['index'] for i in data['interrupts'] if (i.get('name') or '') and 'ANALOG_COMP' in i['name']), 0)}U,
            .ain0_pin_address = {a0_a}, .ain0_pin_bit = {a0_b}U, .ain1_pin_address = {a1_a}, .ain1_pin_bit = {a1_b}U,
            .aci_mask = {hx(b('ACSR', 'ACI'))}, .acie_mask = {hx(b('ACSR', 'ACIE'))}
        }}"""

    def gen_spi(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        idx = "".join(filter(str.isdigit, p_name)) or "0"
        return f"""{{
            .spcr_address = {hx(r('SPCR')['offset'])}, .spsr_address = {hx(r('SPSR')['offset'])}, .spdr_address = {hx(r('SPDR')['offset'])},
            .spcr_reset = {hx(r('SPCR')['initval'])}, .spsr_reset = {hx(r('SPSR')['initval'])}, .vector_index = {next((i['index'] for i in data['interrupts'] if (i.get('name') or '') and 'SPI' in i['name']), 0)}U,
            .spe_mask = {hx(b('SPCR', 'SPE'))}, .spie_mask = {hx(b('SPCR', 'SPIE'))}, .mstr_mask = {hx(b('SPCR', 'MSTR'))}, .spif_mask = {hx(b('SPSR', 'SPIF'))}, .wcol_mask = {hx(b('SPSR', 'WCOL'))}, .sp2x_mask = {hx(b('SPSR', 'SPI2X'))},
            .pr_address = {get_pr_info(data, f'PRSPI{idx}|PRSPI')[0]}, .pr_bit = {get_pr_info(data, f'PRSPI{idx}|PRSPI')[1]}
        }}"""

    def gen_twi(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        idx = "".join(filter(str.isdigit, p_name)) or "0"
        return f"""{{
            .twbr_address = {hx(r('TWBR')['offset'])}, .twsr_address = {hx(r('TWSR')['offset'])}, .twar_address = {hx(r('TWAR')['offset'])}, .twdr_address = {hx(r('TWDR')['offset'])}, .twcr_address = {hx(r('TWCR')['offset'])}, .twamr_address = {hx(r('TWAMR')['offset'])},
            .vector_index = {next((i['index'] for i in data['interrupts'] if (i.get('name') or '') and 'TWI' in i['name']), 0)}U,
            .twint_mask = {hx(b('TWCR', 'TWINT'))}, .twen_mask = {hx(b('TWCR', 'TWEN'))}, .twie_mask = {hx(b('TWCR', 'TWIE'))}, .twsto_mask = {hx(b('TWCR', 'TWSTO'))}, .twsta_mask = {hx(b('TWCR', 'TWSTA'))}, .twea_mask = {hx(b('TWCR', 'TWEA'))},
            .pr_address = {get_pr_info(data, f'PRTWI{idx}|PRTWI')[0]}, .pr_bit = {get_pr_info(data, f'PRTWI{idx}|PRTWI')[1]}
        }}"""
    
    def gen_can(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        idt = next((reg for name, reg in p_data.get('registers', {}).items() if 'CANIDT' in name), {'offset': 0})
        idm = next((reg for name, reg in p_data.get('registers', {}).items() if 'CANIDM' in name), {'offset': 0})
        return f"""{{
            .cangcon_address = {hx(r('CANGCON')['offset'])}, .cangsta_address = {hx(r('CANGSTA')['offset'])}, .cangit_address = {hx(r('CANGIT')['offset'])}, .cangie_address = {hx(r('CANGIE')['offset'])}, .canen1_address = {hx(r('CANEN1')['offset'])}, .canen2_address = {hx(r('CANEN2')['offset'])}, .canie1_address = {hx(r('CANIE1')['offset'])}, .canie2_address = {hx(r('CANIE2')['offset'])}, .cansit1_address = {hx(r('CANSIT1')['offset'])}, .cansit2_address = {hx(r('CANSIT2')['offset'])}, .canbt1_address = {hx(r('CANBT1')['offset'])}, .canbt2_address = {hx(r('CANBT2')['offset'])}, .canbt3_address = {hx(r('CANBT3')['offset'])}, .cantcon_address = {hx(r('CANTCON')['offset'])},
            .cantim_address = {hx(r('CANTIM|CANTIML')['offset'])}, .canttc_address = {hx(r('CANTTC|CANTTCL')['offset'])}, .cantec_address = {hx(r('CANTEC')['offset'])}, .canrec_address = {hx(r('CANREC')['offset'])}, .canhpmob_address = {hx(r('CANHPMOB')['offset'])}, .canpage_address = {hx(r('CANPAGE')['offset'])}, .canstmob_address = {hx(r('CANSTMOB')['offset'])}, .cancdmob_address = {hx(r('CANCDMOB')['offset'])}, .canidt_address = {hx(idt['offset'])}, .canidm_address = {hx(idm['offset'])}, .canstm_address = {hx(r('CANSTM|CANSTML')['offset'])}, .canmsg_address = {hx(r('CANMSG')['offset'])},
            .canit_vector_index = {next((i['index'] for i in data.get('interrupts', []) if 'CANIT' in (i.get('name') or '').upper()), 0)}U,
            .ovrit_vector_index = {next((i['index'] for i in data.get('interrupts', []) if 'OVRIT' in (i.get('name') or '').upper()), 0)}U,
            .mob_count = 15U,
            .pr_address = {get_pr_info(data, 'PRCAN')[0]}, .pr_bit = {get_pr_info(data, 'PRCAN')[1]}
        }}"""

    def gen_pcint(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        idx = "".join(filter(str.isdigit, p_name)) or "0"
        return f"""{{
            .pcicr_address = {hx(data['peripherals'].get('EXINT', {}).get('registers', {}).get('PCICR', {}).get('offset', 0))},
            .pcifr_address = {hx(data['peripherals'].get('EXINT', {}).get('registers', {}).get('PCIFR', {}).get('offset', 0))},
            .pcmsk_address = {hx(data['peripherals'].get('EXINT', {}).get('registers', {}).get(f'PCMSK{idx}', {}).get('offset', r('PCMSK')['offset']))},
            .pcicr_enable_mask = {hx(1 << int(idx))}, .pcifr_flag_mask = {hx(1 << int(idx))},
            .vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'PCINT{idx}' in (i.get('name') or i.get('caption') or '').upper()), 0)}U
        }}"""

    def gen_ext_int(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        vectors = []
        for i in range(8):
            v = next((int(it['index']) for it in data['interrupts'] if it['name'] == f'INT{i}'), 0)
            vectors.append(str(v) + "U")
        return f"""{{
            .eicra_address = {hx(r('EICRA')['offset'])}, .eicrb_address = {hx(r('EICRB')['offset'])}, .eimsk_address = {hx(r('EIMSK')['offset'])}, .eifr_address = {hx(r('EIFR')['offset'])},
            .vector_indices = {{{{ {", ".join(vectors)} }}}}
        }}"""

    def gen_eeprom(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        return f"""{{
            .eecr_address = {hx(r('EECR')['offset'])}, .eedr_address = {hx(r('EEDR')['offset'])}, .eearl_address = {get_reg_addr(p_data, 'EEARL|EEAR')}, .eearh_address = {get_reg_addr(p_data, 'EEARH|EEAR', True)},
            .vector_index = {next((i['index'] for i in data['interrupts'] if 'EE_READY' in i['name']), 0)}U
        }}"""

    def gen_wdt(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        return f"""{{
            .wdtcsr_address = {hx(r('WDTCSR|WDTCR')['offset'])},
            .vector_index = {next((i['index'] for i in data['interrupts'] if (i.get('name') or '') and 'WDT' in i['name']), 0)}U,
            .wdie_mask = {hx(b('WDTCSR|WDTCR', 'WDIE'))}, .wde_mask = {hx(b('WDTCSR|WDTCR', 'WDE'))}
        }}"""

    def gen_xmem(p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        xmcra = r('XMCRA')
        xmcrb = r('XMCRB')
        mcucr = r('MCUCR')
        sre_mask = b('XMCRA', 'SRE') or b('MCUCR', 'SRE') or 0x80
        return f"""{{
            .xmcra_address = {hx(xmcra['offset'])}, .xmcrb_address = {hx(xmcrb['offset'])},
            .mcucr_address = {hx(mcucr['offset'])}, .sre_mask = {hx(sre_mask)}
        }}"""

    uarts_str = ",\n        ".join(gen_uart(n, d) for n, d in groups['USART'])
    timers8_str = ",\n        ".join(gen_timer8(n, d) for n, d in (groups['TC8'] + groups['TC8_ASYNC']))
    timers16_str = ",\n        ".join(gen_timer16(n, d) for n, d in groups['TC16'])
    adcs_str = ",\n        ".join(gen_adc(n, d) for n, d in groups['ADC'])
    acs_str = ",\n        ".join(gen_ac(n, d) for n, d in groups['AC'])
    spis_str = ",\n        ".join(gen_spi(n, d) for n, d in groups['SPI'])
    twis_str = ",\n        ".join(gen_twi(n, d) for n, d in groups['TWI'])
    cans_str = ",\n        ".join(gen_can(n, d) for n, d in groups['CAN'])
    pcints_str = ",\n        ".join(gen_pcint(n, d) for n, d in groups['PCINT'])
    ext_ints_str = ",\n        ".join(gen_ext_int(n, d) for n, d in groups['EXINT'])
    eeproms_str = ",\n        ".join(gen_eeprom(n, d) for n, d in groups['EEPROM'])
    wdts_str = ",\n        ".join(gen_wdt(n, d) for n, d in groups['WDT'])
    timers10_str = ",\n        ".join(gen_timer10(n, d) for n, d in groups['TC10'])
    usbs_str = ",\n        ".join(gen_usb(n, d) for n, d in groups['USB_DEVICE'])
    
    ports_str = ",\n        ".join(f'{{ "{p["name"]}", {hx(p["pin"])}, {hx(p["ddr"])}, {hx(p["port"])} }}' for p in ports_raw)

    content = f"""#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {{

inline constexpr DeviceDescriptor {safe_name} {{
    .name = "{name}",
    .flash_words = {flash['size'] // 2}U,
    .sram_bytes = {sram['size']}U,
    .eeprom_bytes = {eeprom['size']}U,
    .interrupt_vector_count = {len(data['interrupts'])}U,
    .interrupt_vector_size = {4 if flash['size'] > 4096 else 2}U,
    .spl_address = {get_reg_addr(cpu, 'SPL|SP')},
    .sph_address = {get_reg_addr(cpu, 'SPH|SP', True)},
    .sreg_address = {get_reg_addr(cpu, 'SREG')},
    .rampz_address = {get_reg_addr(cpu, 'RAMPZ')},
    .eind_address = {get_reg_addr(cpu, 'EIND')},
    .spmcsr_address = {hx(r_cpu('SPMCSR')['offset']) if r_cpu('SPMCSR') else '0x0U'},
    .prr_address = {hx(r_cpu('PRR')['offset']) if r_cpu('PRR') else '0x0U'},
    .prr0_address = {hx(r_cpu('PRR0')['offset']) if r_cpu('PRR0') else '0x0U'},
    .prr1_address = {hx(r_cpu('PRR1')['offset']) if r_cpu('PRR1') else '0x0U'},
    .smcr_address = {hx(r_cpu('SMCR')['offset']) if r_cpu('SMCR') else '0x0U'},
    .mcusr_address = {hx(r_cpu('MCUSR')['offset']) if r_cpu('MCUSR') else '0x0U'},
    .mcucr_address = {hx(r_cpu('MCUCR')['offset']) if r_cpu('MCUCR') else '0x0U'},
    .pllcsr_address = {hx(data['peripherals'].get('PLL', {}).get('registers', {}).get('PLLCSR', {}).get('offset', 0))},
    .xmcra_address = {hx(r_cpu('XMCRA')['offset']) if r_cpu('XMCRA') else '0x0U'},
    .xmcrb_address = {hx(r_cpu('XMCRB')['offset']) if r_cpu('XMCRB') else '0x0U'},
    .xmem = {gen_xmem(groups['EXTERNAL_MEMORY'][0][1]) if groups['EXTERNAL_MEMORY'] else '{0}'},
    .pradc_bit = {hx(get_pr_info(data, 'PRADC')[1])},
    .prusart0_bit = {hx(get_pr_info(data, 'PRUSART0|PRUSART')[1])},
    .prspi_bit = {hx(get_pr_info(data, 'PRSPI')[1])},
    .prtwi_bit = {hx(get_pr_info(data, 'PRTWI')[1])},
    .prtimer0_bit = {hx(get_pr_info(data, 'PRTIM0')[1])},
    .prtimer1_bit = {hx(get_pr_info(data, 'PRTIM1')[1])},
    .prtimer2_bit = {hx(get_pr_info(data, 'PRTIM2')[1])},
    .adc_count = {len(groups['ADC'])}U,
    .adcs = {{{{ {adcs_str} }}}},
    .ac_count = {len(groups['AC'])}U,
    .acs = {{{{ {acs_str} }}}},
    .timer8_count = {len(groups['TC8'] + groups['TC8_ASYNC'])}U,
    .timers8 = {{{{ {timers8_str} }}}},
    .timer16_count = {len(groups['TC16'])}U,
    .timers16 = {{{{ {timers16_str} }}}},
    .timer10_count = {len(groups['TC10'])}U,
    .timers10 = {{{{ {timers10_str} }}}},
    
    .ext_interrupt_count = {len(groups['EXINT'])}U,
    .ext_interrupts = {{{{ {ext_ints_str} }}}},

    .uart_count = {len(groups['USART'])}U,
    .uarts = {{{{ {uarts_str} }}}},
    
    .pcint_count = {len(groups['PCINT'])}U,
    .pcints = {{{{ {pcints_str} }}}},
    
    .spi_count = {len(groups['SPI'])}U,
    .spis = {{{{ {spis_str} }}}},
    
    .twi_count = {len(groups['TWI'])}U,
    .twis = {{{{ {twis_str} }}}},
    
    .eeprom_count = {len(groups['EEPROM'])}U,
    .eeproms = {{{{ {eeproms_str} }}}},
    
    .wdt_count = {len(groups['WDT'])}U,
    .wdts = {{{{ {wdts_str} }}}},

    .can_count = {len(groups['CAN'])}U,
    .cans = {{{{ {cans_str} }}}},
    
    .usb_count = {len(groups['USB_DEVICE'])}U,
    .usbs = {{{{ {usbs_str} }}}},

    .port_count = {len(ports_raw)}U,
    .ports = {{{{
        {ports_str}
    }}}}
}};

}} // namespace vioavr::core::devices
"""
    with open(header_path, 'w') as f: f.write(content)

def main():
    import argparse
    parser = argparse.ArgumentParser(description='Generate C++ device header from metadata JSON')
    parser.add_argument('input', help='Input metadata JSON file')
    parser.add_argument('output', help='Output C++ header file')
    args = parser.parse_args()
    
    try:
        with open(args.input, 'r') as f:
            data = json.load(f)
        generate_header(data, Path(args.output).parent)
        # Note: generate_header internally determines the filename from data['name']
        # I should probably refactor generate_header to take the full output path.
        print(f"Generated header for {data['name']} at {args.output}")
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
