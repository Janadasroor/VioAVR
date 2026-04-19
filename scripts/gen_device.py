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
    # Prioritize exact match
    p_exact = re.compile(f"^(?:{name_regex})$", re.IGNORECASE)
    for reg_name, reg_data in periph.get('registers', {}).items():
        if p_exact.fullmatch(reg_name):
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
    p = re.compile(f"^(?:{bit_regex})$", re.IGNORECASE)
    cpu = data['peripherals'].get('CPU', data['peripherals'].get('CPU_REGISTERS', {}))
    for r_name in ['PRR', 'PRR0', 'PRR1', 'PRR2']:
        reg = get_reg(cpu, r_name)
        if reg:
            for b_name, b_data in reg.get('bitfields', {}).items():
                if p.fullmatch(b_name) or p.search(b_name):
                    # Convert mask to bit index
                    mask = b_data['mask']
                    bit_idx = 0
                    while mask > 1:
                        mask >>= 1
                        bit_idx += 1
                    return reg['offset'], bit_idx
    return 0, 0xFF

def get_mapping(data, mem_type):
    for m in data['memories'].get('data', []):
        if m['type'] == mem_type or (mem_type == 'flash' and m['name'] == 'MAPPED_PROGMEM'):
             return hx(m['start']), hx(m['size'])
    return "0x0U", "0x0U"

