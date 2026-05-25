#!/usr/bin/env python3
"""Compare VioAVR device descriptor against Microchip DFP iom*.h header."""

import re
import sys
import subprocess
import json

def parse_iom_header(header_path):
    """Parse _SFR_MEM8 / _SFR_IO8 defines from iom*.h header file."""
    regs = {}
    with open(header_path) as f:
        for line in f:
            line = line.strip()
            # Match: #define REGNAME _SFR_MEM8(addr) or _SFR_IO8(addr) or _SFR_MEM16(addr) etc
            m = re.match(r'#define\s+(\w+)\s+_SFR_(MEM|IO)(8|16)\(0x([0-9a-fA-F]+)\)', line)
            if m:
                name = m.group(1)
                addr = int(m.group(4), 16)
                regs[name] = addr
            # Also match _SFR_MEM32
            m = re.match(r'#define\s+(\w+)\s+_SFR_MEM32\(0x([0-9a-fA-F]+)\)', line)
            if m:
                regs[m.group(1)] = int(m.group(2), 16)
    return regs


def get_vioavr_dump(device_name, dump_tool="/tmp/dump_dev"):
    """Run the C++ dumper and parse output."""
    result = subprocess.run([dump_tool, device_name], capture_output=True, text=True)
    if result.returncode != 0:
        print(f"ERROR running dumper for {device_name}: {result.stderr}")
        return None
    return result.stdout


def check_field(dump_lines, label, ref_name, ref_addr, iom_regs, errors):
    """Check a VioAVR field against a reference from the iom header."""
    ref_val = iom_regs.get(ref_name)
    if ref_val is None:
        errors.append(f"  MISSING REF: {ref_name} not found in iom header")
        return

    for line in dump_lines:
        if f"{label}" in line:
            # Extract hex value
            m = re.search(r'0x([0-9a-fA-F]+)', line)
            if m:
                vio_val = int(m.group(1), 16)
                if vio_val != ref_val:
                    errors.append(f"  MISMATCH: {label} (vio=0x{vio_val:04x}, ref={ref_name}=0x{ref_val:04x})")
                return
    errors.append(f"  NOT FOUND: {label} not in dump")


def check_port(dump_lines, port_idx, ref_name_prefix, iom_regs, errors):
    """Check port register addresses."""
    prefix = f"port[{port_idx}]:"
    port_line = None
    for line in dump_lines:
        if line.startswith(prefix):
            port_line = line
            break

    if port_line is None:
        errors.append(f"  Port {port_idx} not found")
        return

    # Parse the port line
    # port[N]: name=PORTX pin=0x... ddr=0x... port=0x... dirset=0x... ...

    fields = {}
    for part in port_line.split():
        if '=' in part:
            k, v = part.split('=', 1)
            fields[k] = v

    port_name = fields.get('name', '?')

    # Check PIN (IN)
    ref_pin = iom_regs.get(f"{ref_name_prefix}_IN")
    if ref_pin is not None:
        pin_val = int(fields.get('pin', '0x0'), 16)
        if pin_val != ref_pin:
            errors.append(f"  {port_name}.PIN: vio=0x{pin_val:04x} ref={ref_pin:04x}")

    # Check DDR (DIR)
    ref_ddr = iom_regs.get(f"{ref_name_prefix}_DIR")
    if ref_ddr is not None:
        ddr_val = int(fields.get('ddr', '0x0'), 16)
        if ddr_val != ref_ddr:
            errors.append(f"  {port_name}.DIR: vio=0x{ddr_val:04x} ref={ref_ddr:04x}")

    # Check PORT (OUT)
    ref_port = iom_regs.get(f"{ref_name_prefix}_OUT")
    if ref_port is not None:
        port_val = int(fields.get('port', '0x0'), 16)
        if port_val != ref_port:
            errors.append(f"  {port_name}.OUT: vio=0x{port_val:04x} ref={ref_port:04x}")


