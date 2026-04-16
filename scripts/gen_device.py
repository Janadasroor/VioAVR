#!/usr/bin/env python3
import os, sys, json, re
from pathlib import Path

def hx(v):
    if v is None or v == "": return "0x0U"
    if isinstance(v, str): v = int(v, 0)
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
                    return hx(reg['offset']), hx(b_data['mask'])
    return "0x0U", "0xFFU"

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
        'WDT': [], 'EEPROM': [], 'SPI': [], 'TWI': [], 'EXINT': [], 'PCINT': []
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

    for k in groups: groups[k].sort()

    # 3. Memories
    sram = next((m for m in data['memories'].get('data', []) if m['type'] == 'ram'), {'size': 0})
    flash = next((m for m in data['memories'].get('prog', []) if m['type'] == 'flash'), {'size': 0})
    eeprom = next((m for m in data['memories'].get('data', []) if m['type'] == 'eeprom'), {'size': 0})
    
    # 4. Core Regs
    cpu = data['peripherals'].get('CPU', data['peripherals'].get('CPU_REGISTERS', {}))
    regs = cpu.get('registers', {})
    
    # 5. Header Content Generation
    def gen_uart(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        idx = "".join(filter(str.isdigit, p_name)) or "0"
        pr_addr, pr_bit = get_pr_info(data, f'PRUSART{idx}|PRUSART')
        return f"""{{
            .udr_address = {hx(r('UDR')['offset'])}, .ucsra_address = {hx(r('UCSRA')['offset'])}, .ucsrb_address = {hx(r('UCSRB')['offset'])}, .ucsrc_address = {hx(r('UCSRC')['offset'])}, .ubrrl_address = {hx(r('UBRR.*L')['offset'])}, .ubrrh_address = {hx(r('UBRR.*H')['offset'])},
            .ucsra_reset = {hx(r('UCSRA')['initval'])}, .ucsrb_reset = {hx(r('UCSRB')['initval'])}, .ucsrc_reset = {hx(r('UCSRC')['initval'])},
            .rx_vector_index = {next((i['index'] for i in data.get('interrupts', []) if 'RX' in (i.get('name') or i.get('caption') or '').upper() and (p_name in (i.get('name') or '').upper() or 'USART' in (i.get('name') or '').upper())), 0)}U,
            .udre_vector_index = {next((i['index'] for i in data.get('interrupts', []) if 'UDRE' in (i.get('name') or i.get('caption') or '').upper() and (p_name in (i.get('name') or '').upper() or 'USART' in (i.get('name') or '').upper())), 0)}U,
            .tx_vector_index = {next((i['index'] for i in data.get('interrupts', []) if 'TX' in (i.get('name') or i.get('caption') or '').upper() and (p_name in (i.get('name') or '').upper() or 'USART' in (i.get('name') or '').upper())), 0)}U,
            .u2x_mask = {hx(b('UCSRA', 'U2X'))}, .rxc_mask = {hx(b('UCSRA', 'RXC'))}, .txc_mask = {hx(b('UCSRA', 'TXC'))}, .udre_mask = {hx(b('UCSRA', 'UDRE'))},
            .rxen_mask = {hx(b('UCSRB', 'RXEN'))}, .txen_mask = {hx(b('UCSRB', 'TXEN'))}, .rxcie_mask = {hx(b('UCSRB', 'RXCIE'))}, .txcie_mask = {hx(b('UCSRB', 'TXCIE'))}, .udrie_mask = {hx(b('UCSRB', 'UDRIE'))},
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
            .tcnt_address = {hx(r('TCNT')['offset'])}, .ocra_address = {hx(r('OCR.*A')['offset'])}, .ocrb_address = {hx(r('OCR.*B')['offset'])}, .tifr_address = {hx(r('TIFR')['offset'])}, .timsk_address = {hx(r('TIMSK')['offset'])}, .tccra_address = {hx(r('TCCR.*A')['offset'])}, .tccrb_address = {hx(r('TCCR.*B')['offset'])}, .assr_address = {hx(r('ASSR')['offset'])},
            .tccra_reset = {hx(r('TCCR.*A')['initval'])}, .tccrb_reset = {hx(r('TCCR.*B')['initval'])}, .assr_reset = {hx(r('ASSR')['initval'])},
            .compare_a_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'{idx}' in (i.get('name') or i.get('caption') or '').upper() and ('COMPA' in (i.get('name') or i.get('caption') or '').upper() or 'OCIE0A' in (i.get('name') or '').upper() or 'Compare Match A' in (i.get('caption') or ''))), 0)}U,
            .compare_b_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'{idx}' in (i.get('name') or i.get('caption') or '').upper() and ('COMPB' in (i.get('name') or i.get('caption') or '').upper() or 'OCIE0B' in (i.get('name') or '').upper() or 'Compare Match B' in (i.get('caption') or ''))), 0)}U,
            .overflow_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'{idx}' in (i.get('name') or i.get('caption') or '').upper() and ('OVF' in (i.get('name') or i.get('caption') or '').upper() or 'Overflow' in (i.get('caption') or ''))), 0)}U,
            .ocra_pin_address = {pa_a}, .ocra_pin_bit = {pa_b}U, .ocrb_pin_address = {pb_a}, .ocrb_pin_bit = {pb_b}U,
            .t_pin_address = {t_a}, .t_pin_bit = {t_b}U,
            .tosc1_pin_address = {t1_a}, .tosc1_pin_bit = {t1_b}U, .tosc2_pin_address = {t2_a}, .tosc2_pin_bit = {t2_b}U,
            .wgm0_mask = {hx(b('TCCR.*A', 'WGM.*0'))}, .wgm2_mask = {hx(b('TCCR.*B', 'WGM.*2'))}, .cs_mask = {hx(b('TCCR.*B', 'CS'))},
            .as2_mask = {hx(b('ASSR', 'AS2'))}, .tcn2ub_mask = {hx(b('ASSR', 'TCN2UB'))},
            .compare_a_enable_mask = {hx(b('TIMSK', 'OCIEA') or b('TIMSK0', 'OCIE0A') or b('TIMSK2', 'OCIE2A'))},
            .compare_b_enable_mask = {hx(b('TIMSK', 'OCIEB') or b('TIMSK0', 'OCIE0B') or b('TIMSK2', 'OCIE2B'))},
            .overflow_enable_mask = {hx(b('TIMSK', 'TOIE') or b('TIMSK0', 'TOIE0') or b('TIMSK2', 'TOIE2'))},
            .pr_address = {get_pr_info(data, f'PRTIM{idx}')[0]}, .pr_bit = {get_pr_info(data, f'PRTIM{idx}')[1]}
        }}"""
    
    def gen_timer16(p_name, p_data):
        r = lambda n: get_reg(p_data, n) or {'offset': 0, 'initval': 0}
        b = lambda n, b_re: get_bit(r(n), b_re)
        idx = "".join(filter(str.isdigit, p_name)) or "1"
        pa_a, pa_b = get_pad_info(port_map, p_data, f'OC{idx}A', 'PORT')
        pb_a, pb_b = get_pad_info(port_map, p_data, f'OC{idx}B', 'PORT')
        t_a, t_b = get_pad_info(port_map, p_data, f'T{idx}', 'PIN')
        ic_a, ic_b = get_pad_info(port_map, p_data, f'ICP{idx}', 'PIN')
        return f"""{{
            .tcnt_address = {hx(r('TCNT.*L?')['offset'])}, .ocra_address = {hx(r('OCR.*AL?')['offset'])}, .ocrb_address = {hx(r('OCR.*BL?')['offset'])}, .icr_address = {hx(r('ICR.*L?')['offset'])}, .tifr_address = {hx(r('TIFR')['offset'])}, .timsk_address = {hx(r('TIMSK')['offset'])}, .tccra_address = {hx(r('TCCR.*A')['offset'])}, .tccrb_address = {hx(r('TCCR.*B')['offset'])}, .tccrc_address = {hx(r('TCCR.*C')['offset'])},
            .tccra_reset = {hx(r('TCCR.*A')['initval'])}, .tccrb_reset = {hx(r('TCCR.*B')['initval'])}, .tccrc_reset = {hx(r('TCCR.*C')['initval'])},
            .capture_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'{idx}' in (i.get('name') or i.get('caption') or '').upper() and ('CAPT' in (i.get('name') or i.get('caption') or '').upper() or 'Capture' in (i.get('caption') or ''))), 0)}U,
            .compare_a_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'{idx}' in (i.get('name') or i.get('caption') or '').upper() and ('COMPA' in (i.get('name') or i.get('caption') or '').upper() or 'OC' in (i.get('name') or '').upper() and 'A' in (i.get('name') or '').upper() or 'Compare Match A' in (i.get('caption') or ''))), 0)}U,
            .compare_b_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'{idx}' in (i.get('name') or i.get('caption') or '').upper() and ('COMPB' in (i.get('name') or i.get('caption') or '').upper() or 'OC' in (i.get('name') or '').upper() and 'B' in (i.get('name') or '').upper() or 'Compare Match B' in (i.get('caption') or ''))), 0)}U,
            .overflow_vector_index = {next((i['index'] for i in data.get('interrupts', []) if f'{idx}' in (i.get('name') or i.get('caption') or '').upper() and ('OVF' in (i.get('name') or i.get('caption') or '').upper() or 'Overflow' in (i.get('caption') or ''))), 0)}U,
            .ocra_pin_address = {pa_a}, .ocra_pin_bit = {pa_b}U, .ocrb_pin_address = {pb_a}, .ocrb_pin_bit = {pb_b}U,
            .icp_pin_address = {ic_a}, .icp_pin_bit = {ic_b}U,
            .t_pin_address = {t_a}, .t_pin_bit = {t_b}U,
            .wgm10_mask = {hx(b('TCCR.*A', 'WGM'))}, .wgm12_mask = {hx(b('TCCR.*B', 'WGM'))}, .cs_mask = {hx(b('TCCR.*B', 'CS'))}, .ices_mask = {hx(b('TCCR.*B', 'ICES'))}, .icnc_mask = {hx(b('TCCR.*B', 'ICNC'))},
            .capture_enable_mask = {hx(b('TIMSK.*', 'ICIE'))},
            .compare_a_enable_mask = {hx(b('TIMSK.*', 'OCIE.*A'))},
            .compare_b_enable_mask = {hx(b('TIMSK.*', 'OCIE.*B'))},
            .compare_c_enable_mask = {hx(b('TIMSK.*', 'OCIE.*C'))},
            .overflow_enable_mask = {hx(b('TIMSK.*', 'TOIE'))},
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

    uarts_str = ",\n        ".join(gen_uart(n, d) for n, d in groups['USART'])
    timers8_str = ",\n        ".join(gen_timer8(n, d) for n, d in (groups['TC8'] + groups['TC8_ASYNC']))
    timers16_str = ",\n        ".join(gen_timer16(n, d) for n, d in groups['TC16'])
    adcs_str = ",\n        ".join(gen_adc(n, d) for n, d in groups['ADC'])
    acs_str = ",\n        ".join(gen_ac(n, d) for n, d in groups['AC'])
    spis_str = ",\n        ".join(gen_spi(n, d) for n, d in groups['SPI'])
    twis_str = ",\n        ".join(gen_twi(n, d) for n, d in groups['TWI'])
    pcints_str = ",\n        ".join(gen_pcint(n, d) for n, d in groups['PCINT'])
    ext_ints_str = ",\n        ".join(gen_ext_int(n, d) for n, d in groups['EXINT'])
    eeproms_str = ",\n        ".join(gen_eeprom(n, d) for n, d in groups['EEPROM'])
    wdts_str = ",\n        ".join(gen_wdt(n, d) for n, d in groups['WDT'])
    
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
    .spmcsr_address = {get_reg_addr(cpu, 'SPMCSR|SPMCR')},
    .prr_address = {get_reg_addr(cpu, 'PRR')},
    .prr0_address = {get_reg_addr(cpu, 'PRR0')},
    .prr1_address = {get_reg_addr(cpu, 'PRR1')},
    .smcr_address = {get_reg_addr(cpu, 'SMCR')},
    .mcusr_address = {get_reg_addr(cpu, 'MCUSR')},
    
    .adc_count = {len(groups['ADC'])}U,
    .adcs = {{{{ {adcs_str} }}}},
    
    .ac_count = {len(groups['AC'])}U,
    .acs = {{{{ {acs_str} }}}},
    
    .timer8_count = {len(groups['TC8'] + groups['TC8_ASYNC'])}U,
    .timers8 = {{{{ {timers8_str} }}}},
    
    .timer16_count = {len(groups['TC16'])}U,
    .timers16 = {{{{ {timers16_str} }}}},
    
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