def generate_header(data, header_path):
    name = data['name']
    safe_name = name.lower().replace('-', '_')
    
    # 1. Build Port Map for PinMux
    port_map = {}
    ports_raw = []
    for p_instance, p_data in data['peripherals'].items():
        if p_data.get('module') == 'PORT':
            char = p_instance[-1] # Usually PORTA, PORTB etc.
            if char not in port_map: port_map[char] = {}
            for r_name, r_data in p_data['registers'].items():
                if r_name == 'DIR' or r_name == f'DDR{char}':
                    port_map[char]['DDR'] = r_data['offset']
                if r_name == 'OUT' or r_name == f'PORT{char}':
                    port_map[char]['PORT'] = r_data['offset']
                if r_name == 'IN' or r_name == f'PIN{char}':
                    port_map[char]['PIN'] = r_data['offset']
    
    for char, regs in port_map.items():
        if not regs: continue
        ports_raw.append({
            'name': f"PORT{char}",
            'port': regs.get('PORT', 0),
            'ddr': regs.get('DDR', 0),
            'pin': regs.get('PIN', 0)
        })
    ports_raw.sort(key=lambda x: x['name'])

    # 2. Group Peripherals
    groups = {
        'USART': [], 'USART8X': [], 'TC8': [], 'TC8_ASYNC': [], 'TC16': [], 'ADC': [], 'AC': [],
        'WDT': [], 'WDT8X': [], 'EEPROM': [], 'SPI': [], 'SPI8X': [], 'TWI': [], 'TWI8X': [], 'EXINT': [], 'PCINT': [],
        'CAN': [], 'EXTERNAL_MEMORY': [], 'TC10': [], 'USB_DEVICE': [], 'PSC': [], 'DAC': [],
        'NVMCTRL': [], 'CPUINT': [], 'TCA': [], 'TCB': [], 'RTC': [], 'EVSYS': [], 'CCL': [], 'CRC': [], 'CRCSCAN': [],
        'PORTMUX': []
    }
    event_generators = {}
    
    # 1. First pass: Find EVSYS to get generator mappings
    evsys_module = None
    for p_name, p_data in data['peripherals'].items():
        if 'EVSYS' in p_name:
            evsys_module = p_data
            break
    
    if evsys_module:
        for reg_name, reg_def in evsys_module['registers'].items():
            if 'CHANNEL' in reg_name:
                for bf_name, bf_def in reg_def.get('bitfields', {}).items():
                    if 'GENERATOR' in bf_name.upper():
                        for val_name, val_def in bf_def.get('values', {}).items():
                            event_generators[val_name] = val_def['value']
                        break
                break

    for p_name, p_data in data['peripherals'].items():
        mod = p_data.get('module')
        if not mod: continue
        
        # Dispatch to groups based on module and register names
        target_mod = mod
        if mod == 'USART':
            if 'RXDATAL' in p_data['registers']: target_mod = 'USART8X'
        elif mod == 'SPI':
            if 'INTFLAGS' in p_data['registers']: target_mod = 'SPI8X'
        elif mod == 'TWI':
            if 'MSTATUS' in p_data['registers']: target_mod = 'TWI8X'
        elif mod == 'WDT':
            if 'CTRLA' in p_data['registers']: target_mod = 'WDT8X'
            
        if target_mod in groups:
            groups[target_mod].append((p_name, p_data))
            if target_mod == 'EXINT':
                for k in p_data.get('registers', {}).keys():
                    if k.startswith('PCMSK'):
                        idx = k[5:]
                        groups['PCINT'].append((f'PCINT{idx}', p_data))
        elif 'PCINT' in p_name or 'PCINT' == mod:
            groups['PCINT'].append((p_name, p_data))
        elif mod.startswith('TC10'):
            groups['TC10'].append((p_name, p_data))
        elif 'PSC' in mod:
            groups['PSC'].append((p_name, p_data))
        elif 'DAC' in mod:
            groups['DAC'].append((p_name, p_data))

    for k in groups: groups[k].sort()

    # 3. Memories
    sram = next((m for m in data['memories'].get('ram', []) + data['memories'].get('data', []) if m['type'] == 'ram'), {'size': 0, 'start': 0})
    sram_bytes = sram['size']
    sram_start = sram['start']
    
    flash = next((m for m in data['memories'].get('prog', []) if m['type'] == 'flash'), {'size': 0})
    eeprom_search = data['memories'].get('eeprom', [])
    if not eeprom_search:
        eeprom_search = [m for m in data['memories'].get('data', []) if m['type'] == 'eeprom']
    eeprom = eeprom_search[0] if eeprom_search else {'size': 0}
    
    # 4. Core Regs
    cpu = data['peripherals'].get('CPU', data['peripherals'].get('CPU_REGISTERS', {}))
    regs = cpu.get('registers', {})
    r_cpu = lambda n: get_reg(cpu, n)
    b_cpu = lambda n, b_re: get_bit(r_cpu(n), b_re)
    gen_pllcsr = lambda: hx((get_reg(data['peripherals'].get('PLL', {}), 'PLLCSR') or r_cpu('PLLCSR') or {'offset': 0})['offset'])
    
    # 5. Header Content Generation
    def gen_uart(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        idx = "".join(filter(str.isdigit, p_name)) or "0"
        
        # Mapping for MegaAVR vs AVR8X
        udr_addr = r(f'UDR{idx}|UDR|RXDATAL|TXDATAL|DATA')['offset']
        status_addr = r(f'UCSR{idx}A|UCSRA|STATUS')['offset']
        ctrl_ie_addr = r(f'UCSR{idx}B|UCSRB|CTRLA')['offset']
        ctrl_en_addr = r(f'UCSR{idx}B|UCSRB|CTRLB')['offset']
        ctrl_mode_addr = r(f'UCSR{idx}C|UCSRC|CTRLC')['offset']
        ubrr_addr = r(f'UBRR{idx}|UBRR|BAUD')['offset']
        
        pr_addr, pr_bit = get_pr_info(data, f'PRUSART{idx}|PRUSART')
        
        # Vector search
        def find_vec(tokens):
            if isinstance(tokens, str): tokens = [tokens]
            for token in tokens:
                for i in data.get('interrupts', []):
                    name = (i.get('name') or i.get('caption') or '').upper()
                    if token in name:
                        module = (i.get('module-instance') or '').upper()
                        if p_name in name or p_name == module or (('USART' in name or 'UART' in name) and ('USART' in p_name or 'UART' in p_name)):
                             return i['index']
            return 0

        return f"""{{
            .udr_address = {hx(udr_addr)}, .ucsra_address = {hx(status_addr)}, .ucsrb_address = {hx(ctrl_ie_addr)}, .ucsrc_address = {hx(ctrl_en_addr)}, .ubrrl_address = {hx(ubrr_addr)}, .ubrrh_address = {hx(ubrr_addr + 1)},
            .ucsra_reset = {hx(r(f'UCSR{idx}A|UCSRA|STATUS')['initval'])}, .ucsrb_reset = {hx(r(f'UCSR{idx}B|UCSRB|CTRLA')['initval'])}, .ucsrc_reset = {hx(r(f'UCSR{idx}B|UCSRB|CTRLB')['initval'])},
            .rx_vector_index = {find_vec(['RXC', 'RX'])}U,
            .udre_vector_index = {find_vec(['DRE', 'UDRE'])}U,
            .tx_vector_index = {find_vec(['TXC', 'TX'])}U,
            .u2x_mask = {hx(b(f'UCSR{idx}A|UCSRA|STATUS', 'U2X') or b(f'UCSR{idx}B|UCSRB|CTRLB', 'RXMODE.*CLK2X'))}, 
            .rxc_mask = {hx(b(f'UCSR{idx}A|UCSRA|STATUS', 'RXC'))}, 
            .txc_mask = {hx(b(f'UCSR{idx}A|UCSRA|STATUS', 'TXC'))}, 
            .udre_mask = {hx(b(f'UCSR{idx}A|UCSRA|STATUS', 'UDRE|DRE'))},
            .rxen_mask = {hx(b(f'UCSR{idx}B|UCSRB|CTRLB', 'RXEN'))}, 
            .txen_mask = {hx(b(f'UCSR{idx}B|UCSRB|CTRLB', 'TXEN'))}, 
            .rxcie_mask = {hx(b(f'UCSR{idx}B|UCSRB|CTRLA', 'RXCIE'))}, 
            .txcie_mask = {hx(b(f'UCSR{idx}B|UCSRB|CTRLA', 'TXCIE'))}, 
            .udrie_mask = {hx(b(f'UCSR{idx}B|UCSRB|CTRLA', 'UDRIE|DREIE'))},
            .pr_address = {pr_addr}, .pr_bit = {pr_bit},
            .uart_index = {idx}U
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
            .as2_mask = {hx(b(f'ASSR{idx}|ASSR', 'AS2'))}, .exclk_mask = {hx(b(f'ASSR{idx}|ASSR', 'EXCLK'))}, .tcn2ub_mask = {hx(b(f'ASSR{idx}|ASSR', 'TCN.*UB'))},
            .ocr2aub_mask = {hx(b(f'ASSR{idx}|ASSR', 'OCR.*AUB'))}, .ocr2bub_mask = {hx(b(f'ASSR{idx}|ASSR', 'OCR.*BUB'))},
            .tcr2aub_mask = {hx(b(f'ASSR{idx}|ASSR', 'TCR.*AUB'))}, .tcr2bub_mask = {hx(b(f'ASSR{idx}|ASSR', 'TCR.*BUB'))},
            .compare_a_enable_mask = {hx(b(f'TIMSK{idx}|TIMSK', 'OCIEA') or b(f'TIMSK{idx}|TIMSK', f'OCIE{idx}A') or b('TIMSK', 'OCIEA'))},
            .compare_b_enable_mask = {hx(b(f'TIMSK{idx}|TIMSK', 'OCIEB') or b(f'TIMSK{idx}|TIMSK', f'OCIE{idx}B') or b('TIMSK', 'OCIEB'))},
            .overflow_enable_mask = {hx(b(f'TIMSK{idx}|TIMSK', 'TOIE') or b(f'TIMSK{idx}|TIMSK', f'TOIE{idx}') or b('TIMSK', 'TOIE'))},
            .foca_mask = {hx(b(f'TCCR{idx}B|TCCR.*B', 'FOC.*A'))}, .focb_mask = {hx(b(f'TCCR{idx}B|TCCR.*B', 'FOC.*B'))},
            .pr_address = {get_pr_info(data, f'PRTIM{idx}')[0]}, .pr_bit = {get_pr_info(data, f'PRTIM{idx}')[1]},
            .timer_index = {idx}U,
            .compare_a_trigger_source = AdcAutoTriggerSource::timer{idx}_compare_a,
            .compare_b_trigger_source = AdcAutoTriggerSource::timer{idx}_compare_b,
            .overflow_trigger_source = AdcAutoTriggerSource::timer{idx}_overflow
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
            .tcnt_address = {hx(r('TCNT.*L?')['offset'])}, .ocra_address = {hx(r('OCR.*AL?')['offset'])}, .ocrb_address = {hx(r('OCR.*BL?')['offset'])}, .ocrc_address = {hx(r('OCR.*CL?')['offset'])}, .icr_address = {hx(r('ICR.*L?')['offset'])}, .tifr_address = {hx(r('TIFR.*')['offset'])}, .timsk_address = {hx(r('TIMSK.*')['offset'])}, .tccra_address = {hx(r('TCCR.*A')['offset'])}, .tccrb_address = {hx(r('TCCR.*B')['offset'])}, .tccrc_address = {hx(r('TCCR.*C')['offset'])},
            .tccra_reset = {hx(r('TCCR.*A')['initval'])}, .tccrb_reset = {hx(r('TCCR.*B')['initval'])}, .tccrc_reset = {hx(r('TCCR.*C')['initval'])},
            .timer_index = {idx}U,
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
            .pr_address = {get_pr_info(data, f'PRTIM{idx}')[0]}, .pr_bit = {get_pr_info(data, f'PRTIM{idx}')[1]},
            .compare_a_trigger_source = AdcAutoTriggerSource::timer{idx}_compare_a,
            .compare_b_trigger_source = AdcAutoTriggerSource::timer{idx}_compare_b,
            .compare_c_trigger_source = AdcAutoTriggerSource::timer{idx}_compare_c,
            .overflow_trigger_source = AdcAutoTriggerSource::timer{idx}_overflow,
            .capture_trigger_source = AdcAutoTriggerSource::timer{idx}_capture
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
            .tccra_address = {hx(r('TCCR.*A')['offset'])}, .tccrb_address = {hx(r('TCCR.*B')['offset'])}, .tccrc_address = {hx(r('TCCR.*C')['offset'])}, .tccrd_address = {hx(r('TCCR.*D')['offset'])}, .tccre_address = {hx(r('TCCR.*E')['offset'])}, .dt4_address = {hx(r('DT4')['offset'])}, .pllcsr_address = {gen_pllcsr()}, .timsk_address = {hx(r('TIMSK.*')['offset'])}, .tifr_address = {hx(r('TIFR.*')['offset'])},
            .tccra_reset = {hx(r('TCCR.*A')['initval'])}, .tccrb_reset = {hx(r('TCCR.*B')['initval'])}, .tccrc_reset = {hx(r('TCCR.*C')['initval'])}, .tccrd_reset = {hx(r('TCCR.*D')['initval'])}, .tccre_reset = {hx(r('TCCR.*E')['initval'])},
            .compare_a_vector_index = {next((i['index'] for i in data.get('interrupts', []) if (f'OC{idx}A' in (i.get('name') or '').upper()) or (f'TIMER{idx}_COMPA' in (i.get('name') or '').upper())), 0)}U,
            .compare_b_vector_index = {next((i['index'] for i in data.get('interrupts', []) if (f'OC{idx}B' in (i.get('name') or '').upper()) or (f'TIMER{idx}_COMPB' in (i.get('name') or '').upper())), 0)}U,
            .compare_d_vector_index = {next((i['index'] for i in data.get('interrupts', []) if (f'OC{idx}D' in (i.get('name') or '').upper()) or (f'TIMER{idx}_COMPD' in (i.get('name') or '').upper())), 0)}U,
            .overflow_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'OVF{idx}' in (i.get('name') or '').upper() or f'TIMER{idx}_OVF' in (i.get('name') or '').upper()), 0)}U,
            .pr_address = {get_pr_info(data, f'PRTIM{idx}')[0]}, .pr_bit = {get_pr_info(data, f'PRTIM{idx}')[1]},
            .compare_a_trigger_source = {"AdcAutoTriggerSource::timer4_compare_a" if idx == "4" else "AdcAutoTriggerSource::none"},
            .compare_b_trigger_source = {"AdcAutoTriggerSource::timer4_compare_b" if idx == "4" else "AdcAutoTriggerSource::none"},
            .compare_d_trigger_source = {"AdcAutoTriggerSource::timer4_compare_d" if idx == "4" else "AdcAutoTriggerSource::none"},
            .overflow_trigger_source = {"AdcAutoTriggerSource::timer4_overflow" if idx == "4" else "AdcAutoTriggerSource::none"}
        }}"""

    def gen_adc_mux_table(mcu_name):
        entries = ["{0xFFU, 0, 1.0f, false}"] * 64
        # Single-ended standard 0-7
        for i in range(8): entries[i] = f"{{ {i}, 0, 1.0f, false }}"
        
        # 32U4 Specifics
        if "32U4" in mcu_name:
            # ADC8-13 (paged via MUX5)
            for i in range(6): entries[0x20+i] = f"{{ {8+i}, 0, 1.0f, false }}"
            # Internal
            entries[0x1E] = "{ 14, 0, 1.0f, false }" # Vbg
            entries[0x1F] = "{ 15, 0, 1.0f, false }" # GND
            entries[0x27] = "{ 13, 0, 1.0f, false }" # Temp
            # Differential (Simplified subset from datasheet)
            diffs = {
                0x08: (0, 1, 10.0), 0x09: (0, 1, 40.0), 0x0A: (1, 1, 10.0), 0x0B: (1, 1, 40.0),
                0x0C: (4, 1, 10.0), 0x0D: (4, 1, 40.0), 0x0E: (5, 1, 10.0), 0x0F: (5, 1, 40.0),
                0x10: (6, 1, 10.0), 0x11: (6, 1, 40.0), 0x12: (2, 1, 10.0), 0x13: (3, 1, 10.0),
                0x14: (2, 1, 40.0), 0x15: (3, 1, 40.0),
                0x28: (0, 1, 200.0), 0x29: (0, 1, 200.0), 0x2A: (1, 1, 200.0),
                0x2C: (4, 5, 10.0), 0x2D: (4, 5, 40.0), 0x2E: (4, 5, 200.0)
            }
            for mux, (p, n, g) in diffs.items():
                entries[mux] = f"{{ {p}, {n}, {g}f, true }}"
        
        # AT90PWM Specifics (Simplified)
        elif "PWM" in mcu_name:
            # Mostly single-ended 0-10
            for i in range(11): entries[i] = f"{{ {i}, 0, 1.0f, false }}"
            entries[0x0E] = "{ 11, 0, 1.0f, false }" # Vbg
            entries[0x0F] = "{ 12, 0, 1.0f, false }" # GND
            
        return f"{{{{ {', '.join(entries)} }}}}"

    def gen_adc(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        idx = "".join(filter(str.isdigit, p_name)) or "0"
        pins_addr, pins_bit = [], []
        for i in range(16):
            a, b_bit = get_pad_info(port_map, p_data, f'ADC{i}', 'PIN')
            # Fallback for AT90PWM where signals are missing
            if a == "0x0U" and "PWM" in data['name'].upper():
                if i < 8: # ADC0-7 -> PORTB0-7
                    port_info = port_map.get('B', {})
                    a = hx(port_info.get('PIN', 0))
                    b_bit = i
                elif i < 11: # ADC8-10 -> PORTD0-2
                    port_info = port_map.get('D', {})
                    a = hx(port_info.get('PIN', 0))
                    b_bit = i - 8
            pins_addr.append(a)
            pins_bit.append(str(b_bit) + "U")
        
        # Build trigger map from ATDF
        adts_reg = r('ADCSRB') if r('ADCSRB')['offset'] else r('ADCSRA')
        adts_bf = adts_reg.get('bitfields', {}).get('ADTS', {})
        vals = adts_bf.get('values', {})
        mapping = ["AdcAutoTriggerSource::none"] * 16
        
        # Fallback for AT90PWM1/2/3 where ADTS is missing or split
        if not vals and "PWM" in data['name'].upper():
            mapping[0] = "AdcAutoTriggerSource::free_running"
            mapping[1] = "AdcAutoTriggerSource::analog_comparator"
            mapping[2] = "AdcAutoTriggerSource::external_interrupt_0"
            mapping[3] = "AdcAutoTriggerSource::timer0_compare_a"
            mapping[4] = "AdcAutoTriggerSource::timer0_overflow"
            mapping[5] = "AdcAutoTriggerSource::timer1_compare_b"
            mapping[6] = "AdcAutoTriggerSource::timer1_overflow"
            mapping[7] = "AdcAutoTriggerSource::timer1_capture"
            mapping[8] = "AdcAutoTriggerSource::psc0_sync"
            mapping[9] = "AdcAutoTriggerSource::psc1_sync"
            mapping[10] = "AdcAutoTriggerSource::psc2_sync"
            mapping[11] = "AdcAutoTriggerSource::analog_comparator_1"
            mapping[12] = "AdcAutoTriggerSource::analog_comparator_2"
        else:
            for trigger_name, info in vals.items():
                v = info['value']
                if v >= 16: continue
                n = trigger_name.upper()
                if "FREE" in n: mapping[v] = "AdcAutoTriggerSource::free_running"
                elif "COMPARATOR" in n: mapping[v] = "AdcAutoTriggerSource::analog_comparator"
                elif "EXT_INT0" in n or ("EXTERNAL" in n and "0" in n): mapping[v] = "AdcAutoTriggerSource::external_interrupt_0"
                elif any(x in n for x in ["T0", "TIMER_COUNTER0", "TIMER0"]):
                    if "A" in n.split('_')[-2:] or "COMPA" in n: mapping[v] = "AdcAutoTriggerSource::timer0_compare_a"
                    elif "OVF" in n or "OVERFLOW" in n: mapping[v] = "AdcAutoTriggerSource::timer0_overflow"
                elif any(x in n for x in ["T1", "TIMER_COUNTER1", "TIMER1"]):
                    if "B" in n.split('_')[-2:] or "COMPB" in n: mapping[v] = "AdcAutoTriggerSource::timer1_compare_b"
                    elif "OVF" in n or "OVERFLOW" in n: mapping[v] = "AdcAutoTriggerSource::timer1_overflow"
                    elif "CAPT" in n or "CAPTURE" in n: mapping[v] = "AdcAutoTriggerSource::timer1_capture"
                elif any(x in n for x in ["T3", "TIMER_COUNTER3", "TIMER3"]):
                    if "B" in n.split('_')[-2:] or "COMPB" in n: mapping[v] = "AdcAutoTriggerSource::timer3_compare_b"
                    elif "OVF" in n or "OVERFLOW" in n: mapping[v] = "AdcAutoTriggerSource::timer3_overflow"
                    elif "CAPT" in n or "CAPTURE" in n: mapping[v] = "AdcAutoTriggerSource::timer3_capture"
                elif "PSC0" in n: mapping[v] = "AdcAutoTriggerSource::psc0_sync"
                elif "PSC1" in n: mapping[v] = "AdcAutoTriggerSource::psc1_sync"
                elif "PSC2" in n: mapping[v] = "AdcAutoTriggerSource::psc2_sync"
                elif "AC1" in n: mapping[v] = "AdcAutoTriggerSource::analog_comparator_1"
                elif "AC2" in n: mapping[v] = "AdcAutoTriggerSource::analog_comparator_2"
                elif "AC3" in n: mapping[v] = "AdcAutoTriggerSource::analog_comparator_3"
                elif any(x in n for x in ["T4", "TIMER_COUNTER4", "TIMER4"]):
                    if "A" in n.split('_')[-2:] or "COMPARE_A" in n: mapping[v] = "AdcAutoTriggerSource::timer4_compare_a"
                    elif "B" in n.split('_')[-2:] or "COMPARE_B" in n: mapping[v] = "AdcAutoTriggerSource::timer4_compare_b"
                    elif "D" in n.split('_')[-2:] or "COMPARE_D" in n: mapping[v] = "AdcAutoTriggerSource::timer4_compare_d"
                    elif "OVF" in n or "OVERFLOW" in n: mapping[v] = "AdcAutoTriggerSource::timer4_overflow"
                elif any(x in n for x in ["T5", "TIMER_COUNTER5", "TIMER5"]):
                    if "B" in n.split('_')[-2:] or "COMPB" in n: mapping[v] = "AdcAutoTriggerSource::timer5_compare_b"
                    elif "OVF" in n or "OVERFLOW" in n: mapping[v] = "AdcAutoTriggerSource::timer5_overflow"
                    elif "CAPT" in n or "CAPTURE" in n: mapping[v] = "AdcAutoTriggerSource::timer5_capture"

        return f"""{{
            .adcl_address = {get_reg_addr(p_data, 'ADCL|ADC')}, .adch_address = {get_reg_addr(p_data, 'ADCH|ADC', True)}, .adcsra_address = {hx(r('ADCSRA')['offset'])}, .adcsrb_address = {hx(r('ADCSRB')['offset'])}, .admux_address = {hx(r('ADMUX')['offset'])},
            .vector_index = {next((i['index'] for i in data['interrupts'] if (i.get('name') or '') and 'ADC' in (i.get('name') or '').upper()), 0)}U,
            .adcsra_reset = {hx(r('ADCSRA')['initval'])}, .adcsrb_reset = {hx(r('ADCSRB')['initval'])}, .admux_reset = {hx(r('ADMUX')['initval'])},
            .didr0_address = {hx(r('DIDR0')['offset'])},
            .adc_pin_address = {{{{ {", ".join(pins_addr)} }}}},
            .adc_pin_bit = {{{{ {", ".join(pins_bit)} }}}},
            .auto_trigger_map = {{{{ {", ".join(mapping)} }}}},
            .adsc_mask = {hx(b('ADCSRA', 'ADSC'))}, .adate_mask = {hx(b('ADCSRA', 'ADATE'))}, .adif_mask = {hx(b('ADCSRA', 'ADIF'))}, .adie_mask = {hx(b('ADCSRA', 'ADIE'))}, .aden_mask = {hx(b('ADCSRA', 'ADEN'))}, .adlar_mask = {hx(b('ADMUX', 'ADLAR'))},
            .adts_mask = {hx(adts_bf.get('mask', 0x07))},
            .pr_address = {get_pr_info(data, 'PRADC')[0]}, .pr_bit = {get_pr_info(data, 'PRADC')[1]},
            .mux_table = {gen_adc_mux_table(name)}
        }}"""

    def gen_adc8x(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        
        res_ready = next((i['index'] for i in data['interrupts'] if i.get('module-instance') == p_name and 'RESRDY' in (i.get('name') or '')), 0)
        wcomp = next((i['index'] for i in data['interrupts'] if i.get('module-instance') == p_name and 'WCOMP' in (i.get('name') or '')), 0)
        
        user_addr = 0
        if groups['EVSYS']:
            user_reg = get_reg(groups['EVSYS'][0][1], f'USER{p_name}')
            if user_reg: user_addr = user_reg['offset']

        return f"""{{
            .ctrla_address = {hx(r('CTRLA')['offset'])}, .ctrlb_address = {hx(r('CTRLB')['offset'])}, .ctrlc_address = {hx(r('CTRLC')['offset'])}, .ctrld_address = {hx(r('CTRLD')['offset'])}, .ctrle_address = {hx(r('CTRLE')['offset'])},
            .sampctrl_address = {hx(r('SAMPCTRL')['offset'])}, .muxpos_address = {hx(r('MUXPOS')['offset'])}, .muxneg_address = {hx(r('MUXNEG')['offset'])}, .command_address = {hx(r('COMMAND')['offset'])}, .evctrl_address = {hx(r('EVCTRL')['offset'])},
            .intctrl_address = {hx(r('INTCTRL')['offset'])}, .intflags_address = {hx(r('INTFLAGS')['offset'])}, .dbgctrl_address = {hx(r('DBGCTRL')['offset'])}, .temp_address = {hx(r('TEMP')['offset'])},
            .res_address = {hx(r('RES')['offset'])}, .winlt_address = {hx(r('WINLT')['offset'])}, .winht_address = {hx(r('WINHT')['offset'])},
            .res_ready_vector_index = {res_ready}U, .wcomp_vector_index = {wcomp}U,
            .user_event_address = {hx(user_addr)},
            .resrd_generator_id = {event_generators.get(f"{p_name}_RESRDY", 0)}U
        }}"""

    def gen_ac8x(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        ints = p_data.get('interrupts', {})
        def get_int(key):
            for k, v in ints.items():
                if key in k.upper(): return v['index']
            return 0
        user_addr = 0
        if groups['EVSYS']:
            user_reg = get_reg(groups['EVSYS'][0][1], f'USER{p_name}')
            if user_reg: user_addr = user_reg['offset']

        return f"""{{
            .ctrla_address = {hx(r('CTRLA')['offset'])}, .muxctrla_address = {hx(r('MUXCTRLA')['offset'])}, .dacctrla_address = {hx(r('DACCTRLA')['offset'])},
            .intctrl_address = {hx(r('INTCTRL')['offset'])}, .status_address = {hx(r('STATUS')['offset'])},
            .vector_index = {get_int('AC')}U,
            .user_event_address = {hx(user_addr)},
            .out_generator_id = {event_generators.get(f"{p_name}_OUT", 0)}U
        }}"""

    def gen_usb(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        return f"""{{
            .uhwcon_address = {hx(r('UHWCON')['offset'])}, .usbcon_address = {hx(r('USBCON')['offset'])}, .usbsta_address = {hx(r('USBSTA')['offset'])}, .usbint_address = {hx(r('USBINT')['offset'])},
            .udcon_address = {hx(r('UDCON')['offset'])}, .udint_address = {hx(r('UDINT')['offset'])}, .udien_address = {hx(r('UDIEN')['offset'])}, .udaddr_address = {hx(r('UDADDR')['offset'])}, .udfnum_address = {hx(r('UDFNUM')['offset'])}, .udmfn_address = {hx(r('UDMFN')['offset'])},
            .uenum_address = {hx(r('UENUM')['offset'])}, .uerst_address = {hx(r('UERST')['offset'])}, .ueint_address = {hx(r('UEINT')['offset'])},
            .ueintx_address = {hx(r('UEINTX')['offset'])}, .ueienx_address = {hx(r('UEIENX')['offset'])}, .uedatx_address = {hx(r('UEDATX')['offset'])},
            .uebclx_address = {hx(r('UEBCLX')['offset'])}, .uebchx_address = {hx(r('UEBCHX')['offset'])}, .ueconx_address = {hx(r('UECONX')['offset'])},
            .uecfg0x_address = {hx(r('UECFG0X')['offset'])}, .uecfg1x_address = {hx(r('UECFG1X')['offset'])}, .uesta0x_address = {hx(r('UESTA0X')['offset'])}, .uesta1x_address = {hx(r('UESTA1X')['offset'])},
            .gen_vector_index = {next((i['index'] for i in data.get('interrupts', []) if 'USB_GEN' in (i.get('name') or '').upper()), 10)}U,
            .com_vector_index = {next((i['index'] for i in data.get('interrupts', []) if 'USB_COM' in (i.get('name') or '').upper()), 11)}U,
            .pllcsr_address = {gen_pllcsr()},
            .usbcon_usbe_mask = {hx(b('USBCON', 'USBE'))}, .usbcon_frzclk_mask = {hx(b('USBCON', 'FRZCLK'))},
            .udint_sofi_mask = {hx(b('UDINT', 'SOFI'))},
            .pr_address = {get_pr_info(data, 'PRUSB')[0]}, .pr_bit = {get_pr_info(data, 'PRUSB')[1]}
        }}"""
    
    def gen_psc(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        idx = "".join(filter(str.isdigit, p_name)) or "0"
        return f"""{{
            .pctl_address = {hx(r('PCTL.*')['offset'])}, .psoc_address = {hx(r('PSOC.*')['offset'])}, .pconf_address = {hx(r('PCNF.*')['offset'])}, .pim_address = {hx(r('PIM.*')['offset'])}, .pifr_address = {hx(r('PIFR.*')['offset'])}, .picr_address = {hx(r('PICR.*')['offset'])},
            .ocrsa_address = {hx(r('OCR.*SA')['offset'])}, .ocrra_address = {hx(r('OCR.*RA')['offset'])}, .ocrsb_address = {hx(r('OCR.*SB')['offset'])}, .ocrrb_address = {hx(r('OCR.*RB')['offset'])},
            .pfrc0a_address = {hx(r('PFRC.*A')['offset'])}, .pfrc0b_address = {hx(r('PFRC.*B')['offset'])},
            .psc_index = {idx}U,
            .gen_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'PSC{idx}_EC' in (i.get('name') or '').upper()), 10)}U,
            .ec_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'PSC{idx}_EC' in (i.get('name') or '').upper()), 10)}U,
            .capt_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'PSC{idx}_CAPT' in (i.get('name') or '').upper()), 11)}U,
            .prun_mask = {hx(get_bit(r('PCTL.*'), 'PRUN'))}, .mode_mask = {hx(get_bit(r('PCNF.*'), 'PMODE'))}, .clksel_mask = {hx(get_bit(r('PCNF.*'), 'PCLKSEL'))},
            .ec_flag_mask = {hx(get_bit(r('PIFR.*'), '^PEOP.*') or get_bit(r('PIFR.*'), 'PEV.*'))}, .capt_flag_mask = {hx(get_bit(r('PIFR.*'), '^PCAP.*') or get_bit(r('PIFR.*'), 'PEV.*A'))},
            .pr_address = {get_pr_info(data, f'PRPSC{idx}')[0]}, .pr_bit = {get_pr_info(data, f'PRPSC{idx}')[1]}
        }}"""

    def gen_dac(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        
        # Check for 16-bit 'DAC' register or split 'DACL'/'DACH'
        dac_reg = r('DAC')
        if dac_reg['offset']:
            dacl_addr = dac_reg['offset']
            dach_addr = dacl_addr + 1
        else:
            dacl_addr = r('DACL')['offset']
            dach_addr = r('DACH')['offset']

        return f"""{{
            .dacon_address = {hx(r('DACON')['offset'])}, .dacl_address = {hx(dacl_addr)}, .dach_address = {hx(dach_addr)},
            .daen_mask = {hx(b('DACON', 'DAEN'))}, .daate_mask = {hx(b('DACON', 'DAATE'))}, .dats_mask = {hx(b('DACON', 'DATS'))}, .dacoe_mask = {hx(b('DACON', 'DAOE'))},
            .pr_address = {get_pr_info(data, 'PRDAC')[0]}, .pr_bit = {get_pr_info(data, 'PRDAC')[1]}
        }}"""

    def gen_ac(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        
        # Check for multiple comparators (AT90PWM style)
        sub_comps = []
        for i in range(4):
            reg_name = f'AC{i}CON'
            if r(reg_name).get('offset'):
                sub_comps.append(i)
        
        if not sub_comps:
            # Standard single AC (e.g. 328P)
            aip_a, aip_b = get_pad_info(port_map, p_data, 'AIN0', 'PIN')
            aim_a, aim_b = get_pad_info(port_map, p_data, 'AIN1', 'PIN')
            return f"""{{
                .acsr_address = {hx(r('ACSR')['offset'])}, .accon_address = 0, .didr_address = {hx(r('DIDR1')['offset'])},
                .vector_index = {next((i['index'] for i in data['interrupts'] if 'ANALOG_COMP' in (i.get('name') or '').upper()), 0)}U,
                .aip_pin_address = {aip_a}, .aip_pin_bit = {aip_b}U, .aim_pin_address = {aim_a}, .aim_pin_bit = {aim_b}U,
                .acd_mask = {hx(b('ACSR', 'ACD'))}, .acbg_mask = {hx(b('ACSR', 'ACBG'))}, .aco_mask = {hx(b('ACSR', 'ACO'))},
                .acif_mask = {hx(b('ACSR', 'ACI'))}, .acie_mask = {hx(b('ACSR', 'ACIE'))}, .acic_mask = {hx(b('ACSR', 'ACIC'))}, .acis_mask = {hx(b('ACSR', 'ACIS'))}
            }}"""
        else:
            # Multi AC
            results = []
            for i in sub_comps:
                reg_name = f'AC{i}CON'
                # PWM specific pin mapping fallback
                aip_a, aip_b = "0x0U", 0
                aim_a, aim_b = "0x0U", 0
                if "PWM" in data['name'].upper():
                    # Heuristic for AT90PWM1/2/3
                    pb = port_map.get('B', {})
                    pd = port_map.get('D', {})
                    if i == 0: # AC0: PB2, PB3
                        aip_a, aip_b = hx(pb.get('PIN', 0)), 2
                        aim_a, aim_b = hx(pb.get('PIN', 0)), 3
                    elif i == 1: # AC1: PD2, PD3
                        aip_a, aip_b = hx(pd.get('PIN', 0)), 2
                        aim_a, aim_b = hx(pd.get('PIN', 0)), 3
                    elif i == 2: # AC2: PB? (Not in AT90PWM1, but in 3)
                        aip_a, aip_b = hx(pb.get('PIN', 0)), 4
                        aim_a, aim_b = hx(pb.get('PIN', 0)), 5
                
                results.append(f"""{{
                    .acsr_address = {hx(r('ACSR')['offset'])}, .accon_address = {hx(r(reg_name)['offset'])}, .didr_address = {hx(r('DIDR1')['offset'])},
                    .vector_index = {next((idx['index'] for idx in data['interrupts'] if (f'AC{i}' in (idx.get('name') or '').upper()) or (f'ANALOG_COMP_{i}' in (idx.get('name') or '').upper()) or ('ANALOG_COMP' in (idx.get('name') or '').upper())), 0)}U,
                    .aip_pin_address = {aip_a}, .aip_pin_bit = {aip_b}U, .aim_pin_address = {aim_a}, .aim_pin_bit = {aim_b}U,
                    .acd_mask = {hx(b(reg_name, f'AC{i}EN'))}, .acbg_mask = 0x0, .aco_mask = {hx(b('ACSR', f'AC{i}O'))},
                    .acif_mask = {hx(b('ACSR', f'AC{i}IF'))}, .acie_mask = {hx(b(reg_name, f'AC{i}IE'))}, .acic_mask = 0x0, .acis_mask = {hx(b(reg_name, f'AC{i}IS'))}
                }}""")
            # HACK: Return multiple descriptors as one string to be splatted by the caller
            return " , ".join(results)

    def gen_spi(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        idx = "".join(filter(str.isdigit, p_name)) or "0"
        
        # Mapping
        spcr = r('SPCR|CTRLA')
        spsr = r('SPSR|INTFLAGS|CTRLB')
        spdr = r('SPDR|DATA')
        spie_reg = r('SPCR|INTCTRL')
        
        return f"""{{
            .spcr_address = {hx(spcr['offset'])}, .spsr_address = {hx(spsr['offset'])}, .spdr_address = {hx(spdr['offset'])},
            .spcr_reset = {hx(spcr['initval'])}, .spsr_reset = {hx(spsr['initval'])}, .vector_index = {next((i['index'] for i in data['interrupts'] if (i.get('name') or '') and 'SPI' in i['name']), 0)}U,
            .spe_mask = {hx(b('SPCR|CTRLA', 'SPE|ENABLE'))}, .spie_mask = {hx(b('SPCR|INTCTRL', 'SPIE|IE'))}, .mstr_mask = {hx(b('SPCR|CTRLA', 'MSTR|MASTER'))}, .spif_mask = {hx(b('SPSR|INTFLAGS', 'SPIF|IF'))}, .wcol_mask = {hx(b('SPSR', 'WCOL'))}, .sp2x_mask = {hx(b('SPSR', 'SPI2X'))},
            .pr_address = {get_pr_info(data, f'PRSPI{idx}|PRSPI')[0]}, .pr_bit = {get_pr_info(data, f'PRSPI{idx}|PRSPI')[1]}
        }}"""

    def gen_twi(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        idx = "".join(filter(str.isdigit, p_name)) or "0"
        
        # MegaAVR
        twbr = r('TWBR')
        if twbr['offset']:
            return f"""{{
                .twbr_address = {hx(r('TWBR')['offset'])}, .twsr_address = {hx(r('TWSR')['offset'])}, .twar_address = {hx(r('TWAR')['offset'])}, .twdr_address = {hx(r('TWDR')['offset'])}, .twcr_address = {hx(r('TWCR')['offset'])}, .twamr_address = {hx(r('TWAMR')['offset'])},
                .vector_index = {next((i['index'] for i in data['interrupts'] if (i.get('name') or '') and 'TWI' in i['name']), 0)}U,
                .twint_mask = {hx(b('TWCR', 'TWINT'))}, .twen_mask = {hx(b('TWCR', 'TWEN'))}, .twie_mask = {hx(b('TWCR', 'TWIE'))}, .twsto_mask = {hx(b('TWCR', 'TWSTO'))}, .twsta_mask = {hx(b('TWCR', 'TWSTA'))}, .twea_mask = {hx(b('TWCR', 'TWEA'))},
                .pr_address = {get_pr_info(data, f'PRTWI{idx}|PRTWI')[0]}, .pr_bit = {get_pr_info(data, f'PRTWI{idx}|PRTWI')[1]}
            }}"""
        else:
            # AVR8X (Mega-0) - map Host (Master) to standard fields for now
            return f"""{{
                .twbr_address = {hx(r('MBAUD')['offset'])}, .twsr_address = {hx(r('MSTATUS')['offset'])}, .twar_address = {hx(r('SADDR')['offset'])}, .twdr_address = {hx(r('MDATA')['offset'])}, .twcr_address = {hx(r('MCTRLA')['offset'])}, .twamr_address = {hx(r('SADDRMASK')['offset'])},
                .vector_index = {next((i['index'] for i in data['interrupts'] if (p_name in (i.get('name') or '').upper() or 'TWI' in (i.get('name') or '').upper())), 0)}U,
                .twint_mask = 0x80U, .twen_mask = 0x01U, .twie_mask = 0x80U, .twsto_mask = 0x03U, .twsta_mask = 0x01U, .twea_mask = 0x04U,
                .pr_address = 0, .pr_bit = 255
            }}"""
    
    def gen_uart8x(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        ints = p_data.get('interrupts', {})
        def get_int(key):
            for k, v in ints.items():
                if key in k.upper(): return v['index']
            return 0
        user_addr = 0
        if groups['EVSYS']:
            user_reg = get_reg(groups['EVSYS'][0][1], f'USER{p_name}')
            if user_reg: user_addr = user_reg['offset']
        return f"""{{
            .ctrla_address = {hx(r('CTRLA')['offset'])}, .ctrlb_address = {hx(r('CTRLB')['offset'])}, .ctrlc_address = {hx(r('CTRLC')['offset'])},
            .ctrld_address = {hx(r('CTRLD')['offset'])}, .status_address = {hx(r('STATUS')['offset'])}, .baud_address = {hx(r('BAUD')['offset'])},
            .rxdata_address = {hx(r('RXDATAL')['offset'])}, .txdata_address = {hx(r('TXDATAL')['offset'])}, .dbgctrl_address = {hx(r('DBGCTRL')['offset'])},
            .rx_vector_index = {get_int('RXC')}U, .tx_vector_index = {get_int('TXC')}U, .dre_vector_index = {get_int('DRE')}U,
            .user_event_address = {hx(user_addr)}
        }}"""

    def gen_spi8x(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        ints = p_data.get('interrupts', {})
        def get_int(key):
            for k, v in ints.items():
                if key in k.upper(): return v['index']
            return 0
        return f"""{{
            .ctrla_address = {hx(r('CTRLA')['offset'])}, .ctrlb_address = {hx(r('CTRLB')['offset'])}, .intctrl_address = {hx(r('INTCTRL')['offset'])}, .intflags_address = {hx(r('INTFLAGS')['offset'])}, .data_address = {hx(r('DATA')['offset'])},
            .vector_index = {get_int('INT')}U
        }}"""
        
    def gen_twi8x(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        ints = p_data.get('interrupts', {})
        def get_int(key):
            for k, v in ints.items():
                if key in k.upper(): return v['index']
            return 0
        return f"""{{
            .mctrla_address = {hx(r('MCTRLA')['offset'])}, .mctrlb_address = {hx(r('MCTRLB')['offset'])}, .mstatus_address = {hx(r('MSTATUS')['offset'])}, .mbaud_address = {hx(r('MBAUD')['offset'])}, .maddr_address = {hx(r('MADDR')['offset'])}, .mdata_address = {hx(r('MDATA')['offset'])},
            .sctrla_address = {hx(r('SCTRLA')['offset'])}, .sctrlb_address = {hx(r('SCTRLB')['offset'])}, .sstatus_address = {hx(r('SSTATUS')['offset'])}, .saddr_address = {hx(r('SADDR')['offset'])}, .sdata_address = {hx(r('SDATA')['offset'])}, .saddrmask_address = {hx(r('SADDRMASK')['offset'])},
            .dbgctrl_address = {hx(r('DBGCTRL')['offset'])},
            .master_vector_index = {get_int('TWIM')}U, .slave_vector_index = {get_int('TWIS')}U
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

    def gen_crc8x(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        return f"""{{
            .ctrla_address = {hx(r('CTRLA')['offset'])}, .status_address = {hx(r('STATUS')['offset'])}, .data_address = {hx(r('DATA')['offset'])}, .checksum_address = {hx(r('CHECKSUM')['offset'])}
        }}"""

    def gen_wdt8x(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        ints = p_data.get('interrupts', {})
        def get_int(key):
            for k, v in ints.items():
                if key in k.upper(): return v['index']
            return 0
        return f"""{{
            .ctrla_address = {hx(r('CTRLA')['offset'])}, 
            .winctrla_address = {hx(r('WINCTRLA')['offset'])},
            .intctrl_address = {hx(r('INTCTRL')['offset'])},
            .status_address = {hx(r('STATUS')['offset'])},
            .vector_index = {get_int('WDT')}U
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
        m_eeprom = get_mapping(data, 'eeprom')
        return f"""{{
            .eecr_address = {hx(r('EECR')['offset'])}, .eedr_address = {hx(r('EEDR')['offset'])}, .eearl_address = {get_reg_addr(p_data, 'EEARL|EEAR')}, .eearh_address = {get_reg_addr(p_data, 'EEARH|EEAR', True)},
            .eecr_reset = {hx(r('EECR')['initval'])},
            .vector_index = {next((i['index'] for i in data['interrupts'] if 'EE_READY' in i['name']), 0)}U,
            .size = {hx(eeprom['size'])},
            .mapped_data = {{ {m_eeprom[0]}, {m_eeprom[1]} }}
        }}"""

    def gen_wdt(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        return f"""{{
            .wdtcsr_address = {hx(r('WDTCSR|WDTCR')['offset'])},
            .wdtcsr_reset = {hx(r('WDTCSR|WDTCR')['initval'])},
            .vector_index = {next((i['index'] for i in data['interrupts'] if (i.get('name') or '') and 'WDT' in i['name']), 0)}U,
            .wdie_mask = {hx(b('WDTCSR|WDTCR', 'WDIE'))}, .wde_mask = {hx(b('WDTCSR|WDTCR', 'WDE'))}, .wdce_mask = {hx(b('WDTCSR|WDTCR', 'WDCE'))}
        }}"""

    def gen_xmem(p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        xmcra = r('XMCRA')
        xmcrb = r('XMCRB')
        mcucr = r('MCUCR')
        sre_mask = b('XMCRA', 'SRE') or b('MCUCR', 'SRE') or 0x80
        srl_mask = b('XMCRA', 'SRL') or (hx(0x70) if xmcra['offset'] else 0)
        srw0_mask = b('XMCRA', 'SRW0') or (hx(0x03) if xmcra['offset'] else 0)
        srw1_mask = b('XMCRA', 'SRW1') or (hx(0x0C) if xmcra['offset'] else 0)
        xmm_mask = b('XMCRB', 'XMM') or (hx(0x07) if xmcrb['offset'] else 0)
        xmbk_mask = b('XMCRB', 'XMBK') or (hx(0x80) if xmcrb['offset'] else 0)
        return f"""{{
            .xmcra_address = {hx(xmcra['offset'])}, .xmcrb_address = {hx(xmcrb['offset'])},
            .mcucr_address = {hx(mcucr['offset'])}, .sre_mask = {hx(sre_mask)},
            .srl_mask = {srl_mask}, .srw0_mask = {srw0_mask}, .srw1_mask = {srw1_mask},
            .xmm_mask = {xmm_mask}, .xmbk_mask = {xmbk_mask}
        }}"""

    def gen_nvm_ctrl(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        return f"""{{
            .ctrla_address = {hx(r('CTRLA')['offset'])},
            .ctrlb_address = {hx(r('CTRLB')['offset'])},
            .status_address = {hx(r('STATUS')['offset'])},
            .intctrl_address = {hx(r('INTCTRL')['offset'])},
            .intflags_address = {hx(r('INTFLAGS')['offset'])},
            .addr_address = {hx(r('ADDR')['offset'])},
            .data_address = {hx(r('DATA')['offset'])},
            .vector_index = {p_data.get('interrupts', {}).get('EEREADY', {}).get('index', next((i['index'] for i in data.get('interrupts', []) if 'EEREADY' in (i.get('name') or '').upper() or 'NVM' in (i.get('name') or '').upper()), 0))}U
        }}"""

    def gen_cpu_int(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        return f"""{{
            .ctrla_address = {hx(r('CTRLA')['offset'])},
            .status_address = {hx(r('STATUS')['offset'])},
            .lvl0pri_address = {hx(r('LVL0PRI')['offset'])},
            .lvl1vec_address = {hx(r('LVL1VEC')['offset'])}
        }}"""

    def gen_tca(p_name, p_data):
        def get_r(n):
            return get_reg(p_data, n) or get_reg(p_data, f'.*\\.{n}')
        r = lambda n: get_r(n) or {'offset': 0, 'initval': 0}
        ints = p_data.get('interrupts', {})
        def get_int(key):
            for k, v in ints.items():
                if key in k.upper(): return v['index']
            return 0
            
        user_addr = 0
        if groups['EVSYS']:
            user_reg = get_reg(groups['EVSYS'][0][1], f'USER{p_name}')
            if user_reg: user_addr = user_reg['offset']

        def get_gen(sig):
            return event_generators.get(f"{p_name}_{sig}", 0)

        return f"""{{
            .ctrla_address = {hx(r('CTRLA')['offset'])}, .ctrlb_address = {hx(r('CTRLB')['offset'])}, .ctrlc_address = {hx(r('CTRLC')['offset'])},
            .ctrld_address = {hx(r('CTRLD')['offset'])}, .ctrleclr_address = {hx(r('CTRLECLR')['offset'])}, .ctrleset_address = {hx(r('CTRLESET')['offset'])},
            .ctrlfclr_address = {hx(r('CTRLFCLR')['offset'])}, .ctrlfset_address = {hx(r('CTRLFSET')['offset'])}, .evctrl_address = {hx(r('EVCTRL')['offset'])},
            .intctrl_address = {hx(r('INTCTRL')['offset'])}, .intflags_address = {hx(r('INTFLAGS')['offset'])}, .dbgctrl_address = {hx(r('DBGCTRL')['offset'])},
            .temp_address = {hx(r('TEMP')['offset'])}, .tcnt_address = {hx(r('CNT|TCNT')['offset'])}, .period_address = {hx(r('PER|PERIOD')['offset'])},
            .cmp0_address = {hx(r('CMP0')['offset'])}, .cmp1_address = {hx(r('CMP1')['offset'])}, .cmp2_address = {hx(r('CMP2')['offset'])},
            .perbuf_address = {hx(r('PERBUF')['offset'])}, .cmp0buf_address = {hx(r('CMP0BUF')['offset'])}, .cmp1buf_address = {hx(r('CMP1BUF')['offset'])}, .cmp2buf_address = {hx(r('CMP2BUF')['offset'])},
            .luf_ovf_vector_index = {get_int('OVF') or get_int('LUF')}U, .cmp0_vector_index = {get_int('CMP0')}U,
            .cmp1_vector_index = {get_int('CMP1')}U, .cmp2_vector_index = {get_int('CMP2')}U,
            .hunf_vector_index = {get_int('HUNF')}U, .lcmp0_vector_index = {get_int('LCMP0')}U,
            .lcmp1_vector_index = {get_int('LCMP1')}U, .lcmp2_vector_index = {get_int('LCMP2')}U,
            .user_event_address = {hx(user_addr)},
            .ovf_generator_id = {get_gen('OVF_LUNF') or get_gen('OVF')}U,
            .cmp0_generator_id = {get_gen('CMP0')}U, .cmp1_generator_id = {get_gen('CMP1')}U, .cmp2_generator_id = {get_gen('CMP2')}U
        }}"""

    def gen_tcb(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        ints = p_data.get('interrupts', {})
        def get_int(key):
            for k, v in ints.items():
                if key in k.upper(): return v['index']
            # Global fallback for modern devices
            for item in data.get('interrupts', []):
                name = item['name'].upper()
                if f"{p_name}_{key}" in name or (p_name in name and key in name):
                    return item['index']
            return 0
            
        user_addr = 0
        if groups['EVSYS']:
            user_reg = get_reg(groups['EVSYS'][0][1], f'USER{p_name}')
            if user_reg: user_addr = user_reg['offset']

        return f"""{{
            .ctrla_address = {hx(r('CTRLA')['offset'])}, .ctrlb_address = {hx(r('CTRLB')['offset'])}, .evctrl_address = {hx(r('EVCTRL')['offset'])},
            .intctrl_address = {hx(r('INTCTRL')['offset'])}, .intflags_address = {hx(r('INTFLAGS')['offset'])}, .status_address = {hx(r('STATUS')['offset'])},
            .dbgctrl_address = {hx(r('DBGCTRL')['offset'])}, .temp_address = {hx(r('TEMP')['offset'])}, .cnt_address = {hx(r('CNT')['offset'])},
            .ccmp_address = {hx(r('CCMP')['offset'])}, .vector_index = {get_int('CAPT') or get_int('INT')}U,
            .user_event_address = {hx(user_addr)},
            .capt_generator_id = {event_generators.get(f"{p_name}_CAPT", 0)}U
        }}"""

    def gen_rtc(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        ints = p_data.get('interrupts', {})
        def get_int(key):
            for k, v in ints.items():
                if key in k.upper(): return v['index']
            # Global fallback
            for item in data.get('interrupts', []):
                name = item['name'].upper()
                if (p_name in name and key in name) or (key == name and p_name == "RTC"):
                    return item['index']
            return 0
        return f"""{{
            .ctrla_address = {hx(r('CTRLA')['offset'])}, .status_address = {hx(r('STATUS')['offset'])}, .intctrl_address = {hx(r('INTCTRL')['offset'])},
            .intflags_address = {hx(r('INTFLAGS')['offset'])}, .temp_address = {hx(r('TEMP')['offset'])}, .dbgctrl_address = {hx(r('DBGCTRL')['offset'])},
            .clksel_address = {hx(r('CLKSEL')['offset'])}, .cnt_address = {hx(r('CNT')['offset'])}, .per_address = {hx(r('PER')['offset'])},
            .cmp_address = {hx(r('CMP')['offset'])}, .pitctrla_address = {hx(r('PITCTRLA')['offset'])}, .pitstatus_address = {hx(r('PITSTATUS')['offset'])},
            .pitintctrl_address = {hx(r('PITINTCTRL')['offset'])}, .pitintflags_address = {hx(r('PITINTFLAGS')['offset'])},
            .ovf_vector_index = {get_int('OVF')}U, .pit_vector_index = {get_int('PIT')}U,
            .ovf_generator_id = {event_generators.get(f"{p_name}_OVF", 0)}U,
            .cmp_generator_id = {event_generators.get(f"{p_name}_CMP", 0)}U
        }}"""

    def gen_evsys(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        ch_count = 0
        user_count = 0
        user_base = 0xFFFF
        for reg_name, reg_def in p_data['registers'].items():
            if 'CHANNEL' in reg_name: ch_count += 1
            if 'USER' in reg_name:
                 user_count += 1
                 user_base = min(user_base, reg_def['offset'])
        if user_base == 0xFFFF: user_base = 0
        return f"""{{
            .strobe_address = {hx(r('STROBE')['offset'])},
            .channels_address = {hx(r('CHANNEL0')['offset'])},
            .users_address = {hx(user_base)},
            .channel_count = {ch_count}U,
            .user_count = {user_count}U
        }}"""

    def gen_ccl(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        luts_str = []
        lut_count = 0
        while True:
            la = get_reg(p_data, f'LUT{lut_count}CTRLA')
            if not la: break
            lb = get_reg(p_data, f'LUT{lut_count}CTRLB') or {'offset': 0}
            lc = get_reg(p_data, f'LUT{lut_count}CTRLC') or {'offset': 0}
            lt = get_reg(p_data, f'TRUTH{lut_count}') or {'offset': 0}
            luts_str.append(f"""{{
                .ctrla_address = {hx(la['offset'])}, .ctrlb_address = {hx(lb['offset'])},
                .ctrlc_address = {hx(lc['offset'])}, .truth_address = {hx(lt['offset'])}
            }}""")
            lut_count += 1
        while len(luts_str) < 8: luts_str.append("{0}")
        
        seqs = [hx(r(f'SEQCTRL{i}')['offset']) for i in range(4)]
        
        def get_multi_reg(base, i):
            reg = get_reg(p_data, f'{base}{i}')
            if reg: return hx(reg['offset'])
            if i == 0:
                generic = get_reg(p_data, base)
                if generic: return hx(generic['offset'])
            return "0"

        ints = [get_multi_reg('INTCTRL', i) for i in range(2)]
        flags = [get_multi_reg('INTFLAGS', i) for i in range(2)]

        return f"""{{
            .ctrla_address = {hx(r('CTRLA')['offset'])},
            .seqctrl_addresses = {{ {", ".join(seqs)} }},
            .intctrl_addresses = {{ {", ".join(ints)} }},
            .intflags_addresses = {{ {", ".join(flags)} }},
            .lut_count = {lut_count}U,
            .luts = {{ {", ".join(luts_str)} }},
            .vector_index = {p_data.get('interrupts', {}).get('INT', {}).get('index', 0)}U
        }}"""

    def gen_portmux(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0}
        return f"""{{
            .twispiroutea_address = {hx(r('TWISPIROUTEA')['offset'])},
            .usartroutea_address = {hx(r('USARTROUTEA')['offset'])},
            .evoutroutea_address = {hx(r('EV.*ROUTEA')['offset'])},
            .tcaroutea_address = {hx(r('TCAROUTEA')['offset'])},
            .tcbroutea_address = {hx(r('TCBROUTEA')['offset'])}
        }}"""

    uarts_str = ",\n        ".join(gen_uart(n, d) for n, d in groups['USART'])
    timers8_str = ",\n        ".join(gen_timer8(n, d) for n, d in (groups['TC8'] + groups['TC8_ASYNC']))
    timers16_str = ",\n        ".join(gen_timer16(n, d) for n, d in groups['TC16'])
    timers_tca_str = ",\n        ".join(gen_tca(n, d) for n, d in groups['TCA'])
    timers_tcb_str = ",\n        ".join(gen_tcb(n, d) for n, d in groups['TCB'])
    timers_rtc_str = ",\n        ".join(gen_rtc(n, d) for n, d in groups['RTC'])
    evsys_descriptor = gen_evsys("EVSYS", groups['EVSYS'][0][1]) if groups['EVSYS'] else "{}"
    ccl_descriptor = gen_ccl("CCL", groups['CCL'][0][1]) if groups['CCL'] else "{}"
    portmux_descriptor = gen_portmux("PORTMUX", groups['PORTMUX'][0][1]) if groups['PORTMUX'] else "{}"
    adcs_descriptors = []
    adc8x_descriptors = []
    for n, d in groups['ADC']:
        if 'MUXPOS' in d.get('registers', {}):
            adc8x_descriptors.append(gen_adc8x(n, d))
        else:
            adcs_descriptors.append(gen_adc(n, d))
    
    adcs_str = ",\n        ".join(adcs_descriptors)
    adcs8x_str = ",\n        ".join(adc8x_descriptors)
    acs_descriptors = []
    acs8x_descriptors = []
    for n, d in groups['AC']:
        if 'CTRLA' in d.get('registers', {}):
            acs8x_descriptors.append(gen_ac8x(n, d))
        else:
            res = gen_ac(n, d)
            if " , " in res: acs_descriptors.extend(res.split(" , "))
            else: acs_descriptors.append(res)

    acs_str = ",\n        ".join(acs_descriptors)
    acs8x_str = ",\n        ".join(acs8x_descriptors)
    uarts_str = ",\n        ".join(gen_uart(n, d) for n, d in groups['USART'])
    uarts8x_str = ",\n        ".join(gen_uart8x(n, d) for n, d in groups['USART8X'])
    spis_str = ",\n        ".join(gen_spi(n, d) for n, d in groups['SPI'])
    spis8x_str = ",\n        ".join(gen_spi8x(n, d) for n, d in groups['SPI8X'])
    twis_str = ",\n        ".join(gen_twi(n, d) for n, d in groups['TWI'])
    twis8x_str = ",\n        ".join(gen_twi8x(n, d) for n, d in groups['TWI8X'])
    cans_str = ",\n        ".join(gen_can(n, d) for n, d in groups['CAN'])
    pcints_str = ",\n        ".join(gen_pcint(n, d) for n, d in groups['PCINT'])
    ext_ints_str = ",\n        ".join(gen_ext_int(n, d) for n, d in groups['EXINT'])
    eeprom_descriptors = [gen_eeprom(n, d) for n, d in groups['EEPROM']]
    if not eeprom_descriptors and eeprom['size'] > 0:
        m_eeprom = get_mapping(data, 'eeprom')
        if m_eeprom[0] != "0x0U":
            eeprom_descriptors.append(f"""{{
                .size = {hx(eeprom['size'])},
                .mapped_data = {{ {m_eeprom[0]}, {m_eeprom[1]} }}
            }}""")
    eeproms_str = ",\n        ".join(eeprom_descriptors)
    wdts_str = ",\n        ".join(gen_wdt(n, d) for n, d in groups['WDT'])
    timers10_str = ",\n        ".join(gen_timer10(n, d) for n, d in groups['TC10'])
    usbs_str = ",\n        ".join(gen_usb(n, d) for n, d in groups['USB_DEVICE'])
    pscs_str = ",\n        ".join(gen_psc(n, d) for n, d in groups['PSC'])
    dacs_str = ",\n        ".join(gen_dac(n, d) for n, d in groups['DAC'])
    crc8x_descriptors = [gen_crc8x(n, d) for n, d in groups['CRC'] + groups['CRCSCAN']]
    crc8x_str = ",\n        ".join(crc8x_descriptors)
    wdt8x_descriptors = [gen_wdt8x(n, d) for n, d in groups['WDT8X']]
    wdt8x_str = ",\n        ".join(wdt8x_descriptors)
    nvm_ctrls_descriptors = [gen_nvm_ctrl(n, d) for n, d in groups['NVMCTRL']]
    nvm_ctrls_str = ",\n        ".join(nvm_ctrls_descriptors)
    
    cpu_ints_descriptors = [gen_cpu_int(n, d) for n, d in groups['CPUINT']]
    cpu_ints_str = ",\n        ".join(cpu_ints_descriptors)
    
    xmem_data = (groups['EXTERNAL_MEMORY'][0][1] if groups['EXTERNAL_MEMORY'] 
                 else data['peripherals'].get('CPU', {}))
    xmem_str = gen_xmem(xmem_data)

    ports_str = ",\n        ".join(f'{{ "{p["name"]}", {hx(p["pin"])}, {hx(p["ddr"])}, {hx(p["port"])} }}' for p in ports_raw)

    # RWW end word calculation (start of first boot section)
    boot_sections = [m for m in data['memories'].get('prog', []) if 'BOOT_SECTION' in m['name']]
    rww_end_word = (min(m['start'] for m in boot_sections) // 2) if boot_sections else (flash['size'] // 2)

    # Address ranges
    io_seg = next((m for m in data['memories'].get('data', []) if m.get('type') == 'io'), {'start': 0x20, 'size': 0x40})
    io_start = io_seg['start']
    io_end = io_start + 0x3F # Default 64 regs
    ext_io_start = io_end + 1
    ext_io_end = io_start + io_seg['size'] - 1

    m_flash = get_mapping(data, 'flash')
    m_eeprom = get_mapping(data, 'eeprom')
    m_fuses = get_mapping(data, 'fuses')
    m_sigs = get_mapping(data, 'signatures')
    m_usigs = get_mapping(data, 'user_signatures')

    # Extract properties and configuration
    props = data.get('properties', {})
    
    sigs = [hx(props.get(f'SIGNATURE{i}', '0xFF')) for i in range(3)]
    
    # Extract fuses (try to find individual regs or 16-bit space)
    fuses_data = [0xFF] * 16
    for f_name, f_reg in data.get('configuration', {}).get('fuses', {}).items():
        off = f_reg.get('offset', 0)
        if off < 16:
            fuses_data[off] = f_reg.get('initval', 0xFF)
    fuses_str = ", ".join(hx(v) for v in fuses_data)

    content = f"""#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {{

inline constexpr DeviceDescriptor {safe_name} {{
    .name = "{name}",
    .flash_words = {flash['size'] // 2}U,
    .sram_bytes = {sram_bytes}U,
    .sram_start = {hx(sram_start)},
    .eeprom_bytes = {eeprom['size']}U,
    .interrupt_vector_count = {len(data['interrupts'])}U,
    .interrupt_vector_size = {4 if flash['size'] > 4096 else 2}U,
    .flash_page_size = {flash['pagesize']}U,
    .io_range = {{ {hx(io_start)}, {hx(io_end)} }},
    .extended_io_range = {{ {hx(ext_io_start)}, {hx(ext_io_end)} }},

    .mapped_flash = {{ {m_flash[0]}, {m_flash[1]} }},
    .mapped_eeprom = {{ {m_eeprom[0]}, {m_eeprom[1]} }},
    .mapped_fuses = {{ {m_fuses[0]}, {m_fuses[1]} }},
    .mapped_signatures = {{ {m_sigs[0]}, {m_sigs[1]} }},
    .mapped_user_signatures = {{ {m_usigs[0]}, {m_usigs[1]} }},

    .spl_address = {get_reg_addr(cpu, 'SPL|SP')},
    .sph_address = {get_reg_addr(cpu, 'SPH|SP', True)},
    .sreg_address = {get_reg_addr(cpu, 'SREG')},
    .rampz_address = {get_reg_addr(cpu, 'RAMPZ')},
    .eind_address = {get_reg_addr(cpu, 'EIND')},
    .spmcsr_address = {hx(get_reg(data['peripherals'].get('CPU', {}), 'SPMCSR')['offset']) if get_reg(data['peripherals'].get('CPU', {}), 'SPMCSR') else hx(get_reg(data['peripherals'].get('BOOT_LOAD', {}), 'SPMCSR')['offset']) if get_reg(data['peripherals'].get('BOOT_LOAD', {}), 'SPMCSR') else '0x0U'},
    .prr_address = {hx(r_cpu('PRR')['offset']) if r_cpu('PRR') else '0x0U'},
    .prr0_address = {hx(r_cpu('PRR0')['offset']) if r_cpu('PRR0') else '0x0U'},
    .prr1_address = {hx(r_cpu('PRR1')['offset']) if r_cpu('PRR1') else '0x0U'},
    .smcr_address = {hx(r_cpu('SMCR')['offset']) if r_cpu('SMCR') else '0x0U'},
    .mcusr_address = {hx(r_cpu('MCUSR')['offset']) if r_cpu('MCUSR') else '0x0U'},
    .mcucr_address = {hx(r_cpu('MCUCR')['offset']) if r_cpu('MCUCR') else '0x0U'},
    .ccp_address = {hx(r_cpu('CCP')['offset']) if r_cpu('CCP') else '0x0U'},
    .pllcsr_address = {gen_pllcsr()},
    .xmcra_address = {hx(r_cpu('XMCRA')['offset']) if r_cpu('XMCRA') else '0x0U'},
    .xmcrb_address = {hx(r_cpu('XMCRB')['offset']) if r_cpu('XMCRB') else '0x0U'},
    .xmem = {xmem_str if r_cpu('XMCRA') else '{0}'},
    .pradc_bit = {hx(get_pr_info(data, 'PRADC')[1])},

    .prusart0_bit = {hx(get_pr_info(data, 'PRUSART0|PRUSART')[1])},
    .prspi_bit = {hx(get_pr_info(data, 'PRSPI')[1])},
    .prtwi_bit = {hx(get_pr_info(data, 'PRTWI')[1])},
    .prtimer0_bit = {hx(get_pr_info(data, 'PRTIM0')[1])},
    .prtimer1_bit = {hx(get_pr_info(data, 'PRTIM1')[1])},
    .prtimer2_bit = {hx(get_pr_info(data, 'PRTIM2')[1])},
    .smcr_sm_mask = {hx(b_cpu('SMCR', 'SM'))},
    .smcr_se_mask = {hx(b_cpu('SMCR', 'SE'))},
    .flash_rww_end_word = {hx(rww_end_word)},
    .spl_reset = {hx(r_cpu('SPL|SP')['initval'])},
    .sph_reset = {hx(r_cpu('SPH|SP')['initval'] if r_cpu('SPH|SP') and r_cpu('SPH|SP')['size'] > 1 else 0)},
    .sreg_reset = {hx(r_cpu('SREG')['initval'])},
    .adc_count = {len(adcs_descriptors)}U,
    .adcs = {{{{ {adcs_str} }}}},
    .adc8x_count = {len(adc8x_descriptors)}U,
    .adcs8x = {{{{ {adcs8x_str} }}}},
    .ac_count = {len(acs_descriptors)}U,
    .acs = {{{{ {acs_str} }}}},
    .ac8x_count = {len(acs8x_descriptors)}U,
    .acs8x = {{{{ {acs8x_str} }}}},
    .timer8_count = {len(groups['TC8'] + groups['TC8_ASYNC'])}U,
    .timers8 = {{{{ {timers8_str} }}}},
    .timer16_count = {len(groups['TC16'])}U,
    .timers16 = {{{{ {timers16_str} }}}},
    .timer10_count = {len(groups['TC10'])}U,
    .timers10 = {{{{ {timers10_str} }}}},
    .tca_count = {len(groups['TCA'])}U,
    .timers_tca = {{{{ {timers_tca_str} }}}},

    .tcb_count = {len(groups['TCB'])}U,
    .timers_tcb = {{{{ {timers_tcb_str} }}}},

    .rtc_count = {len(groups['RTC'])}U,
    .timers_rtc = {{{{ {timers_rtc_str} }}}},

    .evsys = {evsys_descriptor},

    .ccl = {ccl_descriptor},
    .portmux = {portmux_descriptor},
    
    .ext_interrupt_count = {len(groups['EXINT'])}U,
    .ext_interrupts = {{{{ {ext_ints_str} }}}},

    .uart_count = {len(groups['USART'])}U,
    .uarts = {{{{ {uarts_str} }}}},
    
    .uart8x_count = {len(groups['USART8X'])}U,
    .uarts8x = {{{{ {uarts8x_str} }}}},

    .nvm_ctrl_count = {len(nvm_ctrls_descriptors)}U,
    .nvm_ctrls = {{{{ {nvm_ctrls_str} }}}},
    .cpu_int_count = {len(cpu_ints_descriptors)}U,
    .cpu_ints = {{{{ {cpu_ints_str} }}}},
    
    .pcint_count = {len(groups['PCINT'])}U,
    .pcints = {{{{ {pcints_str} }}}},
    
    .spi_count = {len(groups['SPI'])}U,
    .spis = {{{{ {spis_str} }}}},

    .spi8x_count = {len(groups['SPI8X'])}U,
    .spis8x = {{{{ {spis8x_str} }}}},
    
    .twi_count = {len(groups['TWI'])}U,
    .twis = {{{{ {twis_str} }}}},

    .twi8x_count = {len(groups['TWI8X'])}U,
    .twis8x = {{{{ {twis8x_str} }}}},
    
    .eeprom_count = {len(eeprom_descriptors)}U,
    .eeproms = {{{{ {eeproms_str} }}}},
    
    .wdt_count = {len(groups['WDT'])}U,
    .wdts = {{{{ {wdts_str} }}}},

    .wdt8x_count = {len(groups['WDT8X'])}U,
    .wdts8x = {{{{ {wdt8x_str} }}}},

    .crc8x_count = {len(groups['CRC'] + groups['CRCSCAN'])}U,
    .crcs8x = {{{{ {crc8x_str} }}}},

    .can_count = {len(groups['CAN'])}U,
    .cans = {{{{ {cans_str} }}}},
    
    .usb_count = {len(groups['USB_DEVICE'])}U,
    .usbs = {{{{ {usbs_str} }}}},

    .psc_count = {len(groups['PSC'])}U,
    .pscs = {{{{ {pscs_str} }}}},

    .dac_count = {len(groups['DAC'])}U,
    .dacs = {{{{ {dacs_str} }}}},

    .fuse_address = {hx(next((m['start'] for m in data['memories'].get('fuses', [])), 0))},
    .lockbit_address = {hx(next((m['start'] for m in data['memories'].get('lockbits', [])), 0))},
    .signature_address = {hx(next((m['start'] for m in data['memories'].get('fuses', []) if m['type'] == 'signatures'), 0)) if next((m['start'] for m in data['memories'].get('fuses', []) if m['type'] == 'signatures'), None) is not None else hx(next((m['start'] for m in data['memories'].get('signatures', [])), 0))},

    .signature = {{ {", ".join(sigs)} }},
    .fuses = {{ {fuses_str} }},

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
        generate_header(data, Path(args.output))
        print(f"Generated header for {data['name']} at {args.output}")
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