def compare_device(device_name):
    """Main comparison for a device."""
    print(f"\n{'='*60}")
    print(f"Comparing {device_name}")
    print(f"{'='*60}")

    # Determine the iom header path
    # Map device names to header names
    name_map = {
        "atmega328p": "iom328p.h",
        "atmega4809": "iom4809.h",
        "atmega2560": "iom2560.h",
        "atmega32u4": "iom32u4.h",
        "attiny85": "iotn85.h",
        "at90pwm3": "io90pwm3.h",
        "atmega808": "iom808.h",
        "atmega1608": "iom1608.h",
        "atmega3208": "iom3208.h",
        "atmega4808": "iom4808.h",
        "atmega1609": "iom1609.h",
        "atmega3209": "iom3209.h",
        "atmega809": "iom809.h",
    }

    header_name = name_map.get(device_name)
    if header_name is None:
        print(f"  Unknown iom header for {device_name}")
        return

    header_path = f"/home/jnd/cpp_projects/VioAVR/avr-pack/atmega/include/avr/{header_name}"
    try:
        iom_regs = parse_iom_header(header_path)
    except FileNotFoundError:
        print(f"  Header not found: {header_path}")
        return

    dump = get_vioavr_dump(device_name)
    if dump is None:
        return

    lines = dump.split('\n')

    # Filter out section headers and blank lines for easier matching
    data_lines = [l for l in lines if l and not l.startswith('===')]

    errors = []

    # --- System registers (these are on one line in the dump: "spl=0x... sph=0x... sreg=0x...")
    # Parse the system registers line
    for line in data_lines:
        if line.startswith('  spl='):
            parts = line.split()
            for part in parts:
                if '=' in part:
                    k, v = part.split('=', 1)
                    if k == 'spl':
                        ref_spl = iom_regs.get('CPU_SPL')
                        if ref_spl is not None:
                            vio_val = int(v, 16)
                            if vio_val != ref_spl:
                                errors.append(f"  SPL: vio=0x{vio_val:04x} ref=CPU_SPL=0x{ref_spl:04x}")
                    elif k == 'sph':
                        ref_sph = iom_regs.get('CPU_SPH')
                        if ref_sph is not None:
                            vio_val = int(v, 16)
                            if vio_val != ref_sph:
                                errors.append(f"  SPH: vio=0x{vio_val:04x} ref=CPU_SPH=0x{ref_sph:04x}")
                    elif k == 'sreg':
                        ref_sreg = iom_regs.get('CPU_SREG')
                        if ref_sreg is not None:
                            vio_val = int(v, 16)
                            if vio_val != ref_sreg:
                                errors.append(f"  SREG: vio=0x{vio_val:04x} ref=CPU_SREG=0x{ref_sreg:04x}")
            break

    # --- Ports ---
    # Count ports
    port_count = 0
    for line in data_lines:
        m = re.match(r'port\[(\d+)\]: name=(\w+)', line)
        if m:
            port_count = max(port_count, int(m.group(1)) + 1)
            port_name = m.group(2)
            check_port(data_lines, int(m.group(1)), port_name, iom_regs, errors)

    # --- TCA ---
    tca_count = 0
    for line in data_lines:
        m = re.match(r'  tca\[(\d+)\]:', line)
        if m:
            tca_count = max(tca_count, int(m.group(1)) + 1)
            idx = int(m.group(1))
            inst_name = f"TCA{idx}"
            fields = {}
            for part in line.split():
                if '=' in part:
                    k, v = part.split('=', 1)
                    fields[k] = v

            tca_checks = [
                ("ctrla", f"{inst_name}_CTRLA"),
                ("ctrlb", f"{inst_name}_CTRLB"),
                ("ctrlc", f"{inst_name}_CTRLC"),
                ("ctrld", f"{inst_name}_CTRLD"),
                ("evctrl", f"{inst_name}_EVCTRL"),
                ("intctrl", f"{inst_name}_INTCTRL"),
                ("intflags", f"{inst_name}_INTFLAGS"),
                ("dbgctrl", f"{inst_name}_DBGCTRL"),
                ("temp", f"{inst_name}_TEMP"),
                ("cnt", f"{inst_name}_CNT"),
                ("per", f"{inst_name}_PER"),
                ("cmp0", f"{inst_name}_CMP0"),
                ("cmp1", f"{inst_name}_CMP1"),
                ("cmp2", f"{inst_name}_CMP2"),
            ]
            for label, ref_name in tca_checks:
                vio_val_str = fields.get(label, "0x0")
                vio_val = int(vio_val_str, 16)
                ref_val = iom_regs.get(ref_name)
                if ref_val is not None and vio_val != ref_val:
                    errors.append(f"  TCA{idx}.{label}: vio=0x{vio_val:04x} ref={ref_name}=0x{ref_val:04x}")

    # --- TCB ---
    for line in data_lines:
        m = re.match(r'  tcb\[(\d+)\]:', line)
        if m:
            idx = int(m.group(1))
            inst_name = f"TCB{idx}"
            fields = {}
            for part in line.split():
                if '=' in part:
                    k, v = part.split('=', 1)
                    fields[k] = v

            tcb_checks = [
                ("ctrla", f"{inst_name}_CTRLA"),
                ("ctrlb", f"{inst_name}_CTRLB"),
                ("evctrl", f"{inst_name}_EVCTRL"),
                ("intctrl", f"{inst_name}_INTCTRL"),
                ("intflags", f"{inst_name}_INTFLAGS"),
                ("cnt", f"{inst_name}_CNT"),
                ("ccmp", f"{inst_name}_CCMP"),
            ]
            for label, ref_name in tcb_checks:
                vio_val_str = fields.get(label, "0x0")
                vio_val = int(vio_val_str, 16)
                ref_val = iom_regs.get(ref_name)
                if ref_val is not None and vio_val != ref_val:
                    errors.append(f"  TCB{idx}.{label}: vio=0x{vio_val:04x} ref={ref_name}=0x{ref_val:04x}")

    # --- RTC ---
    for line in data_lines:
        if 'rtc[0]' in line:
            fields = {}
            for part in line.split():
                if '=' in part:
                    k, v = part.split('=', 1)
                    fields[k] = v
            rtc_checks = [
                ("ctrla", "RTC_CTRLA"),
                ("status", "RTC_STATUS"),
                ("intctrl", "RTC_INTCTRL"),
                ("intflags", "RTC_INTFLAGS"),
                ("temp", "RTC_TEMP"),
                ("cnt", "RTC_CNT"),
                ("per", "RTC_PER"),
                ("cmp", "RTC_CMP"),
            ]
            for label, ref_name in rtc_checks:
                vio_val_str = fields.get(label, "0x0")
                vio_val = int(vio_val_str, 16)
                ref_val = iom_regs.get(ref_name)
                if ref_val is not None and vio_val != ref_val:
                    errors.append(f"  RTC.{label}: vio=0x{vio_val:04x} ref={ref_name}=0x{ref_val:04x}")

    # --- CLKCTRL ---
    for line in data_lines:
        if 'clkctrl:' in line:
            fields = {}
            for part in line.split():
                if '=' in part:
                    k, v = part.split('=', 1)
                    fields[k] = v
            clk_checks = [
                ("ctrla", "CLKCTRL_MCLKCTRLA"),
                ("ctrlb", "CLKCTRL_MCLKCTRLB"),
                ("mclklock", "CLKCTRL_MCLKLOCK"),
                ("mclkstatus", "CLKCTRL_MCLKSTATUS"),
                ("osc20mctrla", "CLKCTRL_OSC20MCTRLA"),
                ("osc20mcalib", "CLKCTRL_OSC20MCALIBA"),
                ("osc32kctrla", "CLKCTRL_OSC32KCTRLA"),
                ("xosc32kctrla", "CLKCTRL_XOSC32KCTRLA"),
            ]
            for label, ref_name in clk_checks:
                vio_val_str = fields.get(label, "0x0")
                vio_val = int(vio_val_str, 16)
                ref_val = iom_regs.get(ref_name)
                if ref_val is not None and vio_val != ref_val:
                    errors.append(f"  CLKCTRL.{label}: vio=0x{vio_val:04x} ref={ref_name}=0x{ref_val:04x}")

    # --- SLPCTRL ---
    for line in data_lines:
        if 'slpctrl:' in line:
            fields = {}
            for part in line.split():
                if '=' in part:
                    k, v = part.split('=', 1)
                    fields[k] = v
            vio_val_str = fields.get('ctrla', '0x0')
            vio_val = int(vio_val_str, 16)
            ref_val = iom_regs.get('SLPCTRL_CTRLA')
            if ref_val is not None and vio_val != ref_val:
                errors.append(f"  SLPCTRL.CTRLA: vio=0x{vio_val:04x} ref=SLPCTRL_CTRLA=0x{ref_val:04x}")

    # --- RSTCTRL ---
    for line in data_lines:
        if 'rstctrl:' in line:
            fields = {}
            for part in line.split():
                if '=' in part:
                    k, v = part.split('=', 1)
                    fields[k] = v
            vio_val_str = fields.get('rstfr', '0x0')
            vio_val = int(vio_val_str, 16)
            ref_val = iom_regs.get('RSTCTRL_RSTFR')
            if ref_val is not None and vio_val != ref_val:
                errors.append(f"  RSTCTRL.RSTFR: vio=0x{vio_val:04x} ref=RSTCTRL_RSTFR=0x{ref_val:04x}")

    # --- BOD ---
    for line in data_lines:
        if line.startswith('  bod:'):
            fields = {}
            for part in line.split():
                if '=' in part:
                    k, v = part.split('=', 1)
                    fields[k] = v
            bod_checks = [
                ("ctrla", "BOD_CTRLA"),
                ("ctrlb", "BOD_CTRLB"),
                ("vlmctrla", "BOD_VLMCTRLA"),
                ("intctrl", "BOD_INTCTRL"),
                ("intflags", "BOD_INTFLAGS"),
                ("status", "BOD_STATUS"),
            ]
            for label, ref_name in bod_checks:
                vio_val_str = fields.get(label, '0x0')
                vio_val = int(vio_val_str, 16)
                ref_val = iom_regs.get(ref_name)
                if ref_val is not None and vio_val != ref_val:
                    errors.append(f"  BOD.{label}: vio=0x{vio_val:04x} ref={ref_name}=0x{ref_val:04x}")

    # --- CPUINT ---
    for line in data_lines:
        if 'cpuint[0]:' in line:
            fields = {}
            for part in line.split():
                if '=' in part:
                    k, v = part.split('=', 1)
                    fields[k] = v
            cpuint_checks = [
                ("ctrla", "CPUINT_CTRLA"),
                ("status", "CPUINT_STATUS"),
                ("lvl0pri", "CPUINT_LVL0PRI"),
                ("lvl1vec", "CPUINT_LVL1VEC"),
            ]
            for label, ref_name in cpuint_checks:
                vio_val_str = fields.get(label, '0x0')
                vio_val = int(vio_val_str, 16)
                ref_val = iom_regs.get(ref_name)
                if ref_val is not None and vio_val != ref_val:
                    errors.append(f"  CPUINT.{label}: vio=0x{vio_val:04x} ref={ref_name}=0x{ref_val:04x}")

    # --- NVMCTRL ---
    for line in data_lines:
        if 'nvmctrl[0]:' in line:
            fields = {}
            for part in line.split():
                if '=' in part:
                    k, v = part.split('=', 1)
                    fields[k] = v
            nvm_checks = [
                ("ctrla", "NVMCTRL_CTRLA"),
                ("ctrlb", "NVMCTRL_CTRLB"),
                ("status", "NVMCTRL_STATUS"),
                ("intctrl", "NVMCTRL_INTCTRL"),
                ("intflags", "NVMCTRL_INTFLAGS"),
                ("addr", "NVMCTRL_ADDR"),
                ("data", "NVMCTRL_DATA"),
            ]
            for label, ref_name in nvm_checks:
                vio_val_str = fields.get(label, '0x0')
                vio_val = int(vio_val_str, 16)
                ref_val = iom_regs.get(ref_name)
                if ref_val is not None and vio_val != ref_val:
                    errors.append(f"  NVMCTRL.{label}: vio=0x{vio_val:04x} ref={ref_name}=0x{ref_val:04x}")

    # --- WDT ---
    for line in data_lines:
        if 'wdt8x[0]:' in line:
            fields = {}
            for part in line.split():
                if '=' in part:
                    k, v = part.split('=', 1)
                    fields[k] = v
            vio_val_str = fields.get('ctrla', '0x0')
            vio_val = int(vio_val_str, 16)
            ref_val = iom_regs.get('WDT_CTRLA')
            if ref_val is not None and vio_val != ref_val:
                errors.append(f"  WDT.CTRLA: vio=0x{vio_val:04x} ref=WDT_CTRLA=0x{ref_val:04x}")

    # --- ADC8X ---
    for line in data_lines:
        m = re.match(r'  adc8x\[(\d+)\]:', line)
        if m:
            idx = int(m.group(1))
            inst_name = f"ADC{idx}"
            fields = {}
            for part in line.split():
                if '=' in part:
                    k, v = part.split('=', 1)
                    fields[k] = v
            adc_checks = [
                ("ctrla", f"{inst_name}_CTRLA"),
                ("ctrlb", f"{inst_name}_CTRLB"),
                ("ctrlc", f"{inst_name}_CTRLC"),
                ("ctrld", f"{inst_name}_CTRLD"),
                ("ctrle", f"{inst_name}_CTRLE"),
                ("sampctrl", f"{inst_name}_SAMPCTRL"),
                ("muxpos", f"{inst_name}_MUXPOS"),
                ("cmd", f"{inst_name}_COMMAND"),
                ("evctrl", f"{inst_name}_EVCTRL"),
                ("intctrl", f"{inst_name}_INTCTRL"),
                ("intflags", f"{inst_name}_INTFLAGS"),
                ("dbgctrl", f"{inst_name}_DBGCTRL"),
                ("temp", f"{inst_name}_TEMP"),
                ("res", f"{inst_name}_RES"),
                ("winlt", f"{inst_name}_WINLT"),
                ("winht", f"{inst_name}_WINHT"),
            ]
            for label, ref_name in adc_checks:
                vio_val_str = fields.get(label, '0x0')
                vio_val = int(vio_val_str, 16)
                ref_val = iom_regs.get(ref_name)
                if ref_val is not None and vio_val != ref_val:
                    errors.append(f"  {inst_name}.{label}: vio=0x{vio_val:04x} ref={ref_name}=0x{ref_val:04x}")

    # --- AC8X ---
    for line in data_lines:
        m = re.match(r'  ac8x\[(\d+)\]:', line)
        if m:
            idx = int(m.group(1))
            inst_name = f"AC{idx}"
            fields = {}
            for part in line.split():
                if '=' in part:
                    k, v = part.split('=', 1)
                    fields[k] = v
            ac_checks = [
                ("ctrla", f"{inst_name}_CTRLA"),
                ("muxctrla", f"{inst_name}_MUXCTRLA"),
                ("dacctrla", f"{inst_name}_DACREF"),
                ("intctrl", f"{inst_name}_INTCTRL"),
                ("status", f"{inst_name}_STATUS"),
            ]
            for label, ref_name in ac_checks:
                vio_val_str = fields.get(label, '0x0')
                vio_val = int(vio_val_str, 16)
                ref_val = iom_regs.get(ref_name)
                if ref_val is not None and vio_val != ref_val:
                    errors.append(f"  {inst_name}.{label}: vio=0x{vio_val:04x} ref={ref_name}=0x{ref_val:04x}")

    # --- USART ---
    for line in data_lines:
        m = re.match(r'  uart8x\[(\d+)\]:', line)
        if m:
            idx = int(m.group(1))
            inst_name = f"USART{idx}"
            fields = {}
            for part in line.split():
                if '=' in part:
                    k, v = part.split('=', 1)
                    fields[k] = v
            uart_checks = [
                ("ctrla", f"{inst_name}_CTRLA"),
                ("ctrlb", f"{inst_name}_CTRLB"),
                ("ctrlc", f"{inst_name}_CTRLC"),
                ("ctrld", f"{inst_name}_CTRLD"),
                ("status", f"{inst_name}_STATUS"),
                ("baud", f"{inst_name}_BAUD"),
                ("rxdata", f"{inst_name}_RXDATAL"),
                ("txdata", f"{inst_name}_TXDATAL"),
                ("dbgctrl", f"{inst_name}_DBGCTRL"),
                ("evctrl", f"{inst_name}_EVCTRL"),
            ]
            for label, ref_name in uart_checks:
                vio_val_str = fields.get(label, '0x0')
                vio_val = int(vio_val_str, 16)
                ref_val = iom_regs.get(ref_name)
                if ref_val is not None and vio_val != ref_val:
                    errors.append(f"  {inst_name}.{label}: vio=0x{vio_val:04x} ref={ref_name}=0x{ref_val:04x}")

    # --- EVSYS ---
    for line in data_lines:
        if line.startswith('  evsys:'):
            fields = {}
            for part in line.split():
                if '=' in part:
                    k, v = part.split('=', 1)
                    fields[k] = v
            evsys_checks = [
                ("strobe", "EVSYS_STROBE"),
                ("channels", "EVSYS_CHANNEL0"),
                ("users", "EVSYS_USER_CCL_LUT0"),
            ]
            for label, ref_name in evsys_checks:
                vio_val_str = fields.get(label, '0x0')
                vio_val = int(vio_val_str, 16)
                ref_val = iom_regs.get(ref_name)
                if ref_val is not None and vio_val != ref_val:
                    errors.append(f"  EVSYS.{label}: vio=0x{vio_val:04x} ref={ref_name}=0x{ref_val:04x}")

    # --- CCP, SREG in system header ---
    ccp_val = iom_regs.get('CCP')
    if ccp_val is not None:
        for line in data_lines:
            if 'ccp=' in line:
                m = re.search(r'ccp=0x([0-9a-fA-F]+)', line)
                if m:
                    v = int(m.group(1), 16)
                    if v != ccp_val:
                        errors.append(f"  CCP: vio=0x{v:04x} ref=CCP=0x{ccp_val:04x}")

    if errors:
        print(f"  DISCREPANCIES FOUND ({len(errors)}):")
        for e in errors:
            print(e)
    else:
        print(f"  ALL REGISTERS MATCH - no discrepancies found")


if __name__ == "__main__":
    devices = sys.argv[1:] if len(sys.argv) > 1 else [
        "atmega4809", "atmega4808", "atmega3209", "atmega3208",
        "atmega1609", "atmega1608", "atmega809", "atmega808",
        "atmega328p", "atmega2560", "atmega32u4"
    ]
    for dev in devices:
        compare_device(dev)
