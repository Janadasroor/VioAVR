#!/usr/bin/env python3
"""Generate VioAVR DeviceDescriptor C++ header from Microchip ATDF XML.

Usage:
    ./gen_device_descriptor.py path/to/ATtiny85.atdf > attiny85.hpp
    ./gen_device_descriptor.py --all
"""

import xml.etree.ElementTree as ET
import re
import sys
import os
from pathlib import Path

# ---- Register name → C++ field mappings ----

PORT_FIELDS = {
    "PIN":  "pin_address",
    "DDR":  "ddr_address",
    "PORT": "port_address",
    # AVR8X style (megaAVR-0 / tinyAVR 0/1/2):
    "IN":     "pin_address",
    "DIR":   "ddr_address",
    "DIRSET": "dirset_address",
    "DIRCLR": "dirclr_address",
    "DIRTGL": "dirtgl_address",
    "OUT":    "port_address",
    "OUTSET": "outset_address",
    "OUTCLR": "outclr_address",
    "OUTTGL": "outtgl_address",
}

TIMER8_FIELDS = {
    "TCNT": "tcnt_address",
    "TCCR0A": "tccra_address",
    "TCCR0B": "tccrb_address",
    "TCCR2A": "tccra_address",
    "TCCR2B": "tccrb_address",
    "OCR0A": "ocra_address",
    "OCR0B": "ocrb_address",
    "OCR2A": "ocra_address",
    "OCR2B": "ocrb_address",
    "TIMSK": "timsk_address",
    "TIFR":  "tifr_address",
    "ASSR":  "assr_address",
}

TIMER16_FIELDS = {
    "TCNT1L": "tcnt_address",
    "TCNT1H": "tcnt_address",
    "TCNT1":  "tcnt_address",
    "TCCR1A": "tccra_address",
    "TCCR1B": "tccrb_address",
    "TCCR1C": "tccrc_address",
    "TCCR1D": "tccrd_address",
    "TCCR1E": "tccre_address",
    "TCCR1":  "tccra_address",
    "TC1H":   "tc1h_address",
    "OCR1AL": "ocra_address",
    "OCR1AH": "ocra_address",
    "OCR1A":  "ocra_address",
    "OCR1BL": "ocrb_address",
    "OCR1BH": "ocrb_address",
    "OCR1B":  "ocrb_address",
    "OCR1CL": "ocrc_address",
    "OCR1CH": "ocrc_address",
    "OCR1C":  "ocrc_address",
    "OCR1D":  "ocrd_address",
    "DT1":    "dt1_address",
    "ICR1L":  "icr_address",
    "ICR1H":  "icr_address",
    "ICR1":   "icr_address",
    "TIMSK1": "timsk_address",
    "TIMSK":  "timsk_address",
    "TIFR1":  "tifr_address",
    "TIFR":   "tifr_address",
}

USART_FIELDS = {
    "UCSR0A": "ucsra_address",
    "UCSR0B": "ucsrb_address",
    "UCSR0C": "ucsrc_address",
    "UBRR0L": "ubrrl_address",
    "UBRR0H": "ubrrh_address",
    "UDR0":   "udr_address",
}

SPI_FIELDS = {
    "SPCR":  "spcr_address",
    "SPSR":  "spsr_address",
    "SPDR":  "spdr_address",
}

TWI_FIELDS = {
    "TWBR": "twbr_address",
    "TWCR": "twcr_address",
    "TWSR": "twsr_address",
    "TWDR": "twdr_address",
    "TWAR": "twar_address",
    "TWAMR": "twamr_address",
}

USI_FIELDS = {
    "USICR":  "usicr_address",
    "USISR":  "usisr_address",
    "USIDR":  "usidr_address",
    "USIBR":  "usibr_address",
}

ADC_FIELDS = {
    "ADCL":   "adcl_address",
    "ADCH":   "adch_address",
    "ADCSRA": "adcsra_address",
    "ADCSRB": "adcsrb_address",
    "ADMUX":  "admux_address",
    "DIDR0":  "didr0_address",
}

AC_FIELDS = {
    "ACSR": "acsr_address",
    "ACD":  "acd_address",
}

EEPROM_FIELDS = {
    "EEARL": "eearl_address",
    "EEARH": "eearh_address",
    "EEAR":  "eearl_address",  # 16-bit single register -> stored in eearl
    "EEDR":  "eedr_address",
    "EECR":  "eecr_address",
}

EXINT_FIELDS = {
    "EICRA": "eicra_address",
    "EICRB": "eicrb_address",
    "EIMSK": "eimsk_address",
    "EIFR":  "eifr_address",
}

PCINT_FIELDS = {
    "PCICR":  "pcicr_address",
    "PCIFR":  "pcifr_address",
    "PCMSK0": "pcmsk0_address",
    "PCMSK1": "pcmsk1_address",
    "PCMSK2": "pcmsk2_address",
    "PCMSK3": "pcmsk3_address",
}

WDT_FIELDS = {
    "WDTCSR": "wdtcsr_address",
    "WDTCR":  "wdtcr_address",
}

CAN_FIELDS = {
    "CANGCON": "cangcon_address",
    "CANGSTA": "cangsta_address",
    "CANGIT": "cangit_address",
    "CANGIE": "cangie_address",
    "CANEN1": "canen1_address",
    "CANEN2": "canen2_address",
    "CANIE1": "canie1_address",
    "CANIE2": "canie2_address",
    "CANSIT1": "cansit1_address",
    "CANSIT2": "cansit2_address",
    "CANBT1": "canbt1_address",
    "CANBT2": "canbt2_address",
    "CANBT3": "canbt3_address",
    "CANTCON": "cantcon_address",
    "CANTIM": "cantim_address",
    "CANTTC": "canttc_address",
    "CANTEC": "cantec_address",
    "CANREC": "canrec_address",
    "CANHPMOB": "canhpmob_address",
    "CANPAGE": "canpage_address",
    "CANSTMOB": "canstmob_address",
    "CANCDMOB": "cancdmob_address",
    "CANIDT": "canidt_address",
    "CANIDM": "canidm_address",
    "CANSTM": "canstm_address",
    "CANMSG": "canmsg_address",
}

PSC_FIELDS = {
    "PCTL": "pctl_address",
    "PSOC": "psoc_address",
    "PCNF": "pconf_address",
    "PIM": "pim_address",
    "PIFR": "pifr_address",
    "PICR": "picr_address",
    "OCRSA": "ocrsa_address",
    "OCRRA": "ocrra_address",
    "OCRSB": "ocrsb_address",
    "OCRRB": "ocrrb_address",
    "PFRC": "pfrc0a_address",
    "PFRC0A": "pfrc0a_address",
    "PFRC0B": "pfrc0b_address",
    "POM": "pom_address",
    "PCNT": "pcnt_address",
    "PLLCSR": "pllcsr_address",
}

EUSART_FIELDS = {
    "EUDR": "eudr_address",
    "EUCSRA": "eucsra_address",
    "EUCSRB": "eucsrb_address",
    "EUCSRC": "eucsrc_address",
    "MUBRR": "mubrrl_address",
}

LINUART_FIELDS = {
    "LINCR": "ctrla_address",
}

AMP_FIELDS = {
    "AMPCSR": "ampcsr_address",
}

LCD_FIELDS = {
    "LCDCRA": "lcdcra_address",
    "LCDCRB": "lcdcrb_address",
    "LCDFRR": "lcdfrr_address",
    "LCDCCR": "lcdccr_address",
    "LCDDR00": "lcddr_base_address",
    "LCDDR": "lcddr_base_address",
}

DAC_CLASSIC_FIELDS = {
    "DACON": "dacon_address",
    "DAC": "dacl_address",
}


def parse_hex(s):
    """Parse hex string like '0x3F' -> int."""
    if isinstance(s, int):
        return s
    if s is None:
        return 0
    s = s.strip()
    # Strip repeated "0x" prefixes (ATDF sometimes has "0x0x0000")
    while s.startswith("0x") or s.startswith("0X"):
        s = s[2:]
    if s.startswith("x") or s.startswith("X"):
        s = s[1:]
    if not s:
        return 0
    try:
        return int(s, 16)
    except ValueError:
        return int(s, 10) if s.lstrip("-").isdigit() else 0


def parse_int(v):
    """Parse int or hex string."""
    if isinstance(v, int):
        return v
    if isinstance(v, str):
        v = v.strip()
        if v.startswith("0x") or v.startswith("0X"):
            return int(v, 16)
        try:
            return int(v)
        except ValueError:
            return 0
    return 0


def get_register(root, module_name, instance_name, reg_name):
    """Get register address using simpler heuristics."""
    # First, try to find a peripherals base offset
    periph_base = 0
    for mod in root.findall(".//peripherals/module[@name='" + module_name + "']"):
        for inst in mod.findall("instance"):
            if instance_name and inst.get("name") != instance_name:
                continue
            for rg in inst.findall("register-group"):
                periph_base = parse_hex(rg.get("offset", "0"))
                break
    # Now search root-level module for register offset
    for mod in root.findall(".//module[@name='" + module_name + "']"):
        for rg in mod.findall("register-group"):
            rg_name = rg.get("name", "")
            if instance_name and instance_name not in rg_name and rg_name not in instance_name:
                if module_name not in rg_name:
                    continue
            for reg in rg.findall("register"):
                if reg.get("name") == reg_name:
                    base = parse_hex(rg.get("offset", "0"))
                    roff = parse_hex(reg.get("offset", "0"))
                    if base == 0 and periph_base != 0:
                        base = periph_base
                    return base + roff
    return 0


def get_module_base(root, module_name, instance_name):
    """Get the base address of a module instance from the peripherals section."""
    for mod in root.findall(".//peripherals/module[@name='" + module_name + "']"):
        for inst in mod.findall("instance"):
            if instance_name and inst.get("name") != instance_name:
                continue
            for rg in inst.findall("register-group"):
                return parse_hex(rg.get("offset", "0"))
    return 0


def extract_module_registers(root, module_name, instance_name, pattern_map):
    """Extract all matching register addresses for a module.
    Pattern keys can match register name prefixes (e.g. "PIN" matches "PINB").
    Returns dict of {cpp_field_name: address}
    """
    regs = {}
    # First, find base offset from peripherals section's instance register-group
    periph_base = 0
    for mod in root.findall(".//peripherals/module[@name='" + module_name + "']"):
        for inst in mod.findall("instance"):
            inst_name = inst.get("name", "")
            if instance_name and inst_name != instance_name:
                continue
            for rg in inst.findall("register-group"):
                periph_base = parse_hex(rg.get("offset", "0"))
                break
    # Scan all root-level module definitions
    for mod in root.findall(".//module"):
        mname = mod.get("name", "")
        if mname != module_name:
            continue
        for rg in mod.findall("register-group"):
            rg_name = rg.get("name", "")
            # Match register-group name flexibly
            if instance_name and rg_name != instance_name:
                if not (instance_name.endswith(rg_name) or rg_name.endswith(instance_name)):
                    if not (rg_name.endswith(instance_name) or instance_name in rg_name):
                        if not instance_name.startswith(rg_name):
                            continue
            base = parse_hex(rg.get("offset", "0"))
            # If root-level group has no offset, use peripherals base
            if base == 0 and periph_base != 0:
                base = periph_base
            for reg in rg.findall("register"):
                rname = reg.get("name", "")
                matched_key = None
                # First pass: exact match only
                for pat_key, cpp_field in pattern_map.items():
                    if rname == pat_key:
                        matched_key = cpp_field
                        break
                # Second pass: prefix match if no exact match
                if matched_key is None:
                    for pat_key, cpp_field in pattern_map.items():
                        if rname.startswith(pat_key):
                            # For classic AVR style (PORTB/DDRB/PINB), ensure
                            # the suffix after the prefix is exactly one letter
                            suffix = rname[len(pat_key):]
                            if not suffix or (len(suffix) == 1 and (suffix.isalpha() or suffix.isdigit())):
                                matched_key = cpp_field
                                break
                if matched_key and matched_key not in regs:
                    roff = parse_hex(reg.get("offset", "0"))
                    regs[matched_key] = base + roff
    return regs


def find_interrupt_index(root, name):
    """Find interrupt vector index by name."""
    for intr in root.findall(".//interrupts/interrupt"):
        if intr.get("name", "").upper() == name.upper():
            return parse_int(intr.get("index", "0"))
    return 0xFF


def find_interrupt_by_module(root, name, module_instance):
    """Find interrupt vector index by name, filtered by module-instance attribute."""
    for intr in root.findall(".//interrupts/interrupt"):
        if intr.get("name", "").upper() == name.upper():
            if intr.get("module-instance", "").upper() == module_instance.upper():
                return parse_int(intr.get("index", "0"))
    return 0xFF


def generate(root, output_file=sys.stdout, atdf_path=None):
    """Generate DeviceDescriptor C++ header from parsed ATDF."""
    
    device = root.find(".//device")
    if device is None:
        print("ERROR: No <device> found", file=sys.stderr)
        return False
    
    name = device.get("name", "unknown")
    architecture = device.get("architecture", "AVR8")
    family = device.get("family", "")
    
    # Normalize chip name
    chip_name = name.lower().replace("-", "").replace(" ", "")
    
    # ---- Extract memory layout ----
    flash_words = 0
    sram_bytes = 0
    sram_start = 0
    eeprom_bytes = 0
    flash_page_size = 0x80
    io_start = 0x20
    io_end = 0x5F
    
    is_avr8x = (architecture == "AVR8X")
    is_xmega = (architecture == "AVR8_XMEGA")
    
    for addr_space in root.findall(".//address-spaces/address-space"):
        as_name = addr_space.get("name", "")
        if as_name == "prog":
            for seg in addr_space.findall("memory-segment"):
                if seg.get("type") == "flash":
                    flash_size = parse_hex(seg.get("size", "0"))
                    if flash_size > flash_words * 2:
                        flash_words = flash_size // 2
                    ps = seg.get("pagesize", "0")
                    if ps != "0":
                        flash_page_size = parse_hex(ps)
        elif as_name == "data":
            for seg in addr_space.findall("memory-segment"):
                seg_type = seg.get("type", "")
                if seg_type == "ram":
                    sram_bytes = parse_hex(seg.get("size", "0"))
                    sram_start = parse_hex(seg.get("start", "0"))
                elif seg_type == "io":
                    io_start = parse_hex(seg.get("start", "0"))
                    io_size = parse_hex(seg.get("size", "0"))
                    io_end = io_start + io_size - 1
        elif as_name == "eeprom":
            for seg in addr_space.findall("memory-segment"):
                if seg.get("type") == "eeprom":
                    eeprom_bytes = parse_hex(seg.get("size", "0"))
    
    # ---- Extract mapped memory regions (AVR8X) ----
    mapped_flash_start = 0
    mapped_flash_size = 0
    mapped_eeprom_start = 0
    mapped_eeprom_size = 0
    mapped_fuses_start = 0
    mapped_fuses_size = 0
    mapped_signatures_start = 0
    mapped_signatures_size = 0
    mapped_user_signatures_start = 0
    mapped_user_signatures_size = 0
    
    for addr_space in root.findall(".//address-spaces/address-space"):
        if addr_space.get("name", "") == "data":
            for seg in addr_space.findall("memory-segment"):
                seg_type = seg.get("type", "")
                start = parse_hex(seg.get("start", "0"))
                size = parse_hex(seg.get("size", "0"))
                if seg_type == "other" and start >= 0x4000:
                    mapped_flash_start = start
                    mapped_flash_size = size
                elif seg_type == "eeprom" and start >= 0x1000:
                    mapped_eeprom_start = start
                    mapped_eeprom_size = size
                elif seg_type == "fuses":
                    mapped_fuses_start = start
                    mapped_fuses_size = size
                elif seg_type == "signatures" and mapped_signatures_start == 0:
                    mapped_signatures_start = start
                    mapped_signatures_size = size
                elif seg_type == "user_signatures":
                    mapped_user_signatures_start = start
                    mapped_user_signatures_size = size
    
    # ---- Extract interrupt vectors ----
    interrupts = []
    for intr in root.findall(".//interrupts/interrupt"):
        idx = parse_int(intr.get("index", "0"))
        iname = intr.get("name", "")
        interrupts.append((idx, iname))
    
    vector_count = max([idx for idx, _ in interrupts] + [0]) + 1 if interrupts else 0
    if is_avr8x or is_xmega or chip_name.startswith("atmega") or chip_name.startswith("at90"):
        vector_size = 4
    else:
        vector_size = 2
    
    # ---- Identify ports ----
    ports = []
    for mod in root.findall(".//peripherals/module[@name='PORT']"):
        for inst in mod.findall("instance"):
            pname = inst.get("name", "")
            if not pname.startswith("PORT"):
                continue
            port_letter = pname[4]  # B, C, D, etc.
            
            regs = extract_module_registers(root, "PORT", pname, {
                "PIN": "pin_address",
                "DDR": "ddr_address",
                "PORT": "port_address",
                "IN": "pin_address",
                "DIR": "ddr_address",
                "DIRSET": "dirset_address",
                "DIRCLR": "dirclr_address",
                "DIRTGL": "dirtgl_address",
                "OUT": "port_address",
                "OUTSET": "outset_address",
                "OUTCLR": "outclr_address",
                "OUTTGL": "outtgl_address",
                "PINCTRL": "pin_ctrl_base",
                "INTFLAGS": "intflags_address",
            })
            
            # Extract VPORT base address from the separate VPORT module instance
            # VPORT is a separate module (not a register in PORT)
            vport_name = "V" + pname  # e.g., "PORTA" -> "VPORTA"
            vport_base = get_module_base(root, "VPORT", vport_name)
            regs["vport_base"] = vport_base
            
            ports.append((pname, regs))
    
    # Classic AVR with io_start=0x0000: ATDF stores I/O offsets,
    # convert to data-space addresses (+0x20)
    if not is_avr8x and io_start == 0x0000:
        io_start = 0x0020
        io_end = 0x005F
        for pi, (pname, regs) in enumerate(ports):
            for key in regs:
                if regs[key] != 0xFFFF:
                    regs[key] += 0x20
    
    # ---- Identify peripherals ----
    # Timers
    timer8_instances = []
    timer16_instances = []
    tca_instances = []
    tcb_instances = []
    tcd_instances = []
    tce_instances = []
    rtc_instances = []
    usi_instances = []
    uart_instances = []
    uart8x_instances = []
    spi_instances = []
    spi8x_instances = []
    twi_instances = []
    twi8x_instances = []
    adc_instances = []
    adc8x_instances = []
    adc10b_instances = []
    adcea_instances = []
    ac_instances = []
    ac8x_instances = []
    eeprom_instances = []
    extint_instances = []
    pcint_instances = []  # PCINT group indices (0, 1, 2, ...)
    wdt_instances = []
    wdt8x_instances = []
    has_evsys = False
    has_clkctrl = False
    has_slpctrl = False
    has_rstctrl = False
    has_bod = False
    has_cpuint = False
    has_nvmctrl = False
    has_ccl = False
    has_portmux = False
    has_crc8x = False
    has_vref = False
    has_mvio = False
    has_usb8x = False
    
    dac_instances = []
    dac_classic_instances = []
    zcd_instances = []
    opamp_instances = []
    usb_instances = []  # Classic USB (ATmega32U4 style)
    ptc_instances = []
    
    can_instances = []
    psc_instances = []
    eusart_instances = []
    lin_instances = []
    amplifier_instances = []
    lcd_instances = []
    
    for mod in root.findall(".//peripherals/module"):
        mname = mod.get("name", "")
        for inst in mod.findall("instance"):
            iname = inst.get("name", "")
            if mname == "TC0" or (mname == "TC" and "0" in iname):
                timer8_instances.append(iname)
            elif mname in ("TC8", "TC8_ASYNC"):
                if "1" in iname:
                    timer16_instances.append(iname)
                else:
                    timer8_instances.append(iname)
            elif mname == "TC1" or (mname == "TC" and "1" in iname):
                if is_avr8x:
                    tca_instances.append(iname)
                else:
                    timer16_instances.append(iname)
            elif mname == "TC16":
                timer16_instances.append(iname)
            elif mname == "TCA":
                tca_instances.append(iname)
            elif mname == "TCB":
                tcb_instances.append(iname)
            elif mname == "TCD":
                tcd_instances.append(iname)
            elif mname == "TCE":
                tce_instances.append(iname)
            elif mname == "RTC":
                rtc_instances.append(iname)
            elif mname == "USI":
                usi_instances.append(iname)
            elif mname == "USART":
                if is_avr8x:
                    uart8x_instances.append(iname)
                else:
                    uart_instances.append(iname)
            elif mname == "SPI":
                if is_avr8x:
                    spi8x_instances.append(iname)
                else:
                    spi_instances.append(iname)
            elif mname == "TWI":
                if is_avr8x:
                    twi8x_instances.append(iname)
                else:
                    twi_instances.append(iname)
            elif mname == "ADC":
                mod_id = mod.get("id", "")
                if is_avr8x and mod_id == "adc_12b_diff_ctrl_avr_v2":
                    adcea_instances.append(iname)
                elif is_avr8x and mod_id in ("adc_10b_ctrl_avr_v1", "adc_10b_ctrl_avr_v2"):
                    adc10b_instances.append(iname)
                elif is_avr8x:
                    adc8x_instances.append(iname)
                else:
                    adc_instances.append(iname)
            elif mname == "AC":
                if is_avr8x:
                    ac8x_instances.append(iname)
                else:
                    ac_instances.append(iname)
            elif mname == "EEPROM":
                eeprom_instances.append(iname)
            elif mname == "EXINT":
                extint_instances.append(iname)
                # Populate PCINT instances from root EXINT module (PCMSK registers)
                if not is_avr8x:
                    for root_mod in root.findall(".//module[@name='EXINT']"):
                        for rg in root_mod.findall("register-group"):
                            for reg in rg.findall("register"):
                                rname = reg.get("name", "")
                                if rname.startswith("PCMSK"):
                                    idx_str = rname[5:]
                                    if idx_str.isdigit():
                                        pcint_instances.append(int(idx_str))
            elif mname == "PORT":
                # Handled separately
                pass
            elif mname == "WDT":
                if is_avr8x:
                    wdt8x_instances.append(iname)
                else:
                    wdt_instances.append(iname)
            elif mname == "EVSYS":
                has_evsys = True
            elif mname == "CLKCTRL":
                has_clkctrl = True
            elif mname == "SLPCTRL":
                has_slpctrl = True
            elif mname == "RSTCTRL":
                has_rstctrl = True
            elif mname == "BOD":
                has_bod = True
            elif mname == "CPUINT":
                has_cpuint = True
            elif mname == "NVMCTRL":
                has_nvmctrl = True
            elif mname == "CCL":
                has_ccl = True
            elif mname == "PORTMUX" or mname == "PORT_MUX":
                has_portmux = True
            elif mname == "CRC":
                has_crc8x = True
            elif mname == "VREF":
                has_vref = True
            elif mname == "DAC":
                if is_avr8x:
                    dac_instances.append(iname)
                else:
                    dac_classic_instances.append(iname)
            elif mname == "ZCD":
                zcd_instances.append(iname)
            elif mname == "OPAMP":
                opamp_instances.append(iname)
            elif mname == "MVIO":
                if is_avr8x:
                    has_mvio = True
            elif mname == "USB":
                if is_avr8x:
                    has_usb8x = True
                else:
                    usb_instances.append(iname)
            elif mname == "PTC":
                if is_avr8x:
                    ptc_instances.append(iname)
            elif mname == "CAN":
                can_instances.append(iname)
            elif mname == "PSC":
                psc_instances.append(iname)
            elif mname == "EUSART":
                if not is_avr8x:
                    eusart_instances.append(iname)
            elif mname == "LINUART":
                if not is_avr8x:
                    lin_instances.append(iname)
            elif mname == "AMP":
                if not is_avr8x:
                    amplifier_instances.append(iname)
            elif mname == "LCD":
                if not is_avr8x:
                    lcd_instances.append(iname)
    
    # ---- System Registers ----
    # SPL, SPH, SREG are at fixed addresses for classic AVR
    # For AVR8X they're in the IO space
    spl_addr = 0x5D
    sph_addr = 0x5E
    sreg_addr = 0x5F
    
    # For AVR8X, check if they're different
    if is_avr8x:
        spl_addr = get_register(root, "CPU", "CPU", "SPL")
        sph_addr = get_register(root, "CPU", "CPU", "SPH")
        sreg_addr = get_register(root, "CPU", "CPU", "SREG")
        if not sph_addr:
            # AVR8X uses 16-bit SP register (no separate SPH)
            sp_addr = get_register(root, "CPU", "CPU", "SP")
            if sp_addr:
                spl_addr = sp_addr
                sph_addr = sp_addr
        if spl_addr == 0 and sph_addr == 0 and sreg_addr == 0:
            spl_addr = 0x3D
            sph_addr = 0x3E
            sreg_addr = 0x3F
    
    # RAMPZ — search in CPU module (AVR8X: as a register, may not exist)
    rampz_addr = get_register(root, "CPU", "CPU", "RAMPZ")
    eind_addr = get_register(root, "CPU", "CPU", "EIND")
    
    # SPMCSR / SPMCR — search in CPU module or NVMCTRL or standalone SPM module
    spmcsr = 0
    for regname in ["SPMCSR", "SPMCR", "SPM_CTRL", "NVMCTRL_CTRLA"]:
        spmcsr = get_register(root, "SPM", "SPM", regname)
        if spmcsr:
            break
    if not spmcsr:
        spmcsr = get_register(root, "NVMCTRL", "NVMCTRL", "NVMCTRL_CTRLA")
    if not spmcsr:
        spmcsr = get_register(root, "CPU", "CPU", "SPMCSR")
    if not spmcsr:
        # Try a broader search
        for mod in root.findall(".//module"):
            for rg in mod.findall("register-group"):
                for reg in rg.findall("register"):
                    rname = reg.get("name", "")
                    if rname in ("SPMCSR", "SPMCR", "SPMEN"):
                        spmcsr = parse_hex(rg.get("offset", "0")) + parse_hex(reg.get("offset", "0"))
    
    # CCP (Configuration Change Protection) — AVR8X
    ccp_address = 0
    if is_avr8x:
        ccp_address = get_register(root, "CPU", "CPU", "CCP")
    
    # CLKPR (Clock Prescaler Register) — classic AVR only
    clkpr_address = 0
    if not is_avr8x:
        clkpr_address = get_register(root, "CPU", "CPU", "CLKPR")
    
    # ---- Extract signature bytes ----
    sig0 = 0
    sig1 = 0
    sig2 = 0
    for pg in root.findall(".//property-group[@name='SIGNATURES']"):
        for prop in pg.findall("property"):
            pname = prop.get("name", "")
            pval = prop.get("value", "0")
            if pname == "SIGNATURE0":
                sig0 = parse_hex(pval)
            elif pname == "SIGNATURE1":
                sig1 = parse_hex(pval)
            elif pname == "SIGNATURE2":
                sig2 = parse_hex(pval)
    
    # ---- Extract EVSYS USER register addresses and GENERATOR IDs ----
    evsys_user_map = {}  # register name → full address
    evsys_gen_map = {}   # EVSYS_GENERATOR name → value (int)
    if is_avr8x:
        # Find EVSYS base address (from peripherals instance first, then module)
        evsys_base = 0
        for mod in root.findall(".//peripherals/module[@name='EVSYS']"):
            for inst in mod.findall("instance"):
                for rg in inst.findall("register-group"):
                    evsys_base = parse_hex(rg.get("offset", "0"))
        if evsys_base == 0:
            for mod in root.findall(".//module[@name='EVSYS']"):
                for rg in mod.findall("register-group"):
                    off = rg.get("offset")
                    base = parse_hex(off) if off else 0
                    if base:
                        evsys_base = base
        # Extract USER register addresses: find the register-group that has register children
        for rg in root.findall(".//register-group"):
            if rg.get("name") == "EVSYS" and len(list(rg)) > 0:
                rg_off = rg.get("offset")
                base = parse_hex(rg_off) if rg_off else evsys_base
                if base == 0:
                    base = evsys_base
                for reg in rg.findall("register"):
                    rname = reg.get("name", "")
                    roff = parse_hex(reg.get("offset", "0"))
                    if rname.startswith("USER") and not rname.startswith("USERROW") and not rname.startswith("USEREVOUT"):
                        # Normalize: strip trailing "START" for AVR-Dx style
                        norm = rname.replace("START", "")
                        evsys_user_map[norm] = base + roff
                break  # Use first register-group with children
        # Extract GENERATOR IDs from EVSYS_GENERATOR value-group
        for vg in root.iter("value-group"):
            if vg.get("name") == "EVSYS_GENERATOR":
                for val in vg:
                    evsys_gen_map[val.get("name", "")] = parse_hex(val.get("value", "0"))
    
    # ---- Generate C++ output ----
    
    out = []
    out.append("#pragma once")
    out.append("#include \"vioavr/core/device.hpp\"")
    out.append("")
    out.append("namespace vioavr::core::devices {")
    out.append("")
    out.append(f"inline constexpr DeviceDescriptor {chip_name} {{")
    
    # Identity
    out.append(f"    .name = \"{name}\",")
    out.append(f"    .flash_words = {flash_words}U,")
    out.append(f"    .sram_bytes = {sram_bytes}U,")
    if sram_start != 0:
        out.append(f"    .sram_start = 0x{sram_start:04X}U,")
    out.append(f"    .eeprom_bytes = {eeprom_bytes}U,")
    out.append(f"    .interrupt_vector_count = {vector_count}U,")
    out.append(f"    .interrupt_vector_size = {vector_size}U,")
    out.append(f"    .flash_page_size = 0x{flash_page_size:X}U,")
    
    # Split IO range: standard (0x20-0x5F) vs extended (0x60+)
    if is_avr8x:
        low_end = min(io_end, 0x3F)
        ext_start = max(io_start, 0x40)
        out.append(f"    .register_file_range = {{ 0x0000, 0x001F }},")
        out.append(f"    .io_range = {{ 0x{io_start:04X}, 0x{low_end:04X} }},")
        if io_end >= 0x40:
            out.append(f"    .extended_io_range = {{ 0x{ext_start:04X}, 0x{io_end:04X} }},")
        else:
            out.append(f"    .extended_io_range = {{ 0x0040, 0x003F }},")
    else:
        clamped_io_end = min(io_end, 0x5F)
        out.append(f"    .register_file_range = {{ 0x0000, 0x001F }},")
        out.append(f"    .io_range = {{ 0x{io_start:04X}, 0x{clamped_io_end:04X} }},")
        if sram_start > 0x60:
            ext_io_end = min(sram_start - 1, io_end)
            out.append(f"    .extended_io_range = {{ 0x0060, 0x{ext_io_end:04X} }},")
        else:
            out.append(f"    .extended_io_range = {{ 0x0060, 0x005F }},")
    
    # AVR8X mapped memory (must come before spl_address per struct order)
    if is_avr8x and mapped_flash_size:
        out.append(f"    .mapped_flash = {{ 0x{mapped_flash_start:04X}U, 0x{mapped_flash_size:04X}U }},")
    if is_avr8x and mapped_eeprom_size:
        out.append(f"    .mapped_eeprom = {{ 0x{mapped_eeprom_start:04X}U, 0x{mapped_eeprom_size:04X}U }},")
    if is_avr8x and mapped_fuses_size:
        out.append(f"    .mapped_fuses = {{ 0x{mapped_fuses_start:04X}U, 0x{mapped_fuses_size:04X}U }},")
    if is_avr8x and mapped_signatures_size:
        out.append(f"    .mapped_signatures = {{ 0x{mapped_signatures_start:04X}U, 0x{mapped_signatures_size:04X}U }},")
    if is_avr8x and mapped_user_signatures_size:
        out.append(f"    .mapped_user_signatures = {{ 0x{mapped_user_signatures_start:04X}U, 0x{mapped_user_signatures_size:04X}U }},")
    
    out.append("")
    out.append(f"    .spl_address = 0x{spl_addr:02X}U,")
    out.append(f"    .sph_address = 0x{sph_addr:02X}U,")
    out.append(f"    .sreg_address = 0x{sreg_addr:02X}U,")
    if rampz_addr:
        out.append(f"    .rampz_address = 0x{rampz_addr:02X}U,")
    if eind_addr:
        out.append(f"    .eind_address = 0x{eind_addr:02X}U,")
    if spmcsr:
        out.append(f"    .spmcsr_address = 0x{spmcsr:02X}U,")
    if ccp_address:
        out.append(f"    .ccp_address = 0x{ccp_address:02X}U,")
    if not is_avr8x:
        out.append(f"    .sigrd_mask = 0x20U,")
        out.append(f"    .spmen_mask = 0x01U,")
    if not is_avr8x and not is_xmega:
        prr_addr = get_register(root, "CPU", "CPU", "PRR")
        if prr_addr:
            out.append(f"    .prr_address = 0x{prr_addr:02X}U,")
        smcr_addr = get_register(root, "CPU", "CPU", "SMCR")
        if smcr_addr:
            out.append(f"    .smcr_address = 0x{smcr_addr:02X}U,")
        mcusr_addr = get_register(root, "CPU", "CPU", "MCUSR")
        if mcusr_addr:
            out.append(f"    .mcusr_address = 0x{mcusr_addr:02X}U,")
        mcucr_addr = get_register(root, "CPU", "CPU", "MCUCR")
        if mcucr_addr:
            out.append(f"    .mcucr_address = 0x{mcucr_addr:02X}U,")
    if clkpr_address:
        out.append(f"    .clkpr_address = 0x{clkpr_address:02X}U,")
    if not is_avr8x and not is_xmega:
        osccal_addr = get_register(root, "CPU", "CPU", "OSCCAL")
        if osccal_addr:
            out.append(f"    .osccal_address = 0x{osccal_addr:02X}U,")
    
    out.append(f"    .cpu_frequency_hz = 16'000'000U,")
    
    # ============================================================
    # PERIPHERAL OUTPUT — must match DeviceDescriptor struct order
    # ============================================================
    #
    # Struct field order: adc/adc8x → ac/ac8x → timer8/tca → timer16/tcb
    # → tcd → rtc → evsys → ccl → portmux → vref → clkctrl → slpctrl
    # → rstctrl → syscfg → bod → mvio → extint → uart/uart8x → nvmctrl
    # → cpuint → pcint → spi/spi8x → twi/twi8x → usi → eeprom
    # → wdt/wdt8x → crc8x → dac8x → zcd → usb8x → opamp → ptc
    
    # ---- ADC (classic) / ADC8x (AVR8X) ----
    if adc_instances and not is_avr8x:
        out.append("")
        out.append(f"    .adc_count = {len(adc_instances)}U,")
        out.append(f"    .adcs = {{{{")
        for ai, aname in enumerate(adc_instances):
            regs = extract_module_registers(root, "ADC", aname, ADC_FIELDS)
            adc_idx = find_interrupt_index(root, "ADC")
            if adc_idx == 0xFF:
                adc_idx = find_interrupt_index(root, "ADC_CONVERSION_COMPLETE")
            out.append(f"        {{")
            for fn in ["adcl_address", "adch_address", "adcsra_address", "adcsrb_address",
                       "admux_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            if adc_idx != 0xFF:
                out.append(f"            .vector_index = {adc_idx}U,")
            for fn in ["didr0_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            out.append(f"        }}," if ai < len(adc_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    if is_avr8x and adc8x_instances:
        AVR8X_ADC_FIELDS = {
            "CTRLA": "ctrla_address", "CTRLB": "ctrlb_address",
            "CTRLC": "ctrlc_address", "CTRLD": "ctrld_address",
            "CTRLE": "ctrle_address", "SAMPCTRL": "sampctrl_address",
            "MUXPOS": "muxpos_address",
            "COMMAND": "command_address", "EVCTRL": "evctrl_address",
            "INTCTRL": "intctrl_address", "INTFLAGS": "intflags_address",
            "DBGCTRL": "dbgctrl_address", "TEMP": "temp_address",
            "CALIB": "calib_address",
            "RES": "res_address", "WINLT": "winlt_address", "WINHT": "winht_address",
        }
        out.append("")
        out.append(f"    .adc8x_count = {len(adc8x_instances)}U,")
        out.append(f"    .adcs8x = {{{{")
        for ai, aname in enumerate(adc8x_instances):
            regs = extract_module_registers(root, "ADC", aname, AVR8X_ADC_FIELDS)
            resrdy_idx = find_interrupt_by_module(root, "RESRDY", aname)
            wcomp_idx = find_interrupt_by_module(root, "WCOMP", aname)
            out.append(f"        {{")
            for fn in ["ctrla_address", "ctrlb_address", "ctrlc_address",
                      "ctrld_address", "ctrle_address", "sampctrl_address",
                      "muxpos_address", "command_address", "evctrl_address",
                      "intctrl_address", "intflags_address", "dbgctrl_address",
                      "calib_address", "temp_address",
                      "res_address", "winlt_address", "winht_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            if resrdy_idx != 0xFF:
                out.append(f"            .res_ready_vector_index = {resrdy_idx}U,")
            if wcomp_idx != 0xFF:
                out.append(f"            .wcomp_vector_index = {wcomp_idx}U,")
            # EVSYS user event address
            user_key = f"USERADC{ai}"
            if user_key in evsys_user_map:
                out.append(f"            .user_event_address = 0x{evsys_user_map[user_key]:02X}U,")
            # EVSYS generator IDs
            gen_key = f"ADC{ai}_RESRDY"
            if gen_key in evsys_gen_map:
                out.append(f"            .resrd_generator_id = {evsys_gen_map[gen_key]}U,")
            out.append(f"        }}," if ai < len(adc8x_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- ADC10b (AVR8X DU, 10-bit non-differential) ----
    if is_avr8x and adc10b_instances:
        ADC10B_FIELDS = {
            "CTRLA": "ctrla_address", "CTRLB": "ctrlb_address",
            "CTRLC": "ctrlc_address", "CTRLD": "ctrld_address",
            "CTRLE": "ctrle_address", "CTRLF": "ctrlf_address",
            "INTCTRL": "intctrl_address", "INTFLAGS": "intflags_address",
            "STATUS": "status_address",
            "DBGCTRL": "dbgctrl_address", "COMMAND": "command_address",
            "MUXPOS": "muxpos_address",
            "RESULT": "result_address", "SAMPLE": "sample_address",
            "WINLT": "winlt_address", "WINHT": "winht_address",
            "TEMP": "temp_address",
        }
        out.append("")
        out.append(f"    .adc10b_count = {len(adc10b_instances)}U,")
        out.append(f"    .adcs10b = {{{{")
        for ai, aname in enumerate(adc10b_instances):
            regs = extract_module_registers(root, "ADC", aname, ADC10B_FIELDS)
            resrdy_idx = find_interrupt_by_module(root, "RESRDY", aname)
            samprdy_idx = find_interrupt_by_module(root, "SAMPRDY", aname)
            wcmp_idx = find_interrupt_by_module(root, "WCOMP", aname)
            resovr_idx = find_interrupt_by_module(root, "RESOVR", aname)
            sampovr_idx = find_interrupt_by_module(root, "SAMPOVR", aname)
            trigovr_idx = find_interrupt_by_module(root, "TRIGOVR", aname)
            out.append(f"        {{")
            for fn in ["ctrla_address", "ctrlb_address", "ctrlc_address",
                      "ctrld_address", "ctrle_address", "ctrlf_address",
                      "intctrl_address", "intflags_address", "status_address",
                      "dbgctrl_address", "command_address", "muxpos_address",
                      "result_address", "sample_address",
                      "winlt_address", "winht_address", "temp_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            if resrdy_idx != 0xFF:
                out.append(f"            .resrdy_vector_index = {resrdy_idx}U,")
            if samprdy_idx != 0xFF:
                out.append(f"            .samprdy_vector_index = {samprdy_idx}U,")
            if wcmp_idx != 0xFF:
                out.append(f"            .wcmp_vector_index = {wcmp_idx}U,")
            if resovr_idx != 0xFF:
                out.append(f"            .resovr_vector_index = {resovr_idx}U,")
            if sampovr_idx != 0xFF:
                out.append(f"            .sampovr_vector_index = {sampovr_idx}U,")
            if trigovr_idx != 0xFF:
                out.append(f"            .trigovr_vector_index = {trigovr_idx}U,")
            out.append(f"        }}," if ai < len(adc10b_instances) - 1 else f"        }}")
        out.append(f"    }}}},")

    # ---- AVR8X AdcEa (EA 12-bit diff ADC v2) ----
    if is_avr8x and adcea_instances:
        ADCEA_FIELDS = {
            "CTRLA": "ctrla_address", "CTRLB": "ctrlb_address",
            "CTRLC": "ctrlc_address", "CTRLD": "ctrld_address",
            "INTCTRL": "intctrl_address", "INTFLAGS": "intflags_address",
            "STATUS": "status_address",
            "DBGCTRL": "dbgctrl_address", "CTRLE": "ctrle_address",
            "CTRLF": "ctrlf_address", "COMMAND": "command_address",
            "PGACTRL": "pgactrl_address",
            "MUXPOS": "muxpos_address", "MUXNEG": "muxneg_address",
            "RESULT": "result_address", "SAMPLE": "sample_address",
            "TEMP0": "temp0_address", "TEMP1": "temp1_address",
            "TEMP2": "temp2_address",
            "WINLT": "winlt_address", "WINHT": "winht_address",
        }
        out.append("")
        out.append(f"    .adcea_count = {len(adcea_instances)}U,")
        out.append(f"    .adceas = {{{{")
        for ai, aname in enumerate(adcea_instances):
            regs = extract_module_registers(root, "ADC", aname, ADCEA_FIELDS)
            error_idx = find_interrupt_by_module(root, "ERROR", aname)
            resrdy_idx = find_interrupt_by_module(root, "RESRDY", aname)
            samprdy_idx = find_interrupt_by_module(root, "SAMPRDY", aname)
            out.append(f"        {{")
            for fn in ["ctrla_address", "ctrlb_address", "ctrlc_address",
                      "ctrld_address", "intctrl_address", "intflags_address",
                      "status_address", "dbgctrl_address", "ctrle_address",
                      "ctrlf_address", "command_address", "pgactrl_address",
                      "muxpos_address", "muxneg_address",
                      "result_address", "sample_address",
                      "temp0_address", "temp1_address", "temp2_address",
                      "winlt_address", "winht_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            if error_idx != 0xFF:
                out.append(f"            .error_vector_index = {error_idx}U,")
            if resrdy_idx != 0xFF:
                out.append(f"            .resrdy_vector_index = {resrdy_idx}U,")
            if samprdy_idx != 0xFF:
                out.append(f"            .samprdy_vector_index = {samprdy_idx}U,")
            out.append(f"        }}," if ai < len(adcea_instances) - 1 else f"        }}")
        out.append(f"    }}}},")

    # ---- AC (classic) / AC8x (AVR8X) ----
    if ac_instances and not is_avr8x:
        out.append("")
        out.append(f"    .ac_count = {len(ac_instances)}U,")
        out.append(f"    .acs = {{{{")
        for ai, aname in enumerate(ac_instances):
            regs = extract_module_registers(root, "AC", aname, AC_FIELDS)
            ac_idx = find_interrupt_index(root, "ANALOG_COMP")
            if ac_idx == 0xFF:
                ac_idx = find_interrupt_index(root, "AC_READY")
            out.append(f"        {{")
            if regs.get("acsr_address"):
                out.append(f"            .acsr_address = 0x{regs['acsr_address']:02X}U,")
            if ac_idx != 0xFF:
                out.append(f"            .vector_index = {ac_idx}U,")
            out.append(f"        }}," if ai < len(ac_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    if is_avr8x and ac8x_instances:
        AVR8X_AC_FIELDS = {
            "CTRLA": "ctrla_address", "MUXCTRLA": "muxctrla_address",
            "INTCTRL": "intctrl_address",
            "STATUS": "status_address",
        }
        out.append("")
        out.append(f"    .ac8x_count = {len(ac8x_instances)}U,")
        out.append(f"    .acs8x = {{{{")
        for ai, aname in enumerate(ac8x_instances):
            regs = extract_module_registers(root, "AC", aname, AVR8X_AC_FIELDS)
            ac_idx = find_interrupt_by_module(root, "AC", aname)
            out.append(f"        {{")
            for fn in ["ctrla_address", "muxctrla_address",
                      "intctrl_address", "status_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            if ac_idx != 0xFF:
                out.append(f"            .vector_index = {ac_idx}U,")
            # EVSYS user event address
            user_key = f"USERAC{ai}"
            if user_key in evsys_user_map:
                out.append(f"            .user_event_address = 0x{evsys_user_map[user_key]:02X}U,")
            gen_key = f"AC{ai}_OUT"
            if gen_key in evsys_gen_map:
                out.append(f"            .out_generator_id = {evsys_gen_map[gen_key]}U,")
            out.append(f"        }}," if ai < len(ac8x_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- Timer8 (classic) ----
    if timer8_instances and not is_avr8x:
        out.append("")
        out.append(f"    .timer8_count = {len(timer8_instances)}U,")
        out.append(f"    .timers8 = {{{{")
        # Sort by timer index (TC0 before TC2, etc.)
        sorted_t8 = sorted(timer8_instances, key=lambda n: int(re.search(r'(\d+)', n).group(1)) if re.search(r'(\d+)', n) else 0)
        for ti, tname in enumerate(sorted_t8):
            regs = extract_module_registers(root, "TC8", tname, TIMER8_FIELDS)
            if not regs:
                regs = extract_module_registers(root, "TC8_ASYNC", tname, TIMER8_FIELDS)
            if not regs:
                regs = extract_module_registers(root, "TC0", tname, TIMER8_FIELDS)
            if not regs:
                regs = extract_module_registers(root, "TC", tname, TIMER8_FIELDS)
            timer_idx = 0
            m = re.search(r'(\d+)', tname)
            if m:
                timer_idx = int(m.group(1))
            ovf_idx = find_interrupt_index(root, f"TIMER{timer_idx}_OVF")
            compa_idx = find_interrupt_index(root, f"TIMER{timer_idx}_COMPA")
            compb_idx = find_interrupt_index(root, f"TIMER{timer_idx}_COMPB")
            out.append(f"        {{")
            for fn in ["tcnt_address", "ocra_address", "ocrb_address",
                       "tifr_address", "timsk_address",
                       "tccra_address", "tccrb_address", "assr_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            if compa_idx != 0xFF:
                out.append(f"            .compare_a_vector_index = {compa_idx}U,")
            if compb_idx != 0xFF:
                out.append(f"            .compare_b_vector_index = {compb_idx}U,")
            if ovf_idx != 0xFF:
                out.append(f"            .overflow_vector_index = {ovf_idx}U,")
            out.append(f"            .timer_index = {timer_idx}U,")
            out.append(f"        }}," if ti < len(timer8_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- Timer16 (classic) ----
    if timer16_instances and not is_avr8x:
        out.append("")
        out.append(f"    .timer16_count = {len(timer16_instances)}U,")
        out.append(f"    .timers16 = {{{{")
        for ti, tname in enumerate(timer16_instances):
            regs = extract_module_registers(root, "TC16", tname, TIMER16_FIELDS)
            if not regs:
                regs = extract_module_registers(root, "TC1", tname, TIMER16_FIELDS)
            if not regs:
                regs = extract_module_registers(root, "TC", tname, TIMER16_FIELDS)
            if not regs:
                regs = extract_module_registers(root, "TC8", tname, TIMER16_FIELDS)
            timer_idx = 1
            m = re.search(r'(\d+)', tname)
            if m:
                timer_idx = int(m.group(1))
            ovf_idx = find_interrupt_index(root, f"TIMER{timer_idx}_OVF")
            compa_idx = find_interrupt_index(root, f"TIMER{timer_idx}_COMPA")
            compb_idx = find_interrupt_index(root, f"TIMER{timer_idx}_COMPB")
            compd_idx = find_interrupt_index(root, f"TIMER{timer_idx}_COMPD")
            capt_idx = find_interrupt_index(root, f"TIMER{timer_idx}_CAPT")
            out.append(f"        {{")
            for fn in ["tcnt_address", "ocra_address", "ocrb_address",
                       "ocrc_address", "icr_address",
                       "tifr_address", "timsk_address",
                       "tccra_address", "tccrb_address", "tccrc_address",
                       "tccrd_address", "tccre_address",
                       "tc1h_address", "ocrd_address", "dt1_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            if capt_idx != 0xFF:
                out.append(f"            .capture_vector_index = {capt_idx}U,")
            if compa_idx != 0xFF:
                out.append(f"            .compare_a_vector_index = {compa_idx}U,")
            if compb_idx != 0xFF:
                out.append(f"            .compare_b_vector_index = {compb_idx}U,")
            if compd_idx != 0xFF:
                out.append(f"            .compare_d_vector_index = {compd_idx}U,")
            if ovf_idx != 0xFF:
                out.append(f"            .overflow_vector_index = {ovf_idx}U,")
            out.append(f"        }}," if ti < len(timer16_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- AVR8X TCA (replaces classic Timer8/16 for AVR8X) ----
    if is_avr8x and tca_instances:
        AVR8X_TCA_FIELDS = {
            "CTRLA": "ctrla_address", "CTRLB": "ctrlb_address", "CTRLC": "ctrlc_address",
            "CTRLD": "ctrld_address", "CTRLECLR": "ctrleclr_address", "CTRLESET": "ctrleset_address",
            "CTRLFCLR": "ctrlfclr_address", "CTRLFSET": "ctrlfset_address",
            "EVCTRL": "evctrl_address", "INTCTRL": "intctrl_address", "INTFLAGS": "intflags_address",
            "DBGCTRL": "dbgctrl_address", "TEMP": "temp_address",
            "CNT": "tcnt_address", "PER": "period_address", "PERBUF": "perbuf_address",
            "CMP0": "cmp0_address", "CMP1": "cmp1_address", "CMP2": "cmp2_address",
            "CMP0BUF": "cmp0buf_address", "CMP1BUF": "cmp1buf_address", "CMP2BUF": "cmp2buf_address",
        }
        out.append("")
        out.append(f"    .tca_count = {len(tca_instances)}U,")
        out.append(f"    .timers_tca = {{{{")
        for ti, tname in enumerate(tca_instances):
            regs = extract_module_registers(root, "TCA", tname, AVR8X_TCA_FIELDS)
            luf_idx = find_interrupt_by_module(root, "LUNF", tname)
            ovf_idx = find_interrupt_by_module(root, "OVF", tname)
            hunf_idx = find_interrupt_by_module(root, "HUNF", tname)
            cmp0_idx = find_interrupt_by_module(root, "CMP0", tname)
            lcmp0_idx = find_interrupt_by_module(root, "LCMP0", tname)
            cmp1_idx = find_interrupt_by_module(root, "CMP1", tname)
            lcmp1_idx = find_interrupt_by_module(root, "LCMP1", tname)
            cmp2_idx = find_interrupt_by_module(root, "CMP2", tname)
            lcmp2_idx = find_interrupt_by_module(root, "LCMP2", tname)
            use_luf = luf_idx if luf_idx != 0xFF else ovf_idx
            out.append(f"        {{")
            for fn in ["ctrla_address", "ctrlb_address", "ctrlc_address",
                      "ctrld_address", "ctrleclr_address", "ctrleset_address",
                      "ctrlfclr_address", "ctrlfset_address",
                      "evctrl_address", "intctrl_address", "intflags_address",
                      "dbgctrl_address", "temp_address",
                      "tcnt_address", "period_address",
                      "cmp0_address", "cmp1_address", "cmp2_address",
                      "perbuf_address",
                      "cmp0buf_address", "cmp1buf_address", "cmp2buf_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            if use_luf != 0xFF:
                out.append(f"            .luf_ovf_vector_index = {use_luf}U,")
            use_cmp0 = lcmp0_idx if lcmp0_idx != 0xFF else cmp0_idx
            use_cmp1 = lcmp1_idx if lcmp1_idx != 0xFF else cmp1_idx
            use_cmp2 = lcmp2_idx if lcmp2_idx != 0xFF else cmp2_idx
            if use_cmp0 != 0xFF:
                out.append(f"            .cmp0_vector_index = {use_cmp0}U,")
            if use_cmp1 != 0xFF:
                out.append(f"            .cmp1_vector_index = {use_cmp1}U,")
            if use_cmp2 != 0xFF:
                out.append(f"            .cmp2_vector_index = {use_cmp2}U,")
            if hunf_idx != 0xFF:
                out.append(f"            .hunf_vector_index = {hunf_idx}U,")
            # EVSYS user event address for TCA
            user_key = f"USERTCA{ti}"
            if user_key in evsys_user_map:
                out.append(f"            .user_event_address = 0x{evsys_user_map[user_key]:02X}U,")
            # EVSYS generator IDs
            ovf_gen = f"TCA{ti}_OVF_LUNF"
            cmp0_gen = f"TCA{ti}_CMP0"
            cmp1_gen = f"TCA{ti}_CMP1"
            cmp2_gen = f"TCA{ti}_CMP2"
            if ovf_gen in evsys_gen_map:
                out.append(f"            .ovf_generator_id = {evsys_gen_map[ovf_gen]}U,")
            if cmp0_gen in evsys_gen_map:
                out.append(f"            .cmp0_generator_id = {evsys_gen_map[cmp0_gen]}U,")
            if cmp1_gen in evsys_gen_map:
                out.append(f"            .cmp1_generator_id = {evsys_gen_map[cmp1_gen]}U,")
            if cmp2_gen in evsys_gen_map:
                out.append(f"            .cmp2_generator_id = {evsys_gen_map[cmp2_gen]}U,")
            out.append(f"        }}," if ti < len(tca_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- AVR8X TCB ----
    if is_avr8x and tcb_instances:
        AVR8X_TCB_FIELDS = {
            "CTRLA": "ctrla_address", "CTRLB": "ctrlb_address",
            "EVCTRL": "evctrl_address", "INTCTRL": "intctrl_address",
            "INTFLAGS": "intflags_address", "STATUS": "status_address",
            "DBGCTRL": "dbgctrl_address", "TEMP": "temp_address",
            "CNT": "cnt_address", "CCMP": "ccmp_address",
        }
        out.append("")
        out.append(f"    .tcb_count = {len(tcb_instances)}U,")
        out.append(f"    .timers_tcb = {{{{")
        for ti, tname in enumerate(tcb_instances):
            regs = extract_module_registers(root, "TCB", tname, AVR8X_TCB_FIELDS)
            int_idx = find_interrupt_by_module(root, "INT", tname)
            out.append(f"        {{")
            for fn in ["ctrla_address", "ctrlb_address", "evctrl_address",
                      "intctrl_address", "intflags_address", "status_address",
                      "dbgctrl_address", "temp_address",
                      "cnt_address", "ccmp_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            if int_idx != 0xFF:
                out.append(f"            .vector_index = {int_idx}U,")
            # EVSYS user event address
            user_key = f"USERTCB{ti}"
            if user_key in evsys_user_map:
                out.append(f"            .user_event_address = 0x{evsys_user_map[user_key]:02X}U,")
            # EVSYS generator ID
            gen_key = f"TCB{ti}_CAPT"
            if gen_key in evsys_gen_map:
                out.append(f"            .capt_generator_id = {evsys_gen_map[gen_key]}U,")
            out.append(f"        }}," if ti < len(tcb_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- AVR8X TCD ----
    if is_avr8x and tcd_instances:
        AVR8X_TCD_FIELDS = {
            "CTRLA": "ctrla_address", "CTRLB": "ctrlb_address",
            "CTRLC": "ctrlc_address", "CTRLD": "ctrld_address",
            "CTRLE": "ctrle_address",
            "EVCTRLA": "evctrla_address", "EVCTRLB": "evctrlB_address",
            "INTCTRL": "intctrl_address", "INTFLAGS": "intflags_address",
            "STATUS": "status_address",
            "INPUTCTRLA": "inputctrla_address", "INPUTCTRLB": "inputctrlB_address",
            "FAULTCTRL": "faultctrl_address",
            "DLYCTRL": "dlyctrl_address", "DLYVAL": "dlyval_address",
            "DITCTRL": "ditctrl_address", "DITVAL": "ditval_address",
            "DBGCTRL": "dbgctrl_address",
            "CAPTUREA": "capturea_address", "CAPTUREB": "captureb_address",
            "CMPASET": "cmpaset_address", "CMPACLR": "cmpacl_address",
            "CMPBSET": "cmpbset_address", "CMPBCLR": "cmpbcl_address",
        }
        TCD_FIELD_ORDER = [
            "ctrla_address", "ctrlb_address", "ctrlc_address", "ctrld_address", "ctrle_address",
            "evctrla_address", "evctrlB_address",
            "intctrl_address", "intflags_address", "status_address",
            "inputctrla_address", "inputctrlB_address",
            "faultctrl_address",
            "dlyctrl_address", "dlyval_address",
            "ditctrl_address", "ditval_address",
            "dbgctrl_address",
            "capturea_address", "captureb_address",
            "cmpaset_address", "cmpacl_address",
            "cmpbset_address", "cmpbcl_address",
        ]
        out.append("")
        out.append(f"    .tcd_count = {len(tcd_instances)}U,")
        out.append(f"    .timers_tcd = {{{{")
        for ti, tname in enumerate(tcd_instances):
            base_addr = 0
            for mod in root.findall(f".//peripherals/module[@name='TCD']"):
                for inst in mod.findall("instance"):
                    if inst.get("name") == tname:
                        for rg in inst.findall("register-group"):
                            base_addr = parse_hex(rg.get("offset", "0"))
            regs = extract_module_registers(root, "TCD", tname, AVR8X_TCD_FIELDS)
            ovf_idx = find_interrupt_by_module(root, "OVF", tname)
            trig_idx = find_interrupt_by_module(root, "TRIG", tname)
            out.append(f"        {{")
            if base_addr:
                out.append(f"            .base_address = 0x{base_addr:04X}U,")
            for fn in TCD_FIELD_ORDER:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:04X}U,")
            if ovf_idx != 0xFF:
                out.append(f"            .ovf_vector_index = {ovf_idx}U,")
            if trig_idx != 0xFF:
                out.append(f"            .trig_vector_index = {trig_idx}U,")
            out.append(f"        }}," if ti < len(tcd_instances) - 1 else f"        }}")
        out.append(f"    }}}},")

    # ---- AVR8X TCE ----    
    if is_avr8x and tce_instances:
        TCE_FIELDS = {
            "CTRLA": "ctrla_address", "CTRLB": "ctrlb_address",
            "CTRLC": "ctrlc_address",
            "CTRLECLR": "ctrleclr_address", "CTRLESET": "ctrleset_address",
            "CTRLFCLR": "ctrlfclr_address", "CTRLFSET": "ctrlfset_address",
            "EVGENCTRL": "evgenctrl_address", "EVCTRL": "evctrl_address",
            "INTCTRL": "intctrl_address", "INTFLAGS": "intflags_address",
            "DBGCTRL": "dbgctrl_address", "TEMP": "temp_address",
            "CNT": "tcnt_address", "PER": "period_address",
            "CMP0": "cmp0_address", "CMP1": "cmp1_address", "CMP2": "cmp2_address",
            "PERBUF": "perbuf_address",
            "CMP0BUF": "cmp0buf_address", "CMP1BUF": "cmp1buf_address",
            "CMP2BUF": "cmp2buf_address",
        }
        out.append("")
        out.append(f"    .tce_count = {len(tce_instances)}U,")
        out.append(f"    .timers_tce = {{{{")
        for ti, tname in enumerate(tce_instances):
            regs = extract_module_registers(root, "TCE", tname, TCE_FIELDS)
            ovf_idx = find_interrupt_by_module(root, "OVF", tname)
            cmp0_idx = find_interrupt_by_module(root, "CMP0", tname)
            cmp1_idx = find_interrupt_by_module(root, "CMP1", tname)
            cmp2_idx = find_interrupt_by_module(root, "CMP2", tname)
            out.append(f"        {{")
            for fn in ["ctrla_address", "ctrlb_address", "ctrlc_address",
                      "ctrleclr_address", "ctrleset_address",
                      "ctrlfclr_address", "ctrlfset_address",
                      "evgenctrl_address", "evctrl_address",
                      "intctrl_address", "intflags_address",
                      "dbgctrl_address", "temp_address",
                      "tcnt_address", "period_address",
                      "cmp0_address", "cmp1_address", "cmp2_address",
                      "perbuf_address", "cmp0buf_address",
                      "cmp1buf_address", "cmp2buf_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            if ovf_idx != 0xFF:
                out.append(f"            .ovf_vector_index = {ovf_idx}U,")
            if cmp0_idx != 0xFF:
                out.append(f"            .cmp0_vector_index = {cmp0_idx}U,")
            if cmp1_idx != 0xFF:
                out.append(f"            .cmp1_vector_index = {cmp1_idx}U,")
            if cmp2_idx != 0xFF:
                out.append(f"            .cmp2_vector_index = {cmp2_idx}U,")
            out.append(f"        }}," if ti < len(tce_instances) - 1 else f"        }}")
        out.append(f"    }}}},")

    # ---- AVR8X RTC ----
    if is_avr8x and rtc_instances:
        AVR8X_RTC_FIELDS = {
            "CTRLA": "ctrla_address", "STATUS": "status_address",
            "INTCTRL": "intctrl_address", "INTFLAGS": "intflags_address",
            "TEMP": "temp_address", "DBGCTRL": "dbgctrl_address",
            "CLKSEL": "clksel_address",
            "CNT": "cnt_address", "PER": "per_address", "CMP": "cmp_address",
            "PITCTRLA": "pitctrla_address", "PITSTATUS": "pitstatus_address",
            "PITINTCTRL": "pitintctrl_address", "PITINTFLAGS": "pitintflags_address",
        }
        out.append("")
        out.append(f"    .rtc_count = {len(rtc_instances)}U,")
        out.append(f"    .timers_rtc = {{{{")
        for ri, rname in enumerate(rtc_instances):
            regs = extract_module_registers(root, "RTC", rname, AVR8X_RTC_FIELDS)
            cnt_idx = find_interrupt_by_module(root, "CNT", rname)
            pit_idx = find_interrupt_by_module(root, "PIT", rname)
            out.append(f"        {{")
            for fn in ["ctrla_address", "status_address", "intctrl_address",
                      "intflags_address", "temp_address", "dbgctrl_address",
                      "clksel_address", "cnt_address", "per_address", "cmp_address",
                      "pitctrla_address", "pitstatus_address",
                      "pitintctrl_address", "pitintflags_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            if cnt_idx != 0xFF:
                out.append(f"            .ovf_vector_index = {cnt_idx}U,")
            if pit_idx != 0xFF:
                out.append(f"            .pit_vector_index = {pit_idx}U,")
            # EVSYS generator IDs (RTC has no per-instance USER register)
            ovf_gen = "RTC_OVF"
            cmp_gen = "RTC_CMP"
            if ovf_gen in evsys_gen_map:
                out.append(f"            .ovf_generator_id = {evsys_gen_map[ovf_gen]}U,")
            if cmp_gen in evsys_gen_map:
                out.append(f"            .cmp_generator_id = {evsys_gen_map[cmp_gen]}U,")
            out.append(f"        }}," if ri < len(rtc_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- AVR8X EVSYS ----
    if is_avr8x and has_evsys:
        evsys_regs = extract_module_registers(root, "EVSYS", "EVSYS", {
            "ASYNCSTROBE": "strobe_address", "SYNCSTROBE": "strobe_address",
            "ASYNCCH0": "channels_address", "SYNCCH0": "channels_address",
            "ASYNCUSER0": "users_address", "SYNCUSER0": "users_address",
        })
        ch_count = 0
        us_count = 0
        for mod in root.findall(".//module[@name='EVSYS']"):
            for rg in mod.findall("register-group"):
                for reg in rg.findall("register"):
                    rname = reg.get("name", "")
                    if rname.startswith("ASYNCCH") or rname.startswith("SYNCCH"):
                        ch_count += 1
                    if rname.startswith("ASYNCUSER") or rname.startswith("SYNCUSER"):
                        us_count += 1
        out.append("")
        out.append(f"    .evsys = {{")
        if evsys_regs.get("strobe_address"):
            out.append(f"        .strobe_address = 0x{evsys_regs['strobe_address']:02X}U,")
        if evsys_regs.get("channels_address"):
            out.append(f"        .channels_address = 0x{evsys_regs['channels_address']:02X}U,")
        if evsys_regs.get("users_address"):
            out.append(f"        .users_address = 0x{evsys_regs['users_address']:02X}U,")
        if ch_count:
            out.append(f"        .channel_count = {ch_count}U,")
        if us_count:
            out.append(f"        .user_count = {us_count}U,")
        # EVOUT on AVR8X: users start at USER2, 2 channels by default
        if is_avr8x and us_count >= 4:
            out.append(f"        .evout_user_start = 2U,")
            out.append(f"        .evout_user_count = 2U,")
        out.append(f"    }},")
    
    # ---- AVR8X CCL ----
    if is_avr8x and has_ccl:
        ccl_regs = extract_module_registers(root, "CCL", "CCL", {
            "CTRLA": "ctrla_address",
            "SEQCTRL0": "seqctrl0_address",
        })
        lut_count = 0
        for mod in root.findall(".//module[@name='CCL']"):
            for rg in mod.findall("register-group"):
                for reg in rg.findall("register"):
                    rname = reg.get("name", "")
                    if rname.startswith("LUT") and rname.endswith("CTRLA"):
                        lut_count += 1
        
        out.append("")
        out.append(f"    .ccl = {{")
        if ccl_regs.get("ctrla_address"):
            out.append(f"        .ctrla_address = 0x{ccl_regs['ctrla_address']:02X}U,")
        if ccl_regs.get("seqctrl0_address"):
            out.append(f"        .seqctrl_addresses = {{ 0x{ccl_regs['seqctrl0_address']:02X}U, 0x0U, 0x0U, 0x0U }},")
        if lut_count:
            out.append(f"        .lut_count = {lut_count}U,")
            out.append(f"        .luts = {{")
            for li in range(lut_count):
                lut_regs = extract_module_registers(root, "CCL", "CCL", {
                    f"LUT{li}CTRLA": "ctrla",
                    f"LUT{li}CTRLB": "ctrlb",
                    f"LUT{li}CTRLC": "ctrlc",
                    f"TRUTH{li}": "truth",
                })
                out_pin_addr = 0
                out_pin_bit = 0xFF
                for mod in root.findall(".//peripherals/module[@name='CCL']"):
                    for inst in mod.findall("instance"):
                        for sig in inst.findall(".//signal"):
                            sgroup = sig.get("group", "")
                            if sgroup == f"LUT{li}_OUT":
                                pad = sig.get("pad", "")
                                if pad:
                                    port_letter = pad[1] if len(pad) > 1 else 'A'
                                    bit = int(pad[2:]) if len(pad) > 2 else 0
                                    for port_name, port_regs in ports:
                                        if port_name[4] == port_letter:
                                            out_pin_addr = port_regs.get("port_address", 0)
                                            out_pin_bit = bit
                                            break
                                    if out_pin_addr == 0:
                                        port_idx = ord(port_letter) - ord('A')
                                        out_pin_addr = 0x400 + port_idx * 0x40
                out.append(f"        {{")
                if lut_regs.get("ctrla"):
                    out.append(f"            .ctrla_address = 0x{lut_regs['ctrla']:02X}U,")
                if lut_regs.get("ctrlb"):
                    out.append(f"            .ctrlb_address = 0x{lut_regs['ctrlb']:02X}U,")
                if lut_regs.get("ctrlc"):
                    out.append(f"            .ctrlc_address = 0x{lut_regs['ctrlc']:02X}U,")
                if lut_regs.get("truth"):
                    out.append(f"            .truth_address = 0x{lut_regs['truth']:02X}U,")
                out.append(f"            .generator_id = 0U,")
                if out_pin_addr:
                    out.append(f"            .output_pin_address = 0x{out_pin_addr:02X}U,")
                if out_pin_bit != 0xFF:
                    out.append(f"            .output_bit_index = {out_pin_bit}U,")
                out.append(f"        }},")
            out.append(f"        }},")
        out.append(f"    }},")
    
    # ---- AVR8X PORTMUX ----
    if is_avr8x and has_portmux:
        pmux_regs = extract_module_registers(root, "PORTMUX", "PORTMUX", {
            "CTRLA": "ctrla", "CTRLB": "ctrlb", "CTRLC": "ctrlc", "CTRLD": "ctrld",
        })
        route_regs = extract_module_registers(root, "PORTMUX", "PORTMUX", {
            "EVSYSROUTEA": "evsysroutea",
            "CCLROUTEA": "cclroutea",
            "USARTROUTEA": "usartroutea",
            "USARTROUTEB": "usartrouteb",
            "SPIROUTEA": "spiroutea",
            "TWIROUTEA": "twiroutea",
            "TCAROUTEA": "tcaroutea",
            "TCBROUTEA": "tcbroutea",
            "TCDROUTEA": "tcdroutea",
            "ACROUTEA": "acroutea",
            "ZCDROUTEA": "zcdroutea",
        })
        out.append("")
        out.append(f"    .portmux = {{")
        # Map routing registers to existing PortMuxDescriptor fields
        # Must match PortMuxDescriptor declaration order:
        # twispiroutea → usartroutea → evoutroutea → tcaroutea → tcbroutea
        if not route_regs.get("usartroutea") and pmux_regs.get("ctrla"):
            # Fallback for megaAVR-0 style (CTRLA = TWISPI, CTRLB = USART)
            out.append(f"        .twispiroutea_address = 0x{pmux_regs['ctrla']:02X}U,")
            out.append(f"        .usartroutea_address = 0x{pmux_regs['ctrlb']:02X}U,")
        else:
            out.append(f"        .twispiroutea_address = 0U,")
            if route_regs.get("usartroutea"):
                out.append(f"        .usartroutea_address = 0x{route_regs['usartroutea']:02X}U,")
        if route_regs.get("evsysroutea"):
            out.append(f"        .evoutroutea_address = 0x{route_regs['evsysroutea']:02X}U,")
        if route_regs.get("tcaroutea"):
            out.append(f"        .tcaroutea_address = 0x{route_regs['tcaroutea']:02X}U,")
        if route_regs.get("tcbroutea"):
            out.append(f"        .tcbroutea_address = 0x{route_regs['tcbroutea']:02X}U,")
        out.append(f"        .usart = {{}},")
        out.append(f"        .spi = {{}},")
        out.append(f"        .twi = {{}},")
        # ATtiny AVR8X chips have fixed (reversed) TCA WO-to-pin mapping
        if "attiny" in chip_name and tca_instances:
            out.append(f"        .tca_wo_pin_bit = {{3, 2, 1, 0, 5, 4}},")
        out.append(f"    }},")
    
    # ---- AVR8X VREF ----
    if is_avr8x and has_vref:
        vref_regs = extract_module_registers(root, "VREF", "VREF", {
            "CTRLA": "ctrla_address", "CTRLB": "ctrlb_address",
        })
        out.append("")
        out.append(f"    .vref = {{")
        if vref_regs.get("ctrla_address"):
            out.append(f"        .ctrla_address = 0x{vref_regs['ctrla_address']:02X}U,")
        if vref_regs.get("ctrlb_address"):
            out.append(f"        .ctrlb_address = 0x{vref_regs['ctrlb_address']:02X}U,")
        out.append(f"    }},")
    
    # ---- AVR8X CLKCTRL ----
    if is_avr8x and has_clkctrl:
        clk_regs = extract_module_registers(root, "CLKCTRL", "CLKCTRL", {
            "MCLKCTRLA": "ctrla_address", "MCLKCTRLB": "ctrlb_address",
            "MCLKLOCK": "mclklock_address", "MCLKSTATUS": "mclkstatus_address",
            "OSCHFCTRLA": "osc20mctrla_address",
            "OSC20MCTRLA": "osc20mctrla_address",
            "OSCHFTUNE": "osc20mcalib_address",
            "OSC20MCALIB": "osc20mcalib_address",
            "OSC32KCTRLA": "osc32kctrla_address",
            "XOSC32KCTRLA": "xosc32kctrla_address",
            "XOSCHFCTRLA": "xoschfctrla_address",
            "MCLKCTRLC": "mclkctrlc_address",
            "MCLKINTCTRL": "mclkintctrl_address",
            "MCLKINTFLAGS": "mclkintflags_address",
            "MCLKTIMEBASE": "mclktimebase_address",
            "OSCHFSTATUS": "oschfstatus_address",
            "USBPLLSTATUS": "usbpllstatus_address",
            "PLLCTRLA": "pllctrla_address",
        })
        out.append("")
        out.append(f"    .clkctrl = {{")
        for fn in ["ctrla_address", "ctrlb_address", "mclklock_address",
                  "mclkstatus_address", "osc20mctrla_address",
                  "osc20mcalib_address", "osc32kctrla_address",
                  "xosc32kctrla_address", "xoschfctrla_address",
                  "mclkctrlc_address", "mclkintctrl_address",
                  "mclkintflags_address", "mclktimebase_address",
                  "oschfstatus_address", "usbpllstatus_address",
                  "pllctrla_address"]:
            if clk_regs.get(fn):
                out.append(f"        .{fn} = 0x{clk_regs[fn]:02X}U,")
        out.append(f"    }},")
    
    # ---- AVR8X SLPCTRL ----
    if is_avr8x and has_slpctrl:
        slp_regs = extract_module_registers(root, "SLPCTRL", "SLPCTRL", {
            "CTRLA": "ctrla_address",
        })
        out.append("")
        out.append(f"    .slpctrl = {{")
        if slp_regs.get("ctrla_address"):
            out.append(f"        .ctrla_address = 0x{slp_regs['ctrla_address']:02X}U,")
        out.append(f"    }},")
    
    # ---- AVR8X RSTCTRL ----
    if is_avr8x and has_rstctrl:
        rst_regs = extract_module_registers(root, "RSTCTRL", "RSTCTRL", {
            "RSTFR": "rstfr_address", "SWRR": "swrr_address",
        })
        out.append("")
        out.append(f"    .rstctrl = {{")
        if rst_regs.get("rstfr_address"):
            out.append(f"        .rstfr_address = 0x{rst_regs['rstfr_address']:02X}U,")
        if rst_regs.get("swrr_address"):
            out.append(f"        .swrr_address = 0x{rst_regs['swrr_address']:02X}U,")
        out.append(f"    }},")
    
    # ---- AVR8X SYSCFG ----
    if is_avr8x:
        syscfg_regs = extract_module_registers(root, "SYSCFG", "SYSCFG", {
            "REVES": "reves_address",
        })
        if syscfg_regs.get("reves_address"):
            out.append("")
            out.append(f"    .syscfg = {{")
            out.append(f"        .reves_address = 0x{syscfg_regs['reves_address']:02X}U,")
            out.append(f"    }},")
    
    # ---- AVR8X BOD ----
    if is_avr8x and has_bod:
        bod_regs = extract_module_registers(root, "BOD", "BOD", {
            "CTRLA": "ctrla_address", "CTRLB": "ctrlb_address",
            "VLMCTRLA": "vlmctrla_address",
            "INTCTRL": "intctrl_address", "INTFLAGS": "intflags_address",
            "STATUS": "status_address",
        })
        vlm_idx = find_interrupt_index(root, "VLM")
        out.append("")
        out.append(f"    .bod = {{")
        for fn in ["ctrla_address", "ctrlb_address", "vlmctrla_address",
                  "intctrl_address", "intflags_address", "status_address"]:
            if bod_regs.get(fn):
                out.append(f"        .{fn} = 0x{bod_regs[fn]:02X}U,")
        if vlm_idx != 0xFF:
            out.append(f"        .vlm_vector_index = {vlm_idx}U,")
        out.append(f"    }},")
    
    # ---- AVR8X MVIO ----
    if is_avr8x and has_mvio:
        mvio_regs = extract_module_registers(root, "MVIO", "MVIO", {
            "INTCTRL": "intctrl_address",
            "INTFLAGS": "intflags_address",
            "STATUS": "status_address",
        })
        mvio_idx = find_interrupt_index(root, "MVIO")
        if mvio_idx == 0xFF:
            mvio_idx = find_interrupt_by_module(root, "MVIO", "MVIO")
        out.append("")
        out.append(f"    .mvio = {{")
        for fn in ["intctrl_address", "intflags_address", "status_address"]:
            if mvio_regs.get(fn):
                out.append(f"        .{fn} = 0x{mvio_regs[fn]:02X}U,")
        if mvio_idx != 0xFF:
            out.append(f"        .vector_index = {mvio_idx}U,")
        out.append(f"    }},")
    
    # ---- EXINT (classic only - AVR8X handles through PORT) ----
    if extint_instances and not is_avr8x:
        out.append("")
        out.append(f"    .ext_interrupt_count = {len(extint_instances)}U,")
        out.append(f"    .ext_interrupts = {{{{")
        for ei, ename in enumerate(extint_instances):
            regs = extract_module_registers(root, "EXINT", ename, EXINT_FIELDS)
            out.append(f"        {{")
            for fn in ["eicra_address", "eicrb_address", "eimsk_address", "eifr_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            int0_idx = find_interrupt_index(root, "INT0")
            if int0_idx != 0xFF:
                out.append(f"            .vector_indices = {{ {int0_idx}U, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }},")
            else:
                int0_idx = find_interrupt_index(root, "INT1")
                if int0_idx != 0xFF:
                    out.append(f"            .vector_indices = {{ 0xFF, {int0_idx}U, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }},")
            out.append(f"        }}," if ei < len(extint_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- Classic UART / AVR8X UART8x ----
    if uart_instances and not is_avr8x:
        out.append("")
        out.append(f"    .uart_count = {len(uart_instances)}U,")
        out.append(f"    .uarts = {{{{")
        for ui, uname in enumerate(uart_instances):
            regs = extract_module_registers(root, "USART", uname, USART_FIELDS)
            uart_num = 0
            m = re.search(r'(\d+)', uname)
            if m:
                uart_num = int(m.group(1))
            uart_idx = uart_num
            out.append(f"        {{")
            for field_name in ["udr_address", "ucsra_address", "ucsrb_address",
                               "ucsrc_address", "ubrrl_address", "ubrrh_address"]:
                if regs.get(field_name):
                    out.append(f"            .{field_name} = 0x{regs[field_name]:02X}U,")
            out.append(f"        }}," if ui < len(uart_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    if is_avr8x and uart8x_instances:
        AVR8X_USART_FIELDS = {
            "CTRLA": "ctrla_address", "CTRLB": "ctrlb_address",
            "CTRLC": "ctrlc_address", "CTRLD": "ctrld_address",
            "STATUS": "status_address", "BAUD": "baud_address",
            "RXDATAL": "rxdata_address", "TXDATAL": "txdata_address",
            "DBGCTRL": "dbgctrl_address", "EVCTRL": "evctrl_address",
        }
        out.append("")
        out.append(f"    .uart8x_count = {len(uart8x_instances)}U,")
        out.append(f"    .uarts8x = {{{{")
        for ui, uname in enumerate(uart8x_instances):
            regs = extract_module_registers(root, "USART", uname, AVR8X_USART_FIELDS)
            rxc_idx = find_interrupt_by_module(root, "RXC", uname)
            dre_idx = find_interrupt_by_module(root, "DRE", uname)
            txc_idx = find_interrupt_by_module(root, "TXC", uname)
            txd_addr = 0; txd_bit = 0
            rxd_addr = 0; rxd_bit = 0
            for mod in root.findall(f".//peripherals/module[@name='USART']"):
                for inst in mod.findall("instance"):
                    if inst.get("name") != uname:
                        continue
                    txd_found = False; rxd_found = False
                    for sig in inst.findall(".//signal"):
                        if txd_found and rxd_found:
                            break
                        sgroup = sig.get("group", "")
                        pad = sig.get("pad", "")
                        if sgroup == "TXD" and pad and not txd_found:
                            pl = pad[1]; b = int(pad[2:]) if len(pad) > 2 else 0
                            for pn, pr in ports:
                                if pn[4] == pl:
                                    txd_addr = pr.get("port_address", 0); txd_bit = b; txd_found = True; break
                        elif sgroup == "RXD" and pad and not rxd_found:
                            pl = pad[1]; b = int(pad[2:]) if len(pad) > 2 else 0
                            for pn, pr in ports:
                                if pn[4] == pl:
                                    rxd_addr = pr.get("port_address", 0); rxd_bit = b; rxd_found = True; break
            uart_idx = 0
            m = re.search(r'(\d+)', uname)
            if m:
                uart_idx = int(m.group(1))
            out.append(f"        {{")
            for fn in ["ctrla_address", "ctrlb_address", "ctrlc_address",
                      "ctrld_address", "status_address", "baud_address",
                      "rxdata_address", "txdata_address",
                      "dbgctrl_address", "evctrl_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            if rxc_idx != 0xFF:
                out.append(f"            .rx_vector_index = {rxc_idx}U,")
            if txc_idx != 0xFF:
                out.append(f"            .tx_vector_index = {txc_idx}U,")
            if dre_idx != 0xFF:
                out.append(f"            .dre_vector_index = {dre_idx}U,")
            # EVSYS user event address (before pin fields per struct order)
            user_key = f"USERUSART{uart_idx}"
            if user_key in evsys_user_map:
                out.append(f"            .user_event_address = 0x{evsys_user_map[user_key]:02X}U,")
            if txd_addr:
                out.append(f"            .txd_pin_address = 0x{txd_addr:02X}U,")
                out.append(f"            .txd_pin_bit = {txd_bit}U,")
            if rxd_addr:
                out.append(f"            .rxd_pin_address = 0x{rxd_addr:02X}U,")
                out.append(f"            .rxd_pin_bit = {rxd_bit}U,")
            out.append(f"            .index = {uart_idx}U,")
            out.append(f"        }}," if ui < len(uart8x_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- PCINT (classic only, from EXINT module) ----
    if pcint_instances and not is_avr8x:
        pcint_regs = extract_module_registers(root, "EXINT", extint_instances[0] if extint_instances else "EXINT", PCINT_FIELDS)
        pcicr = pcint_regs.get("pcicr_address", 0)
        pcifr = pcint_regs.get("pcifr_address", 0)
        out.append("")
        out.append(f"    .pcint_count = {len(pcint_instances)}U,")
        out.append(f"    .pcints = {{{{")
        for pi, pidx in enumerate(sorted(set(pcint_instances))):
            pcmsk_key = f"pcmsk{pidx}_address"
            paddr = pcint_regs.get(pcmsk_key, 0)
            mask = 1 << pidx
            pci_idx = find_interrupt_index(root, f"PCINT{pidx}")
            out.append(f"        {{")
            if pcicr:
                out.append(f"            .pcicr_address = 0x{pcicr:02X}U,")
            if pcifr:
                out.append(f"            .pcifr_address = 0x{pcifr:02X}U,")
            if paddr:
                out.append(f"            .pcmsk_address = 0x{paddr:02X}U,")
            out.append(f"            .pcicr_enable_mask = 0x{mask:X}U, .pcifr_flag_mask = 0x{mask:X}U,")
            if pci_idx != 0xFF:
                out.append(f"            .vector_index = {pci_idx}U")
            out.append(f"        }}," if pi < len(set(pcint_instances)) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- AVR8X NVMCTRL ----
    if is_avr8x and has_nvmctrl:
        nvm_regs = extract_module_registers(root, "NVMCTRL", "NVMCTRL", {
            "CTRLA": "ctrla_address", "CTRLB": "ctrlb_address",
            "STATUS": "status_address", "INTCTRL": "intctrl_address",
            "INTFLAGS": "intflags_address",
            "DATA": "data_address", "ADDR": "addr_address",
        })
        nvm_idx = find_interrupt_by_module(root, "EE", "NVMCTRL")
        if nvm_idx == 0xFF:
            nvm_idx = find_interrupt_index(root, "EE")
        out.append("")
        out.append(f"    .nvm_ctrl_count = 1U,")
        out.append(f"    .nvm_ctrls = {{{{")
        out.append(f"        {{")
        for fn in ["ctrla_address", "ctrlb_address", "status_address",
                  "intctrl_address", "intflags_address",
                  "addr_address", "data_address"]:
            if nvm_regs.get(fn):
                out.append(f"            .{fn} = 0x{nvm_regs[fn]:02X}U,")
        if nvm_idx != 0xFF:
            out.append(f"            .vector_index = {nvm_idx}U,")
        out.append(f"        }}")
        out.append(f"    }}}},")
    
    # ---- AVR8X CPUINT ----
    if is_avr8x and has_cpuint:
        cpuint_regs = extract_module_registers(root, "CPUINT", "CPUINT", {
            "CTRLA": "ctrla_address", "STATUS": "status_address",
            "LVL0PRI": "lvl0pri_address", "LVL1VEC": "lvl1vec_address",
        })
        out.append("")
        out.append(f"    .cpu_int_count = 1U,")
        out.append(f"    .cpu_ints = {{{{")
        out.append(f"        {{")
        for fn in ["ctrla_address", "status_address", "lvl0pri_address", "lvl1vec_address"]:
            if cpuint_regs.get(fn):
                out.append(f"            .{fn} = 0x{cpuint_regs[fn]:02X}U,")
        out.append(f"        }}")
        out.append(f"    }}}},")
    
    # ---- Classic SPI / AVR8X SPI8x ----
    if spi_instances and not is_avr8x:
        out.append("")
        out.append(f"    .spi_count = {len(spi_instances)}U,")
        out.append(f"    .spis = {{{{")
        for si, sname in enumerate(spi_instances):
            regs = extract_module_registers(root, "SPI", sname, SPI_FIELDS)
            out.append(f"        {{")
            for field_name in ["spcr_address", "spsr_address", "spdr_address"]:
                if regs.get(field_name):
                    out.append(f"            .{field_name} = 0x{regs[field_name]:02X}U,")
            out.append(f"        }}," if si < len(spi_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    if is_avr8x and spi8x_instances:
        AVR8X_SPI_FIELDS = {
            "CTRLA": "ctrla_address", "CTRLB": "ctrlb_address",
            "INTCTRL": "intctrl_address", "INTFLAGS": "intflags_address",
            "DATA": "data_address",
        }
        out.append("")
        out.append(f"    .spi8x_count = {len(spi8x_instances)}U,")
        out.append(f"    .spis8x = {{{{")
        for si, sname in enumerate(spi8x_instances):
            regs = extract_module_registers(root, "SPI", sname, AVR8X_SPI_FIELDS)
            sp_idx = find_interrupt_by_module(root, "INT", sname)
            spi_idx = 0
            m = re.search(r'(\d+)', sname)
            if m:
                spi_idx = int(m.group(1))
            out.append(f"        {{")
            for fn in ["ctrla_address", "ctrlb_address", "intctrl_address",
                      "intflags_address", "data_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            if sp_idx != 0xFF:
                out.append(f"            .vector_index = {sp_idx}U,")
            # EVSYS user event address (before index per struct order)
            user_key = f"USERSPI{spi_idx}"
            if user_key in evsys_user_map:
                out.append(f"            .user_event_address = 0x{evsys_user_map[user_key]:02X}U,")
            out.append(f"            .index = {spi_idx}U,")
            out.append(f"        }}," if si < len(spi8x_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- Classic TWI / AVR8X TWI8x ----
    if twi_instances and not is_avr8x:
        out.append("")
        out.append(f"    .twi_count = {len(twi_instances)}U,")
        out.append(f"    .twis = {{{{")
        for ti, tname in enumerate(twi_instances):
            regs = extract_module_registers(root, "TWI", tname, TWI_FIELDS)
            out.append(f"        {{")
            for field_name in ["twbr_address", "twsr_address", "twar_address",
                               "twdr_address", "twcr_address", "twamr_address"]:
                if regs.get(field_name):
                    out.append(f"            .{field_name} = 0x{regs[field_name]:02X}U,")
            out.append(f"        }}," if ti < len(twi_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    if is_avr8x and twi8x_instances:
        AVR8X_TWI_FIELDS = {
            "MCTRLA": "mctrla_address", "MCTRLB": "mctrlb_address",
            "MSTATUS": "mstatus_address", "MBAUD": "mbaud_address",
            "MADDR": "maddr_address", "MDATA": "mdata_address",
            "SCTRLA": "sctrla_address", "SCTRLB": "sctrlb_address",
            "SSTATUS": "sstatus_address", "SADDR": "saddr_address",
            "SDATA": "sdata_address", "SADDRMASK": "saddrmask_address",
            "DBGCTRL": "dbgctrl_address",
        }
        out.append("")
        out.append(f"    .twi8x_count = {len(twi8x_instances)}U,")
        out.append(f"    .twis8x = {{{{")
        for ti, tname in enumerate(twi8x_instances):
            regs = extract_module_registers(root, "TWI", tname, AVR8X_TWI_FIELDS)
            twim_idx = find_interrupt_by_module(root, "TWIM", tname)
            twis_idx = find_interrupt_by_module(root, "TWIS", tname)
            twi_idx = 0
            m = re.search(r'(\d+)', tname)
            if m:
                twi_idx = int(m.group(1))
            out.append(f"        {{")
            for fn in ["mctrla_address", "mctrlb_address", "mstatus_address",
                      "mbaud_address", "maddr_address", "mdata_address",
                      "sctrla_address", "sctrlb_address", "sstatus_address",
                      "saddr_address", "sdata_address", "saddrmask_address",
                      "dbgctrl_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            if twim_idx != 0xFF:
                out.append(f"            .master_vector_index = {twim_idx}U,")
            if twis_idx != 0xFF:
                out.append(f"            .slave_vector_index = {twis_idx}U,")
            # EVSYS user event address (before index per struct order)
            user_key = f"USERTWI{twi_idx}"
            if user_key in evsys_user_map:
                out.append(f"            .user_event_address = 0x{evsys_user_map[user_key]:02X}U,")
            out.append(f"            .index = {twi_idx}U,")
            out.append(f"        }}," if ti < len(twi8x_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- USI (classic only - AVR8X doesn't have USI) ----
    if usi_instances:
        out.append("")
        out.append(f"    .usi_count = {len(usi_instances)}U,")
        out.append(f"    .usis = {{{{")
        for ui, usi_name in enumerate(usi_instances):
            usi_regs = extract_module_registers(root, "USI", usi_name, USI_FIELDS)
            start_idx = find_interrupt_index(root, "USI_START")
            ovf_idx = find_interrupt_index(root, "USI_OVF")
            
            sda_pin_addr = 0; sda_pin_bit = 0
            scl_pin_addr = 0; scl_pin_bit = 0
            do_pin_addr = 0; do_pin_bit = 0
            for mod in root.findall(".//peripherals/module[@name='USI']"):
                for inst in mod.findall("instance"):
                    if inst.get("name") != usi_name:
                        continue
                    for sig in inst.findall(".//signal"):
                        sgroup = sig.get("group", "").strip()
                        pad = sig.get("pad", "")
                        port_addr = 0x38 if not ports else ports[0][1].get("port_address", 0x38)
                        if sgroup == "SDA":
                            sda_pin_bit = parse_int(pad[-1]) if pad else 0
                            sda_pin_addr = port_addr
                        elif sgroup == "SCL":
                            scl_pin_bit = parse_int(pad[-1]) if pad else 0
                            scl_pin_addr = port_addr
                        elif sgroup == "DO":
                            do_pin_bit = parse_int(pad[-1]) if pad else 0
                            do_pin_addr = port_addr

            # Fallback: if no USI signals found in this ATDF, try the base/-a variant pair
            if not sda_pin_addr and not scl_pin_addr:
                chip_lower = chip_name.lower()
                variant_pairs = {
                    "attiny24": "attiny24a", "attiny24a": "attiny24",
                    "attiny44": "attiny44a", "attiny44a": "attiny44",
                    "attiny84": "attiny84a", "attiny84a": "attiny84",
                    "attiny261": "attiny261a", "attiny261a": "attiny261",
                    "attiny461": "attiny461a", "attiny461a": "attiny461",
                    "attiny861": "attiny861a", "attiny861a": "attiny861",
                }
                alt_chip = variant_pairs.get(chip_lower)
                if alt_chip:
                    # Build path to variant ATDF (same directory as current file)
                    atdf_dir = os.path.dirname(atdf_path) if atdf_path else None
                    if atdf_dir:
                        # The variant filename might use different casing
                        base_name = alt_chip
                        # Try to find the file with case-insensitive matching
                        alt_path = None
                        for fn in os.listdir(atdf_dir):
                            if fn.lower().replace('-', '').replace(' ', '') == base_name + '.atdf':
                                alt_path = os.path.join(atdf_dir, fn)
                                break
                        if alt_path and os.path.exists(alt_path):
                            try:
                                alt_root = ET.parse(alt_path).getroot()
                                for mod in alt_root.findall(".//peripherals/module[@name='USI']"):
                                    for inst in mod.findall("instance"):
                                        if inst.get("name") != "USI":
                                            continue
                                        for sig in inst.findall(".//signal"):
                                            sgroup = sig.get("group", "").strip()
                                            pad = sig.get("pad", "")
                                            # Use same port from this device's ports
                                            port_addr = 0x38 if not ports else ports[0][1].get("port_address", 0x38)
                                            if sgroup == "SDA":
                                                sda_pin_bit = parse_int(pad[-1]) if pad else 0
                                                sda_pin_addr = port_addr
                                            elif sgroup == "SCL":
                                                scl_pin_bit = parse_int(pad[-1]) if pad else 0
                                                scl_pin_addr = port_addr
                                            elif sgroup == "DO":
                                                do_pin_bit = parse_int(pad[-1]) if pad else 0
                                                do_pin_addr = port_addr
                            except Exception:
                                pass
            
            out.append(f"        {{")
            if usi_regs.get("usicr_address"):
                out.append(f"            .usicr_address = 0x{usi_regs['usicr_address']:02X}U,")
            if usi_regs.get("usisr_address"):
                out.append(f"            .usisr_address = 0x{usi_regs['usisr_address']:02X}U,")
            if usi_regs.get("usidr_address"):
                out.append(f"            .usidr_address = 0x{usi_regs['usidr_address']:02X}U,")
            if usi_regs.get("usibr_address"):
                out.append(f"            .usibr_address = 0x{usi_regs['usibr_address']:02X}U,")
            if start_idx != 0xFF:
                out.append(f"            .start_vector_index = {start_idx}U,")
            if ovf_idx != 0xFF:
                out.append(f"            .overflow_vector_index = {ovf_idx}U,")
            if sda_pin_addr:
                out.append(f"            .sda_pin_address = 0x{sda_pin_addr:02X}U,")
                out.append(f"            .sda_pin_bit = {sda_pin_bit}U,")
            if scl_pin_addr:
                out.append(f"            .scl_pin_address = 0x{scl_pin_addr:02X}U,")
                out.append(f"            .scl_pin_bit = {scl_pin_bit}U,")
            if do_pin_addr:
                out.append(f"            .do_pin_address = 0x{do_pin_addr:02X}U,")
                out.append(f"            .do_pin_bit = {do_pin_bit}U,")
            out.append(f"        }}")
            if ui < len(usi_instances) - 1:
                out.append(",")
        out.append(f"    }}}},")
    
    # ---- EEPROM ----
    if eeprom_instances:
        out.append("")
        out.append(f"    .eeprom_count = {len(eeprom_instances)}U,")
        out.append(f"    .eeproms = {{{{")
        for ei, ename in enumerate(eeprom_instances):
            if is_avr8x:
                out.append(f"        {{")
                if mapped_eeprom_size:
                    out.append(f"            .size = 0x{mapped_eeprom_size:X}U,")
                    out.append(f"            .mapped_data = {{ 0x{mapped_eeprom_start:04X}U, 0x{mapped_eeprom_size:04X}U }},")
                ee_idx = find_interrupt_by_module(root, "EE", ename.upper())
                if ee_idx != 0xFF:
                    out.append(f"            .vector_index = {ee_idx}U,")
                out.append(f"        }}," if ei < len(eeprom_instances) - 1 else f"        }}")
            else:
                regs = extract_module_registers(root, "EEPROM", ename, EEPROM_FIELDS)
                out.append(f"        {{")
                for field_name in ["eecr_address", "eedr_address", "eearl_address", "eearh_address"]:
                    if regs.get(field_name):
                        out.append(f"            .{field_name} = 0x{regs[field_name]:02X}U,")
                if eeprom_bytes:
                    out.append(f"            .size = 0x{eeprom_bytes:X}U,")
                out.append(f"        }}," if ei < len(eeprom_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- Classic WDT / AVR8X WDT8x ----
    if wdt_instances and not is_avr8x:
        out.append("")
        out.append(f"    .wdt_count = {len(wdt_instances)}U,")
        out.append(f"    .wdts = {{{{")
        for wi, wname in enumerate(wdt_instances):
            regs = extract_module_registers(root, "WDT", wname, WDT_FIELDS)
            wdt_idx = find_interrupt_index(root, "WDT")
            if wdt_idx == 0xFF:
                wdt_idx = find_interrupt_index(root, "WDT_OVERFLOW")
            out.append(f"        {{")
            if regs.get("wdtcsr_address"):
                out.append(f"            .wdtcsr_address = 0x{regs['wdtcsr_address']:02X}U,")
            elif regs.get("wdtcr_address"):
                out.append(f"            .wdtcsr_address = 0x{regs['wdtcr_address']:02X}U,")
            if wdt_idx != 0xFF:
                out.append(f"            .vector_index = {wdt_idx}U,")
            out.append(f"        }}," if wi < len(wdt_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    if is_avr8x and wdt8x_instances:
        wdt_regs = extract_module_registers(root, "WDT", "WDT", {
            "CTRLA": "ctrla_address",
            "STATUS": "status_address",
        })
        wdt_idx = find_interrupt_by_module(root, "WDT", "WDT")
        if wdt_idx == 0xFF:
            wdt_idx = find_interrupt_index(root, "WDT")
        out.append("")
        out.append(f"    .wdt8x_count = {len(wdt8x_instances)}U,")
        out.append(f"    .wdts8x = {{{{")
        out.append(f"        {{")
        if wdt_regs.get("ctrla_address"):
            out.append(f"            .ctrla_address = 0x{wdt_regs['ctrla_address']:02X}U,")
        if wdt_regs.get("status_address"):
            out.append(f"            .status_address = 0x{wdt_regs['status_address']:02X}U,")
        if wdt_idx != 0xFF:
            out.append(f"            .vector_index = {wdt_idx}U,")
        out.append(f"        }}")
        out.append(f"    }}}},")
    
    # ---- AVR8X CRC8x (CRCSCAN) ----
    if is_avr8x and has_crc8x:
        crc_regs = extract_module_registers(root, "CRCSCAN", "CRCSCAN", {
            "CTRLA": "ctrla_address", "CTRLB": "ctrlb_address",
            "STATUS": "status_address",
        })
        out.append("")
        out.append(f"    .crc8x_count = 1U,")
        out.append(f"    .crcs8x = {{{{")
        out.append(f"        {{")
        for fn in ["ctrla_address", "ctrlb_address", "status_address"]:
            if crc_regs.get(fn):
                out.append(f"            .{fn} = 0x{crc_regs[fn]:02X}U,")
        out.append(f"        }}")
        out.append(f"    }}}},")
    
    # ---- CAN (AT90CAN / ATmega32C1/64C1) ----
    if can_instances:
        out.append("")
        out.append(f"    .can_count = {len(can_instances)}U,")
        out.append(f"    .cans = {{{{")
        for ci, ename in enumerate(can_instances):
            regs = extract_module_registers(root, "CAN", ename, CAN_FIELDS)
            out.append(f"        {{")
            for fn in ["cangcon_address", "cangsta_address", "cangit_address", "cangie_address",
                       "canen1_address", "canen2_address", "canie1_address", "canie2_address",
                       "cansit1_address", "cansit2_address", "canbt1_address", "canbt2_address",
                       "canbt3_address", "cantcon_address", "cantim_address", "canttc_address",
                       "cantec_address", "canrec_address", "canhpmob_address", "canpage_address",
                       "canstmob_address", "cancdmob_address", "canidt_address", "canidm_address",
                       "canstm_address", "canmsg_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            canit_idx = find_interrupt_index(root, "CANIT")
            if canit_idx != 0xFF:
                out.append(f"            .canit_vector_index = {canit_idx}U,")
            ovrit_idx = find_interrupt_index(root, "OVRIT")
            if ovrit_idx != 0xFF:
                out.append(f"            .ovrit_vector_index = {ovrit_idx}U,")
            out.append(f"        }}," if ci < len(can_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- AVR8X USB8x (DU FIFO-based USB) ----
    if is_avr8x and has_usb8x:
        usb8x_regs = extract_module_registers(root, "USB", "USB0", {
            "CTRLA": "ctrla_address",
            "CTRLB": "ctrlb_address",
            "BUSSTATE": "busstate_address",
            "ADDR": "addr_address",
            "FIFOWP": "fifowp_address",
            "FIFORP": "fiforp_address",
            "EPPTR": "epptr_address",
            "INTCTRLA": "intctrla_address",
            "INTCTRLB": "intctrlb_address",
            "INTFLAGSA": "intflagsa_address",
            "INTFLAGSB": "intflagsb_address",
        })
        usb8x_idx = find_interrupt_by_module(root, "USB", "USB0")
        if usb8x_idx == 0xFF:
            usb8x_idx = find_interrupt_index(root, "USB")
        pllcsr_addr = 0
        for regname in ["PLLCSR", "PLLCTRLA"]:
            pllcsr_addr = get_register(root, "USB", "USB0", regname)
            if pllcsr_addr:
                break
        if not pllcsr_addr:
            pllcsr_addr = get_register(root, "CLKCTRL", "CLKCTRL", "PLLCTRLA")
        out.append("")
        out.append(f"    .usb8x_count = 1U,")
        out.append(f"    .usbs8x = {{{{")
        out.append(f"        {{")
        for fn in ["ctrla_address", "ctrlb_address", "busstate_address",
                   "addr_address", "fifowp_address", "fiforp_address",
                   "epptr_address", "intctrla_address", "intctrlb_address",
                   "intflagsa_address", "intflagsb_address"]:
            if usb8x_regs.get(fn):
                out.append(f"            .{fn} = 0x{usb8x_regs[fn]:02X}U,")
        if pllcsr_addr:
            out.append(f"            .pllcsr_address = 0x{pllcsr_addr:02X}U,")
        if usb8x_idx != 0xFF:
            out.append(f"            .gen_vector_index = {usb8x_idx}U,")
        out.append(f"        }}")
        out.append(f"    }}}},")
    
    # ---- LCD (ATmega329/649) ----
    if lcd_instances and not is_avr8x:
        out.append("")
        out.append(f"    .lcd_count = {len(lcd_instances)}U,")
        out.append(f"    .lcds = {{{{")
        for li, ename in enumerate(lcd_instances):
            regs = extract_module_registers(root, "LCD", ename, LCD_FIELDS)
            out.append(f"        {{")
            for fn in ["lcdcra_address", "lcdcrb_address", "lcdfrr_address", "lcdccr_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            if regs.get("lcddr_base_address"):
                out.append(f"            .lcddr_base_address = 0x{regs['lcddr_base_address']:02X}U,")
            lcd_idx = find_interrupt_by_module(root, "LCD", ename)
            if lcd_idx != 0xFF:
                out.append(f"            .vector_index = {lcd_idx}U,")
            out.append(f"        }}," if li < len(lcd_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- PSC (AT90PWM) ----
    if psc_instances and not is_avr8x:
        out.append("")
        out.append(f"    .psc_count = {len(psc_instances)}U,")
        out.append(f"    .pscs = {{{{")
        for pi, ename in enumerate(psc_instances):
            regs = extract_module_registers(root, "PSC", ename, PSC_FIELDS)
            out.append(f"        {{")
            for fn in ["pctl_address", "psoc_address", "pconf_address", "pim_address",
                       "pifr_address", "picr_address", "ocrsa_address", "ocrra_address",
                       "ocrsb_address", "ocrrb_address", "pfrc0a_address", "pfrc0b_address",
                       "pom_address", "pcnt_address", "pllcsr_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            gen_idx = find_interrupt_by_module(root, "PSC", ename + "_PSC")
            if gen_idx != 0xFF:
                out.append(f"            .gen_vector_index = {gen_idx}U,")
            out.append(f"        }}," if pi < len(psc_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- EUSART (AT90PWM) ----
    if eusart_instances and not is_avr8x:
        out.append("")
        out.append(f"    .eusart_count = {len(eusart_instances)}U,")
        out.append(f"    .eusarts = {{{{")
        for ei, ename in enumerate(eusart_instances):
            regs = extract_module_registers(root, "EUSART", ename, EUSART_FIELDS)
            out.append(f"        {{")
            for fn in ["eudr_address", "eucsra_address", "eucsrb_address", "eucsrc_address",
                       "mubrrl_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            out.append(f"        }}," if ei < len(eusart_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- Classic DAC (AT90PWM / ATmega32C1/64C1) ----
    if dac_classic_instances and not is_avr8x:
        out.append("")
        out.append(f"    .dac_count = {len(dac_classic_instances)}U,")
        out.append(f"    .dacs = {{{{")
        for di, ename in enumerate(dac_classic_instances):
            regs = extract_module_registers(root, "DAC", ename, DAC_CLASSIC_FIELDS)
            out.append(f"        {{")
            if regs.get("dacon_address"):
                out.append(f"            .dacon_address = 0x{regs['dacon_address']:02X}U,")
            if regs.get("dacl_address"):
                out.append(f"            .dacl_address = 0x{regs['dacl_address']:02X}U,")
                # DAC register is 16-bit; dach = dacl + 1
                out.append(f"            .dach_address = 0x{regs['dacl_address'] + 1:02X}U,")
            out.append(f"        }}," if di < len(dac_classic_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- AVR8X DAC8x (DA/DB/DD) ----
    if is_avr8x and dac_instances:
        AVR8X_DAC_FIELDS = {
            "CTRLA": "ctrla_address",
            "DATA": "data_address",
        }
        out.append("")
        out.append(f"    .dac8x_count = {len(dac_instances)}U,")
        out.append(f"    .dacs8x = {{{{")
        for di, dname in enumerate(dac_instances):
            regs = extract_module_registers(root, "DAC", dname, AVR8X_DAC_FIELDS)
            out.append(f"        {{")
            for fn in ["ctrla_address", "data_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            out.append(f"        }}," if di < len(dac_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- Amplifier (AT90PWM) ----
    if amplifier_instances and not is_avr8x:
        out.append("")
        out.append(f"    .amplifier_count = {len(amplifier_instances)}U,")
        out.append(f"    .amplifiers = {{{{")
        for ai, ename in enumerate(amplifier_instances):
            regs = extract_module_registers(root, "AMP", ename, AMP_FIELDS)
            out.append(f"        {{")
            if regs.get("ampcsr_address"):
                out.append(f"            .ampcsr_address = 0x{regs['ampcsr_address']:02X}U,")
            out.append(f"        }}," if ai < len(amplifier_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- ZCD ----
    if zcd_instances:
        AVR8X_ZCD_FIELDS = {
            "CTRLA": "ctrla_address",
            "INTCTRL": "intctrl_address",
            "STATUS": "status_address",
        }
        out.append("")
        out.append(f"    .zcd_count = {len(zcd_instances)}U,")
        out.append(f"    .zcds = {{{{")
        for zi, zname in enumerate(zcd_instances):
            regs = extract_module_registers(root, "ZCD", zname, AVR8X_ZCD_FIELDS)
            zcd_idx = find_interrupt_by_module(root, "ZCD", zname)
            out.append(f"        {{")
            for fn in ["ctrla_address", "intctrl_address", "status_address"]:
                if regs.get(fn):
                    out.append(f"            .{fn} = 0x{regs[fn]:02X}U,")
            if zcd_idx != 0xFF:
                out.append(f"            .vector_index = {zcd_idx}U,")
            out.append(f"        }}," if zi < len(zcd_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- OPAMP (DB only) ----
    if opamp_instances:
        out.append("")
        out.append(f"    .opamp_count = {len(opamp_instances)}U,")
        out.append(f"    .opamps = {{{{")
        for oi, oname in enumerate(opamp_instances):
            opamp_regs = extract_module_registers(root, "OPAMP", oname, {
                "CTRLA": "ctrla_address",
                "DBGCTRL": "ctrlb_address",
                "TIMEBASE": "resctrl_address",
                "PWRCTRL": "muxctrl_address",
            })
            out.append(f"        {{")
            for fn in ["ctrla_address", "ctrlb_address", "resctrl_address", "muxctrl_address"]:
                if opamp_regs.get(fn):
                    out.append(f"            .{fn} = 0x{opamp_regs[fn]:02X}U,")
            out.append(f"        }}," if oi < len(opamp_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- AVR8X PTC (DA only) ----
    if is_avr8x and ptc_instances:
        out.append("")
        out.append(f"    .ptc_count = {len(ptc_instances)}U,")
        out.append(f"    .ptcs = {{{{")
        for pi, pname in enumerate(ptc_instances):
            base_addr = 0
            for mod in root.findall(f".//peripherals/module[@name='PTC']"):
                for inst in mod.findall("instance"):
                    if inst.get("name") == pname:
                        for rg in inst.findall("register-group"):
                            base_addr = parse_hex(rg.get("offset", "0"))
            eoc_idx = find_interrupt_by_module(root, "EOC", pname)
            wcomp_idx = find_interrupt_by_module(root, "WCOMP", pname)
            out.append(f"        {{")
            if base_addr:
                out.append(f"            .base_address = 0x{base_addr:04X}U,")
            if eoc_idx != 0xFF:
                out.append(f"            .eoc_vector_index = {eoc_idx}U,")
            if wcomp_idx != 0xFF:
                out.append(f"            .wcomp_vector_index = {wcomp_idx}U,")
            out.append(f"        }}," if pi < len(ptc_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # ---- LIN (ATmega32M1/64M1/16M1) ----
    if lin_instances and not is_avr8x:
        out.append("")
        out.append(f"    .lin_count = {len(lin_instances)}U,")
        out.append(f"    .lins = {{{{")
        for li, ename in enumerate(lin_instances):
            regs = extract_module_registers(root, "LINUART", ename, LINUART_FIELDS)
            out.append(f"        {{")
            if regs.get("ctrla_address"):
                out.append(f"            .ctrla_address = 0x{regs['ctrla_address']:02X}U,")
            lin_idx = find_interrupt_by_module(root, "LIN", ename)
            if lin_idx != 0xFF:
                out.append(f"            .vector_index = {lin_idx}U,")
            out.append(f"        }}," if li < len(lin_instances) - 1 else f"        }}")
        out.append(f"    }}}},")
    
    # Signature
    out.append("")
    out.append(f"    .signature = {{ 0x{sig0:02X}, 0x{sig1:02X}, 0x{sig2:02X} }},")
    
    out.append("")
    out.append(f"    .operating_voltage_v = 5.0,")
    out.append(f"    .vil_factor = 0.3,")
    out.append(f"    .vih_factor = 0.6,")
    
    # ---- Ports ----
    if ports:
        out.append("")
        out.append(f"    .port_count = {len(ports)}U,")
        out.append(f"    .ports = {{{{")
        for pi, (pname, regs) in enumerate(ports):
            port_letter = pname[4]
            pin_addr = regs.get("pin_address", 0)
            ddr_addr = regs.get("ddr_address", 0)
            port_addr = regs.get("port_address", 0)
            dirset = regs.get("dirset_address", 0)
            dirclr = regs.get("dirclr_address", 0)
            dirtgl = regs.get("dirtgl_address", 0)
            outset = regs.get("outset_address", 0)
            outclr = regs.get("outclr_address", 0)
            outtgl = regs.get("outtgl_address", 0)
            pin_ctrl = regs.get("pin_ctrl_base", 0)
            intflags = regs.get("intflags_address", 0)
            vport = regs.get("vport_base", 0xFFFF)
            
            out.append(f"        {{ \"PORT{port_letter}\", 0x{pin_addr:02X}U, 0x{ddr_addr:02X}U, 0x{port_addr:02X}U,")
            out.append(f"          0x{dirset:02X}U, 0x{dirclr:02X}U, 0x{dirtgl:02X}U, 0x{outset:02X}U, 0x{outclr:02X}U, 0x{outtgl:02X}U,")
            out.append(f"          0x{pin_ctrl:02X}U, 0x{intflags:02X}U, 0x{vport:04X}U, 255U }}")
            if pi < len(ports) - 1:
                out.append(",")
        out.append(f"    }}}}")
    
    out.append("};")
    out.append("")
    out.append("} // namespace vioavr::core::devices")
    out.append("")
    
    output_file.write("\n".join(out))
    return True


def generate_all(atdf_glob_pattern, output_dir, description):
    out_dir = Path(output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    count = 0
    import glob as gglob
    files = sorted(gglob.glob(atdf_glob_pattern))
    for atdf_path_str in files:
        atdf_path = Path(atdf_path_str)
        chip = atdf_path.stem.lower()
        # Architecture check (AVR8X ATmega devices handled by same generate function)
        try:
            tree = ET.parse(str(atdf_path))
            root = tree.getroot()
            devs = root.find('devices')
            arch = ''
            if devs is not None:
                for dev_el in devs.findall('device'):
                    arch = dev_el.get('architecture', '')
                    break
        except Exception:
            pass
        out_path = out_dir / f"{chip}.hpp"
        print(f"  {atdf_path.stem} -> {out_path.name}")
        try:
            tree = ET.parse(str(atdf_path))
            with open(str(out_path), "w") as f:
                if not generate(tree.getroot(), f, str(atdf_path)):
                    print(f"    FAILED", file=sys.stderr)
            count += 1
        except Exception as e:
            print(f"    ERROR: {e}", file=sys.stderr)
    print(f"  Generated {count} files")
    return count

def build_xmega_register_map(root):
    """Build register address map for XMEGA ATDF format.
    
    XMEGA ATDF splits register definitions across two locations:
    - Root <modules>: defines register layout (names + offsets relative to register-group)
    - Device <peripherals>: maps register-group to address space
    
    Returns dict: (module_name, instance_name) -> {register_name: absolute_address}
    """
    # Step 1: Build module register layouts from root <modules>
    # <modules> -> <module> -> <register-group> -> <register>
    module_layouts = {}  # (module_name, rg_name) -> [(offset, name, size)]
    for mod in root.find('modules'):
        mname = mod.get('name', '')
        for rg in mod.findall('register-group'):
            rgname = rg.get('name', '')
            regs = []
            for r in rg.findall('register'):
                roff = parse_hex(r.get('offset', '0'))
                regs.append((roff, r.get('name', ''), r.get('size', '1')))
            if regs:
                module_layouts[(mname, rgname)] = regs
    
    def match_rg(mname, dev_rgname):
        """Match device register-group name to root register-group name."""
        # Exact match
        key = (mname, dev_rgname)
        if key in module_layouts:
            return module_layouts[key]
        # Try module name as group name
        key2 = (mname, mname)
        if key2 in module_layouts:
            return module_layouts[key2]
        # Strip trailing digits from device group name (e.g. "PORTA" -> "PORT")
        stripped = re.sub(r'[0-9]+$', '', dev_rgname)
        if stripped and stripped != dev_rgname:
            key3 = (mname, stripped)
            if key3 in module_layouts:
                return module_layouts[key3]
                # Try matching by prefix (e.g. "PORTA" starts with "PORT")
        for (mn, rgn), regs in module_layouts.items():
            if mn == mname:
                if rgn.startswith(dev_rgname) or dev_rgname.startswith(rgn):
                    return regs
                # Try stripping common prefixes/suffixes
                # "TCC0" -> "TC0" (strip port letter, keep channel number)
                if dev_rgname.startswith(mname) and len(dev_rgname) > len(mname):
                    without_prefix = dev_rgname[len(mname):]
                    if rgn.endswith(without_prefix) or without_prefix.endswith(rgn):
                        return regs
                # XMEGA TC/TCD0 "TCD0" -> "TC0": strip port letter from middle
                # dev_rgname format: TC<port_letter><num> (e.g. TCC0, TCD0, TCF0)
                # rgn format: TC<num> (e.g. TC0, TC1)
                if dev_rgname.startswith(mname):
                    num_match = re.match(r'^TC[A-Z](\d+)$', dev_rgname)
                    if num_match:
                        num = num_match.group(1)
                        if rgn == f'TC{num}':
                            return regs
        return []
    
    # Step 2: Map device peripherals using base addresses
    result = {}
    dev = root.find('.//device')
    periphs = dev.find('peripherals')
    
    for mod in periphs:
        mname = mod.get('name', '')
        for inst in mod:
            iname = inst.get('name', '')
            instance_regs = {}
            
            for rg in inst.findall('register-group'):
                base = parse_hex(rg.get('offset', '0'))
                rgname = rg.get('name', '')
                matched_regs = match_rg(mname, rgname)
                for roff, rname, rsize in matched_regs:
                    instance_regs[rname] = base + roff
            
            # Also look for sub-register-groups (DMA_CH, ADC_CH, TWI_MASTER, etc.)
            # These are at different offsets within the same module
            seen_names = set(instance_regs.keys())
            for (mn, rgn), regs in module_layouts.items():
                if mn == mname and rgn != mname and rgn not in seen_names:
                    # Check if this sub-group name relates to the instance
                    if rgn.endswith('_CH') or rgn in ('TWI_MASTER', 'TWI_SLAVE'):
                        for roff, rname, rsize in regs:
                            instance_regs[rname] = base + roff
                            seen_names.add(rname)
            
            if instance_regs:
                result[(mname, iname)] = instance_regs
    
    return result


def generate_xmega(root, output_file=sys.stdout):
    """Generate DeviceDescriptor for XMEGA devices (architecture=AVR8_XMEGA)."""
    
    dev = root.find('.//device')
    if dev is None:
        print("ERROR: No <device> found", file=sys.stderr)
        return False
    
    name = dev.get("name", "unknown")
    chip_name = name.lower().replace("-", "").replace(" ", "")
    
    # Build register map
    regs = build_xmega_register_map(root)
    
    # ---- Memory layout ----
    flash_words = 0
    sram_bytes = 0
    sram_start = 0
    eeprom_bytes = 0
    flash_page_size = 0x80
    io_start = 0x0000
    io_end = 0x0FFF  # XMEGA has 4KB IO space
    
    for asp in dev.findall('.//address-spaces/address-space'):
        aname = asp.get('name', '')
        if aname == 'prog':
            for seg in asp.findall('memory-segment'):
                if seg.get('type') == 'flash':
                    fs = parse_hex(seg.get('size', '0'))
                    flash_words += fs // 2
                    ps = seg.get('pagesize', '0')
                    if ps != '0':
                        flash_page_size = parse_hex(ps)
        elif aname == 'data':
            for seg in asp.findall('memory-segment'):
                stype = seg.get('type', '')
                if stype == 'ram' and 'EXTERNAL' not in seg.get('name', ''):
                    sram_bytes = parse_hex(seg.get('size', '0'))
                    sram_start = parse_hex(seg.get('start', '0'))
                elif stype == 'io':
                    io_size = parse_hex(seg.get('size', '0'))
                    io_end = parse_hex(seg.get('start', '0')) + io_size - 1
        elif aname == 'eeprom':
            for seg in asp.findall('memory-segment'):
                if seg.get('type') == 'eeprom':
                    eeprom_bytes = parse_hex(seg.get('size', '0'))
    
    # ---- Interrupt vectors (XMEGA uses <interrupt-group> not <interrupt>) ----
    interrupts = []
    for i in dev.findall('interrupts/interrupt-group'):
        idx = parse_int(i.get('index', '0'))
        interrupts.append(idx)
    
    vector_count = max(interrupts) + 1 if interrupts else 0
    vector_size = 4  # XMEGA uses 4-byte vectors (EIND + 2-byte address)
    
    # ---- Signature bytes ----
    sig0 = sig1 = sig2 = 0
    for pg in root.findall(".//property-group[@name='SIGNATURES']"):
        for prop in pg.findall("property"):
            pname = prop.get("name", "")
            pval = prop.get("value", "0")
            if pname == "SIGNATURE0": sig0 = parse_hex(pval)
            elif pname == "SIGNATURE1": sig1 = parse_hex(pval)
            elif pname == "SIGNATURE2": sig2 = parse_hex(pval)
    
    # ---- System registers from CPU module ----
    cpu_regs = regs.get(("CPU", "CPU"), {})
    spl_addr = cpu_regs.get("SPL", 0x003D)
    sph_addr = cpu_regs.get("SPH", 0x003E)
    sreg_addr = cpu_regs.get("SREG", 0x003F)
    rampz_addr = cpu_regs.get("RAMPZ", 0)
    cc_addr = cpu_regs.get("CCP", 0)
    eind_addr = cpu_regs.get("EIND", 0)
    
    # ---- Ports ----
    port_instances = []
    for (mn, iname), iregs in regs.items():
        if mn == 'PORT' and re.match(r'^PORT[A-Z]$', iname):
            port_letter = iname[4]
            port_instances.append((port_letter, iregs))
    
    # Find VPORT instances
    vport_bases = {}
    for (mn, iname), iregs in regs.items():
        if re.match(r'^VPORT[0-3]$', iname):
            vport_num = int(iname[5])
            # VPORT0=0x10, VPORT1=0x14, VPORT2=0x18, VPORT3=0x1C
            # XMEGA maps VPORTs sequentially to ports
            if vport_num < len(port_instances):
                vport_bases[port_instances[vport_num][0]] = parse_hex(
                    next((rg.get('offset') for mod in dev.findall('.//peripherals/module')
                          for inst in mod if inst.get('name') == iname
                          for rg in inst.findall('register-group')), '0'))
    
    out = []
    out.append("#pragma once")
    out.append("#include \"vioavr/core/device.hpp\"")
    out.append("")
    out.append("namespace vioavr::core::devices {")
    out.append("")
    out.append(f"inline constexpr DeviceDescriptor {chip_name} {{")
    
    # Identity
    out.append(f"    .name = \"{name}\",")
    out.append(f"    .flash_words = {flash_words}U,")
    out.append(f"    .sram_bytes = {sram_bytes}U,")
    out.append(f"    .sram_start = 0x{sram_start:04X}U,")
    out.append(f"    .eeprom_bytes = {eeprom_bytes}U,")
    out.append(f"    .interrupt_vector_count = {vector_count}U,")
    out.append(f"    .interrupt_vector_size = {vector_size}U,")
    out.append(f"    .flash_page_size = 0x{flash_page_size:X}U,")
    
    # Memory ranges
    out.append(f"    .register_file_range = {{ 0x0000, 0x001F }},")
    out.append(f"    .io_range = {{ 0x{io_start:04X}, 0x{min(io_end, 0x3F):04X} }},")
    ext_end = max(io_end, 0x3F)
    out.append(f"    .extended_io_range = {{ 0x0040, 0x{ext_end:04X} }},")
    
    # Mapped memory (XMEGA has mapped EEPROM)
    mapped_eeprom_start = 0
    mapped_eeprom_size = 0
    for seg in dev.findall('.//address-spaces/address-space[@name="data"]//memory-segment'):
        if seg.get('type') == 'eeprom' and parse_hex(seg.get('start', '0')) > 0x100:
            mapped_eeprom_start = parse_hex(seg.get('start', '0'))
            mapped_eeprom_size = parse_hex(seg.get('size', '0'))
    if mapped_eeprom_size:
        out.append(f"    .mapped_eeprom = {{ 0x{mapped_eeprom_start:04X}U, 0x{mapped_eeprom_size:04X}U }},")
    
    # System registers
    out.append("")
    out.append(f"    .spl_address = 0x{spl_addr:02X}U,")
    out.append(f"    .sph_address = 0x{sph_addr:02X}U,")
    out.append(f"    .sreg_address = 0x{sreg_addr:02X}U,")
    if rampz_addr:
        out.append(f"    .rampz_address = 0x{rampz_addr:02X}U,")
    if eind_addr:
        out.append(f"    .eind_address = 0x{eind_addr:02X}U,")
    # PR (Power Reduction) — comes before ccp_address in DeviceDescriptor
    pr_regs = regs.get(("PR", "PR"), {})
    pr_base = pr_regs.get("PRGEN", 0)
    if pr_base:
        out.append(f"    .prr_address = {pr_base}U,")
    if cc_addr:
        out.append(f"    .ccp_address = 0x{cc_addr:02X}U,")
    
    out.append(f"    .cpu_frequency_hz = 32'000'000U,")
    
    # ---- XMEGA ADC ----
    for (mn, iname), iregs in regs.items():
        if mn == 'ADC' and re.match(r'^ADC[A-Z]$', iname):
            out.append("")
            out.append(f"    .adc_xmega_count = 1U,")
            out.append(f"    .adcs_xmega = {{{{")
            out.append(f"        {{")
            for rn, field in [("CTRLA","ctrla_address"),("CTRLB","ctrlb_address"),
                              ("REFCTRL","refctrl_address"),("EVCTRL","evctrl_address"),
                              ("PRESCALER","prescaler_address"),("INTFLAGS","intflags_address"),
                              ("TEMP","temp_address"),("SAMPCTRL","sampctrl_address"),
                              ("CAL","cal_address"),("CH0RES","ch0res_address"),
                              ("CMP","cmp_address"),("MUXCTRL","muxctrl_address"),
                              ("INTCTRL","ch_intctrl_address")]:
                if rn in iregs:
                    out.append(f"            .{field} = {iregs[rn]}U,")
            # Find ADC interrupt vector
            for i in dev.findall('interrupts/interrupt-group'):
                if i.get('module-instance', '') == iname:
                    vec = parse_int(i.get('index', '0'))
                    out.append(f"            .vector_index = {vec}U,")
                    break
            out.append(f"        }}}}}},")
            break
    
    # ---- XMEGA AC ----
    for (mn, iname), iregs in regs.items():
        if mn == 'AC' and re.match(r'^AC[A-Z]$', iname):
            out.append("")
            out.append(f"    .ac_xmega_count = 1U,")
            out.append(f"    .acs_xmega = {{{{")
            out.append(f"        {{")
            for rn, field in [("CTRLA","ctrla_address"),("CTRLB","ctrlb_address"),
                              ("WINCTRL","winctrl_address"),("STATUS","status_address"),
                              ("AC0CTRL","ac0ctrl_address"),("AC1CTRL","ac1ctrl_address"),
                              ("AC0MUXCTRL","ac0mux_address"),("AC1MUXCTRL","ac1mux_address")]:
                if rn in iregs:
                    out.append(f"            .{field} = {iregs[rn]}U,")
            for i in dev.findall('interrupts/interrupt-group'):
                if i.get('module-instance', '') == iname:
                    vec = parse_int(i.get('index', '0'))
                    out.append(f"            .vector_index = {vec}U,")
                    break
            out.append(f"        }}}}}},")
            break
    
    # ---- TC (Timer/Counter 16-bit) ----
    XMEGA_TC_FIELDS = {
        "CTRLA": "ctrla_address",
        "CTRLB": "ctrlb_address",
        "CTRLC": "ctrlc_address",
        "CTRLD": "ctrld_address",
        "CTRLE": "ctrle_address",
        "INTCTRLA": "intctrla_address",
        "INTCTRLB": "intctrlb_address",
        "CTRLFCLR": "ctrlfclr_address",
        "CTRLFSET": "ctrlfset_address",
        "CTRLGCLR": "ctrlgclr_address",
        "CTRLGSET": "ctrlgset_address",
        "INTFLAGS": "intflags_address",
        "TEMP": "temp_address",
        "CNT": "cnt_address",
        "PER": "period_address",
        "CCA": "cca_address",
        "CCB": "ccb_address",
        "CCC": "ccc_address",
        "CCD": "ccd_address",
        "PERBUF": "perbuf_address",
        "CCABUF": "ccabuf_address",
        "CCBBUF": "ccbbuf_address",
        "CCCBUF": "cccbuf_address",
        "CCDBUF": "ccdbuf_address",
    }
    tc_instances = []
    for (mn, iname), iregs in regs.items():
        if mn == 'TC' and re.match(r'^TC[A-Z]\d+$', iname):
            addrs = {}
            # Extract port letter from instance name (e.g., "TCC4" -> 'C')
            port_letter = ord(iname[2]) if len(iname) > 2 else ord('A')
            addrs['port_letter'] = port_letter
            for rname, field in XMEGA_TC_FIELDS.items():
                if rname in iregs:
                    addrs[field] = iregs[rname]
            # Find base interrupt vector
            base_vec = 0
            for i in dev.findall('interrupts/interrupt-group'):
                if i.get('module-instance', '') == iname:
                    base_vec = parse_int(i.get('index', '0'))
                    break
            # XMEGA TC has per-source vectors offset from base:
            # TC0 (full, 4 CC): OVF=base, ERR=base+1, CCA=base+2, CCB=base+3, CCC=base+4, CCD=base+5
            # TC1 (reduced, 2 CC): OVF=base, CCA=base+1, CCB=base+2
            has_ccc = 'CCC' in iregs
            has_err = 'CTRLGCLR' in iregs or 'CTRLGSET' in iregs
            if base_vec:
                addrs['ovf_vector_index'] = base_vec
                if has_err:
                    addrs['err_vector_index'] = base_vec + 1
                    addrs['cca_vector_index'] = base_vec + 2
                    addrs['ccb_vector_index'] = base_vec + 3
                    if has_ccc:
                        addrs['ccc_vector_index'] = base_vec + 4
                        addrs['ccd_vector_index'] = base_vec + 5
                else:
                    addrs['cca_vector_index'] = base_vec + 1
                    addrs['ccb_vector_index'] = base_vec + 2
            tc_instances.append((iname, addrs))
    if tc_instances:
        out.append("")
        out.append(f"    .tc_count = {len(tc_instances)}U,")
        out.append(f"    .timers_tc = {{{{")
        for ti, (tname, taddrs) in enumerate(tc_instances):
            sep = ',' if ti < len(tc_instances) - 1 else ''
            out.append(f"        {{")
            for field in ['ctrla_address', 'ctrlb_address', 'ctrlc_address', 'ctrld_address', 'ctrle_address',
                          'intctrla_address', 'intctrlb_address',
                          'ctrlfclr_address', 'ctrlfset_address',
                          'ctrlgclr_address', 'ctrlgset_address',
                          'intflags_address', 'temp_address',
                          'cnt_address', 'period_address',
                          'cca_address', 'ccb_address', 'ccc_address', 'ccd_address',
                          'perbuf_address', 'ccabuf_address', 'ccbbuf_address', 'cccbuf_address', 'ccdbuf_address',
                          'ovf_vector_index', 'err_vector_index',
                          'cca_vector_index', 'ccb_vector_index', 'ccc_vector_index', 'ccd_vector_index',
                          'port_letter']:
                val = taddrs.get(field, 0)
                if field == 'port_letter':
                    if val:
                        out.append(f"            .{field} = {val}U,")
                elif val:
                    out.append(f"            .{field} = {val}U,")
            out.append(f"        }}{sep}")
        out.append(f"    }}}},")
    
    # ---- AWEX (Advanced Waveform Extension) ----
    XMEGA_AWEX_FIELDS = {
        "CTRL": "ctrl_address",
        "FDEMASK": "fdemask_address",
        "FDCTRL": "fdctrl_address",
        "STATUS": "status_address",
        "DTBOTH": "dtboth_address",
        "DTBOTHBUF": "dtbothbuf_address",
        "DTLS": "dtls_address",
        "DTHS": "dths_address",
        "DTLSBUF": "dtlsbuf_address",
        "DTHSBUF": "dthsbuf_address",
        "OUTOVEN": "outoven_address",
    }
    awex_instances = []
    for (mn, iname), iregs in regs.items():
        if re.match(r'^AWEX[A-Z]$', iname):
            addrs = {}
            for rname, field in XMEGA_AWEX_FIELDS.items():
                if rname in iregs:
                    addrs[field] = iregs[rname]
            awex_instances.append((iname, addrs))
    if awex_instances:
        out.append("")
        out.append(f"    .awex_count = {len(awex_instances)}U,")
        out.append(f"    .awexs = {{{{")
        for ai, (aname, aaddrs) in enumerate(awex_instances):
            sep = ',' if ai < len(awex_instances) - 1 else ''
            out.append(f"        {{")
            for field in ['ctrl_address', 'fdemask_address', 'fdctrl_address', 'status_address',
                          'dtboth_address', 'dtbothbuf_address',
                          'dtls_address', 'dths_address',
                          'dtlsbuf_address', 'dthsbuf_address', 'outoven_address']:
                val = aaddrs.get(field, 0)
                if val:
                    out.append(f"            .{field} = {val}U,")
            out.append(f"        }}{sep}")
        out.append(f"    }}}},")
    
    # ---- RTC (Real-Time Counter) ----
    rtc_regs = regs.get(("RTC", "RTC"), {})
    if rtc_regs:
        # Find RTC interrupt vectors
        rtc_ovf_vec = 0
        rtc_pit_vec = 0
        for i in dev.findall('interrupts/interrupt-group'):
            if i.get('module-instance', '') == 'RTC':
                rtc_ovf_vec = parse_int(i.get('index', '0'))
            # PIT is usually RTC + 1 (common for XMEGA)
        rtc_pit_vec = rtc_ovf_vec + 1 if rtc_ovf_vec else 0
        
        out.append("")
        out.append(f"    .rtc_count = 1U,")
        out.append(f"    .timers_rtc = {{{{")
        rtc_fields = [
            ("CTRL", "ctrla_address"), ("STATUS", "status_address"),
            ("INTCTRL", "intctrl_address"), ("INTFLAGS", "intflags_address"),
            ("TEMP", "temp_address"), ("CALIB", "calib_address"),
            ("CNT", "cnt_address"), ("PER", "per_address"), ("COMP", "cmp_address"),
        ]
        out.append(f"        {{")
        for rn, field in rtc_fields:
            if rn in rtc_regs:
                out.append(f"            .{field} = {rtc_regs[rn]}U,")
        if rtc_ovf_vec:
            out.append(f"            .ovf_vector_index = {rtc_ovf_vec}U,")
        if rtc_pit_vec:
            out.append(f"            .pit_vector_index = {rtc_pit_vec}U,")
        out.append(f"        }}}}}},")
    
    # ---- EVSYS (Event System) ----
    evsys_regs = regs.get(("EVSYS", "EVSYS"), {})
    if evsys_regs:
        out.append("")
        strobe = evsys_regs.get("STROBE", 0)
        channels = evsys_regs.get("CH0MUX", 0)
        users = evsys_regs.get("CH0CTRL", 0)  # CH0CTRL serves as first user reg
        out.append(f"    .evsys = {{")
        if strobe:
            out.append(f"        .strobe_address = {strobe}U,")
        if channels:
            out.append(f"        .channels_address = {channels}U,")
        if users:
            out.append(f"        .users_address = {users}U,")
        out.append(f"        .channel_count = 8U,")
        out.append(f"    }},")
    
    # ---- CLKCTRL (Clock Controller) ----
    clk_regs = regs.get(("CLK", "CLK"), {})
    if clk_regs:
        out.append("")
        ctrla = clk_regs.get("CTRL", 0)
        out.append(f"    .clkctrl = {{")
        if ctrla:
            out.append(f"        .ctrla_address = {ctrla}U,")
        out.append(f"    }},")
    
    # ---- SLPCTRL (Sleep Controller) ----
    sleep_regs = regs.get(("SLEEP", "SLEEP"), {})
    if sleep_regs:
        ctrla = sleep_regs.get("CTRL", 0)
        out.append(f"    .slpctrl = {{ .ctrla_address = {ctrla}U }},")
    
    # ---- RSTCTRL (Reset Controller) ----
    rst_regs = regs.get(("RST", "RST"), {})
    if rst_regs:
        rstfr = rst_regs.get("STATUS", 0)
        out.append(f"    .rstctrl = {{ .rstfr_address = {rstfr}U }},")
    
    # ---- SYSCFG / MCU ----
    mcu_regs = regs.get(("MCU", "MCU"), {})
    if mcu_regs:
        reves = mcu_regs.get("DEVID0", 0)
        if not reves:
            reves = list(mcu_regs.values())[0] if mcu_regs else 0
        out.append(f"    .syscfg = {{ .reves_address = {reves}U }},")
    
    # ---- XMEGA USART (mapped to existing Uart8xDescriptor) ----
    usart_instances = []
    for (mn, iname), iregs in regs.items():
        if mn == 'USART' and re.match(r'^USART[A-Z]\d*$', iname):
            usart_instances.append((iname, iregs))
    if usart_instances:
        out.append("")
        out.append(f"    .uart8x_count = {len(usart_instances)}U,")
        out.append(f"    .uarts8x = {{{{")
        usart_map = [
            ("CTRLA","ctrla_address"), ("CTRLB","ctrlb_address"),
            ("CTRLC","ctrlc_address"), ("CTRLD","ctrld_address"),
            ("STATUS","status_address"), ("BAUDCTRLA","baud_address"),
            ("DATA","rxdata_address"), ("DATA","txdata_address"),
        ]
        for ui, (uname, uregs) in enumerate(usart_instances):
            sep = ',' if ui < len(usart_instances) - 1 else ''
            out.append(f"        {{")
            for rn, field in usart_map:
                if rn in uregs:
                    out.append(f"            .{field} = {uregs[rn]}U,")
            for i in dev.findall('interrupts/interrupt-group'):
                inst = i.get('module-instance', '')
                if inst == uname:
                    vec = parse_int(i.get('index', '0'))
                    out.append(f"            .rx_vector_index = {vec}U,")
                    out.append(f"            .tx_vector_index = {vec + 2}U,")
                    out.append(f"            .dre_vector_index = {vec + 1}U,")
                    break
            out.append(f"        }}{sep}")
        out.append(f"    }}}},")
    
    # ---- NVMCTRL (Non-Volatile Memory Controller) ----
    nvm_regs = regs.get(("NVM", "NVM"), {})
    if nvm_regs:
        out.append("")
        out.append(f"    .nvm_ctrl_count = 1U,")
        out.append(f"    .nvm_ctrls = {{{{")
        out.append(f"        {{")
        cmd = nvm_regs.get("CMD", 0)
        ctrl = nvm_regs.get("CTRLA", 0)
        intctrl = nvm_regs.get("INTCTRL", 0)
        status = nvm_regs.get("STATUS", 0)
        if ctrl:
            out.append(f"            .ctrla_address = {ctrl}U,")
        if status:
            out.append(f"            .status_address = {status}U,")
        if intctrl:
            out.append(f"            .intctrl_address = {intctrl}U,")
        out.append(f"        }}}}}},")
    
    # ---- CPUINT / PMIC (Programmable Multilevel Interrupt Controller) ----
    pmic_regs = regs.get(("PMIC", "PMIC"), {})
    if pmic_regs:
        out.append("")
        out.append(f"    .cpu_int_count = 1U,")
        out.append(f"    .cpu_ints = {{{{")
        out.append(f"        {{")
        ctrla = pmic_regs.get("CTRL", 0)
        status = pmic_regs.get("STATUS", 0)
        intpri = pmic_regs.get("INTPRI", 0)
        if ctrla:
            out.append(f"            .ctrla_address = {ctrla}U,")
        if status:
            out.append(f"            .status_address = {status}U,")
        if intpri:
            out.append(f"            .lvl0pri_address = {intpri}U,")
        out.append(f"        }}}}}},")
    
    # ---- XMEGA SPI (mapped to existing Spi8xDescriptor) ----
    spi_instances = []
    for (mn, iname), iregs in regs.items():
        if mn == 'SPI' and re.match(r'^SPI[A-Z]\d*$', iname):
            spi_instances.append((iname, iregs))
    if spi_instances:
        out.append("")
        out.append(f"    .spi8x_count = {len(spi_instances)}U,")
        out.append(f"    .spis8x = {{{{")
        spi_map = [
            ("CTRL","ctrla_address"), ("CTRLB","ctrlb_address"),
            ("INTCTRL","intctrl_address"), ("STATUS","intflags_address"),
            ("DATA","data_address"),
        ]
        for si, (sname, sregs) in enumerate(spi_instances):
            sep = ',' if si < len(spi_instances) - 1 else ''
            out.append(f"        {{")
            for rn, field in spi_map:
                if rn in sregs:
                    out.append(f"            .{field} = {sregs[rn]}U,")
            for i in dev.findall('interrupts/interrupt-group'):
                if i.get('module-instance', '') == sname:
                    vec = parse_int(i.get('index', '0'))
                    out.append(f"            .vector_index = {vec}U,")
                    break
            out.append(f"        }}{sep}")
        out.append(f"    }}}},")
    
    # ---- XMEGA TWI (mapped to existing Twi8xDescriptor) ----
    twi_instances = []
    for (mn, iname), iregs in regs.items():
        if mn == 'TWI' and re.match(r'^TWI[A-Z]\d*$', iname):
            twi_instances.append((iname, iregs))
    if twi_instances:
        out.append("")
        out.append(f"    .twi8x_count = {len(twi_instances)}U,")
        out.append(f"    .twis8x = {{{{")
        twi_map = [
            ("CTRLA","mctrla_address"), ("CTRLB","mctrlb_address"),
            ("STATUS","mstatus_address"), ("BAUD","mbaud_address"),
            ("ADDR","maddr_address"), ("DATA","mdata_address"),
        ]
        twi_slave_map = [
            ("CTRLA","sctrla_address"), ("CTRLB","sctrlb_address"),
            ("STATUS","sstatus_address"), ("ADDR","saddr_address"),
            ("DATA","sdata_address"), ("ADDRMASK","saddrmask_address"),
        ]
        for ti, (tname, tregs) in enumerate(twi_instances):
            sep = ',' if ti < len(twi_instances) - 1 else ''
            out.append(f"        {{")
            for rn, field in twi_map:
                if rn in tregs:
                    out.append(f"            .{field} = {tregs[rn]}U,")
            for rn, field in twi_slave_map:
                if rn in tregs:
                    out.append(f"            .{field} = {tregs[rn]}U,")
            out.append(f"        }}{sep}")
        out.append(f"    }}}},")
    
    # ---- WDT (Watchdog Timer) as wdt8x ----
    wdt_regs = regs.get(("WDT", "WDT"), {})
    if wdt_regs:
        out.append("")
        out.append(f"    .wdt8x_count = 1U,")
        out.append(f"    .wdts8x = {{{{")
        out.append(f"        {{")
        ctrla = wdt_regs.get("CTRL", 0)
        winctrl = wdt_regs.get("WINCTRL", 0)
        status = wdt_regs.get("STATUS", 0)
        if ctrla:
            out.append(f"            .ctrla_address = {ctrla}U,")
        if winctrl:
            out.append(f"            .winctrla_address = {winctrl}U,")
        if status:
            out.append(f"            .status_address = {status}U,")
        out.append(f"        }}}}}},")
    
    # ---- CRC (Cyclic Redundancy Check) as crc8x ----
    crc_regs = regs.get(("CRC", "CRC"), {})
    if crc_regs:
        out.append("")
        out.append(f"    .crc8x_count = 1U,")
        out.append(f"    .crcs8x = {{{{")
        out.append(f"        {{")
        ctrla = crc_regs.get("CTRL", 0)
        ctrlb = crc_regs.get("CTRLB", 0)
        status = crc_regs.get("STATUS", 0)
        data_in = crc_regs.get("DATAIN", 0)
        checksum = crc_regs.get("CHECKSUM0", 0)
        if ctrla:
            out.append(f"            .ctrla_address = {ctrla}U,")
        if ctrlb:
            out.append(f"            .ctrlb_address = {ctrlb}U,")
        if status:
            out.append(f"            .status_address = {status}U,")
        if data_in:
            out.append(f"            .data_address = {data_in}U,")
        if checksum:
            out.append(f"            .checksum_address = {checksum}U,")
        out.append(f"        }}}}}},")
    
    # ---- XMEGA USB (maps to existing Usb8xDescriptor) ----
    usb_regs = regs.get(("USB", "USB"), {})
    if usb_regs:
        out.append("")
        # Find USB interrupt vectors
        usb_gen_vec = 0
        usb_bus_vec = 0
        for i in dev.findall('interrupts/interrupt-group'):
            if i.get('module-instance', '') == 'USB':
                usb_bus_vec = parse_int(i.get('index', '0'))
                usb_gen_vec = usb_bus_vec + 1
                break
        out.append(f"    .usb8x_count = 1U,")
        out.append(f"    .usbs8x = {{{{")
        out.append(f"        {{")
        usb_map = [
            ("CTRLA","ctrla_address"), ("CTRLB","ctrlb_address"),
            ("STATUS","busstate_address"),
            ("ADDR","addr_address"), ("FIFOWP","fifowp_address"),
            ("FIFORP","fiforp_address"), ("EPPTR","epptr_address"),
            ("INTCTRLA","intctrla_address"), ("INTCTRLB","intctrlb_address"),
            ("INTFLAGSACLR","intflagsa_address"), ("INTFLAGSBCLR","intflagsb_address"),

        ]
        for rn, field in usb_map:
            if rn in usb_regs:
                out.append(f"            .{field} = {usb_regs[rn]}U,")
        # cal0/cal1 fields after intflagsb_address in struct
        cal0 = usb_regs.get("CAL0", 0)
        cal1 = usb_regs.get("CAL1", 0)
        if cal0:
            out.append(f"            .cal0_address = {cal0}U,")
        if cal1:
            out.append(f"            .cal1_address = {cal1}U,")
        # pllcsr not present on XMEGA USB, skip
        if usb_gen_vec:
            out.append(f"            .gen_vector_index = {usb_gen_vec}U,")
        if usb_bus_vec:
            out.append(f"            .bus_vector_index = {usb_bus_vec}U,")
        out.append(f"        }}}}}},")
    
    # ---- XMEGA DAC (has 2 channels, richer register set) ----
    xmega_dac_instances = []
    for (mn, iname), iregs in regs.items():
        if mn == 'DAC' and re.match(r'^DAC[A-Z]$', iname):
            xmega_dac_instances.append((iname, iregs))
    xmega_dac_vec = 0
    for i in dev.findall('interrupts/interrupt-group'):
        if i.get('module-instance', '') == 'DAC':
            xmega_dac_vec = parse_int(i.get('index', '0'))
            break
    if xmega_dac_instances:
        out.append("")
        out.append(f"    .xmega_dac_count = {len(xmega_dac_instances)}U,")
        out.append(f"    .xmega_dacs = {{{{")
        for di, (dn, dregs) in enumerate(xmega_dac_instances):
            sep = ',' if di < len(xmega_dac_instances) - 1 else ''
            out.append(f"        {{")
            dac_map = [
                ("CTRLA","ctrla_address"), ("CTRLB","ctrlb_address"),
                ("CTRLC","ctrlc_address"), ("EVCTRL","evctrl_address"),
                ("STATUS","status_address"),
                ("CH0GAINCAL","ch0gaincal_address"), ("CH0OFFSETCAL","ch0offsetcal_address"),
                ("CH1GAINCAL","ch1gaincal_address"), ("CH1OFFSETCAL","ch1offsetcal_address"),
                ("CH0DATA","ch0data_address"), ("CH1DATA","ch1data_address"),
            ]
            for rn, field in dac_map:
                if rn in dregs:
                    out.append(f"            .{field} = {dregs[rn]}U,")
            if xmega_dac_vec:
                out.append(f"            .vector_index = {xmega_dac_vec}U,")
            out.append(f"        }}{sep}")
        out.append(f"    }}}},")
    
    # ---- EBI (External Bus Interface) ----
    ebi_regs = regs.get(("EBI", "EBI"), {})
    if ebi_regs:
        out.append("")
        out.append(f"    .ebi_count = 1U,")
        out.append(f"    .ebis = {{{{")
        out.append(f"        {{")
        ebi_base_map = [
            ("CTRL","ctrl_address"), ("SDRAMCTRLA","sdramctrla_address"),
            ("REFRESH","refresh_address"), ("INITDLY","initdly_address"),
            ("SDRAMCTRLB","sdramctrlb_address"), ("SDRAMCTRLC","sdramctrlc_address"),
        ]
        for rn, field in ebi_base_map:
            if rn in ebi_regs:
                out.append(f"            .{field} = {ebi_regs[rn]}U,")
        # CS0..CS3 registers at base+0x10, base+0x14, base+0x18, base+0x1C
        ebi_base = ebi_regs.get("CTRL", 0)
        if ebi_base:
            for cs_idx in range(4):
                cs_base = ebi_base + 0x10 + cs_idx * 4
                out.append(f"            .cs{cs_idx}_ctrla_address = {cs_base}U,")
                out.append(f"            .cs{cs_idx}_ctrlb_address = {cs_base + 1}U,")
                out.append(f"            .cs{cs_idx}_baseaddr_address = {cs_base + 2}U,")
        out.append(f"        }}}}}},")
    
    # ---- IRCOM (IR Communication Module) ----
    ircom_regs = regs.get(("IRCOM", "IRCOM"), {})
    if ircom_regs:
        out.append("")
        out.append(f"    .ircom_count = 1U,")
        out.append(f"    .ircoms = {{{{")
        out.append(f"        {{")
        ircom_map = [
            ("CTRL","ctrl_address"), ("TXPLCTRL","txplctrl_address"),
            ("RXPLCTRL","rxplctrl_address"),
        ]
        for rn, field in ircom_map:
            if rn in ircom_regs:
                out.append(f"            .{field} = {ircom_regs[rn]}U,")
        out.append(f"        }}}}}},")
    
    # ---- DMA (Direct Memory Access) ----
    dma_regs = regs.get(("DMA", "DMA"), {})
    if dma_regs:
        out.append("")
        out.append(f"    .dma_count = 1U,")
        out.append(f"    .dmas = {{{{")
        out.append(f"        {{")
        ctrla = dma_regs.get("CTRL", 0)
        intflags = dma_regs.get("INTFLAGS", 0)
        status = dma_regs.get("STATUS", 0)
        if ctrla:
            out.append(f"            .ctrla_address = {ctrla}U,")
        if status:
            out.append(f"            .status_address = {status}U,")
        if intflags:
            out.append(f"            .intflags_address = {intflags}U,")
        out.append(f"        }}}}}},")
    
    # ---- Signature/fuse fields (BEFORE port_count in struct order) ----
    out.append(f"    .fuse_address = 0x0000U,")
    out.append(f"    .lockbit_address = 0x0000U,")
    out.append(f"    .signature_address = 0x0000U,")
    out.append(f"    .signature = {{ 0x{sig0:02X}U, 0x{sig1:02X}U, 0x{sig2:02X}U }},")
    
    # Ports
    out.append("")
    out.append(f"    .port_count = {len(port_instances)}U,")
    out.append(f"    .ports = {{{{")
    for pi, (pl, iregs) in enumerate(port_instances):
        dir_addr = iregs.get('DIR', 0)
        dirset_addr = iregs.get('DIRSET', 0)
        dirclr_addr = iregs.get('DIRCLR', 0)
        dirtgl_addr = iregs.get('DIRTGL', 0)
        out_addr = iregs.get('OUT', 0)
        outset_addr = iregs.get('OUTSET', 0)
        outclr_addr = iregs.get('OUTCLR', 0)
        outtgl_addr = iregs.get('OUTTGL', 0)
        in_addr = iregs.get('IN', 0)
        intflags_addr = iregs.get('INTFLAGS', 0)
        
        # Find interrupt vector for this port
        port_vector = 0xFF
        for i in dev.findall('interrupts/interrupt-group'):
            if i.get('module-instance', '') == f'PORT{pl}':
                port_vector = parse_int(i.get('index', '0xFF'))
                break
        
        separator = ',' if pi < len(port_instances) - 1 else ''
        out.append(f"        {{")
        out.append(f"            .name = \"PORT{pl}\",")
        if in_addr:
            out.append(f"            .pin_address = 0x{in_addr:04X}U,")
        if dir_addr:
            out.append(f"            .ddr_address = 0x{dir_addr:04X}U,")
        if out_addr:
            out.append(f"            .port_address = 0x{out_addr:04X}U,")
        if dirset_addr:
            out.append(f"            .dirset_address = 0x{dirset_addr:04X}U,")
        if dirclr_addr:
            out.append(f"            .dirclr_address = 0x{dirclr_addr:04X}U,")
        if dirtgl_addr:
            out.append(f"            .dirtgl_address = 0x{dirtgl_addr:04X}U,")
        if outset_addr:
            out.append(f"            .outset_address = 0x{outset_addr:04X}U,")
        if outclr_addr:
            out.append(f"            .outclr_address = 0x{outclr_addr:04X}U,")
        if outtgl_addr:
            out.append(f"            .outtgl_address = 0x{outtgl_addr:04X}U,")
        # pin_ctrl_base (PIN0CTRL address, comes before intflags_address in struct)
        pin_ctrl_addr = iregs.get('PIN0CTRL', 0)
        if pin_ctrl_addr:
            out.append(f"            .pin_ctrl_base = 0x{pin_ctrl_addr:04X}U,")
        if intflags_addr:
            out.append(f"            .intflags_address = 0x{intflags_addr:04X}U,")
        # VPORT base (must come before vector_index in struct)
        vp_base = vport_bases.get(pl, 0)
        if vp_base:
            out.append(f"            .vport_base = 0x{vp_base:04X}U,")
        if port_vector != 0xFF:
            out.append(f"            .vector_index = {port_vector}U,")
        # REMAP register (XMEGA port routing)
        remap_addr = iregs.get('REMAP', 0)
        if remap_addr:
            out.append(f"            .remap_address = 0x{remap_addr:04X}U,")
        separator = ',' if pi < len(port_instances) - 1 else ''
        out.append(f"        }}{separator}")
    out.append(f"    }}}},")
    
    out.append(f"}};")
    out.append("")
    out.append(f"}}  // namespace vioavr::core::devices")
    out.append("")
    
    output_file.write("\n".join(out))
    return True


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Generate VioAVR device descriptor from ATDF")
    parser.add_argument("atdf", nargs="?", help="Path to ATDF XML file")
    parser.add_argument("--all", action="store_true", help="Generate all ATtiny descriptors")
    parser.add_argument("--all-classic", action="store_true", help="Generate all ATmega/AT90 classic descriptors")
    parser.add_argument("--all-avr-dx", action="store_true", help="Generate all AVR-DA/DB/DD/DU descriptors")
    parser.add_argument("--all-avr-ex", action="store_true", help="Generate all AVR-EA/EB descriptors")
    parser.add_argument("--all-avr-lx", action="store_true", help="Generate all AVR-LA descriptors")
    parser.add_argument("--all-avr-sx", action="store_true", help="Generate all AVR-SD descriptors")
    parser.add_argument("--all-xmega", action="store_true", help="Generate all XMEGA descriptors")
    parser.add_argument("--output-dir", default=None, help="Output directory for --all / --all-classic / --all-avr-dx")
    args = parser.parse_args()
    
    base_dir = Path(__file__).parent.parent
    output_dir = Path(args.output_dir) if args.output_dir else base_dir / "include" / "vioavr" / "core" / "devices"
    output_dir.mkdir(parents=True, exist_ok=True)
    
    if args.all:
        atdf_dir = base_dir / "avr-pack" / "attiny" / "atdf"
        if not atdf_dir.exists():
            print(f"ATDF directory not found: {atdf_dir}", file=sys.stderr)
            return 1
        generate_all(str(atdf_dir / "ATtiny*.atdf"), output_dir, "ATtiny")
        return 0
    
    if args.all_classic:
        atmega_dir = base_dir / "avr-pack" / "atmega" / "atdf"
        if not atmega_dir.exists():
            print(f"ATmega ATDF directory not found: {atmega_dir}", file=sys.stderr)
            return 1
        generate_all(str(atmega_dir / "ATmega*.atdf"), output_dir, "ATmega/AT90")
        generate_all(str(atmega_dir / "AT90*.atdf"), output_dir, "AT90")
        return 0
    
    if args.all_avr_dx:
        avrdx_dir = base_dir / "avr-pack" / "avr-dx" / "atdf"
        if not avrdx_dir.exists():
            print(f"AVR-Dx ATDF directory not found: {avrdx_dir}", file=sys.stderr)
            return 1
        generate_all(str(avrdx_dir / "AVR*.atdf"), output_dir, "AVR-Dx")
        return 0
    
    if args.all_avr_ex:
        ex_dir = base_dir / "avr-pack" / "avr-ex" / "atdf"
        if not ex_dir.exists():
            print(f"AVR-Ex ATDF directory not found: {ex_dir}", file=sys.stderr)
            return 1
        generate_all(str(ex_dir / "AVR*.atdf"), output_dir, "AVR-Ex")
        return 0
    
    if args.all_avr_lx:
        lx_dir = base_dir / "avr-pack" / "avr-lx" / "atdf"
        if not lx_dir.exists():
            print(f"AVR-Lx ATDF directory not found: {lx_dir}", file=sys.stderr)
            return 1
        generate_all(str(lx_dir / "AVR*.atdf"), output_dir, "AVR-Lx")
        return 0
    
    if args.all_avr_sx:
        sx_dir = base_dir / "avr-pack" / "avr-sx" / "atdf"
        if not sx_dir.exists():
            print(f"AVR-Sx ATDF directory not found: {sx_dir}", file=sys.stderr)
            return 1
        generate_all(str(sx_dir / "AVR*.atdf"), output_dir, "AVR-Sx")
        return 0
    
    if args.all_xmega:
        xmega_dir = base_dir / "avr-pack" / "xmega" / "atdf"
        if not xmega_dir.exists():
            print(f"XMEGA ATDF directory not found: {xmega_dir}", file=sys.stderr)
            return 1
        xmega_out_dir = output_dir / "xmega"
        xmega_out_dir.mkdir(parents=True, exist_ok=True)
        count = 0
        import glob as gglob
        for atdf_path in sorted(gglob.glob(str(xmega_dir / "ATxmega*.atdf"))):
            chip = Path(atdf_path).stem.lower()
            out_path = xmega_out_dir / f"{chip}.hpp"
            print(f"  {Path(atdf_path).stem} -> {out_path.name}")
            try:
                tree = ET.parse(atdf_path)
                with open(str(out_path), "w") as f:
                    if not generate_xmega(tree.getroot(), f):
                        print(f"    FAILED", file=sys.stderr)
                count += 1
            except Exception as e:
                print(f"    ERROR: {e}", file=sys.stderr)
                import traceback
                traceback.print_exc()
        print(f"  Generated {count} XMEGA descriptor files")
        return 0
    
    if not args.atdf:
        parser.print_help()
        return 1
    
    tree = ET.parse(args.atdf)
    root = tree.getroot()
    # Auto-detect format
    architecture = root.find('.//device')
    arch = architecture.get('architecture', '') if architecture is not None else ''
    if arch == 'AVR8_XMEGA':
        generate_xmega(root)
    else:
        generate(root)
    return 0


if __name__ == "__main__":
    sys.exit(main())
