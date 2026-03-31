import sys
import os

def create_hex(filename, opcodes):
    """
    opcodes: list of 16-bit integers (in CPU word order, little endian is handled here)
    """
    with open(filename, "w") as f:
        # Address 0, type 0 (data)
        addr = 0
        for op in opcodes:
            # Intel hex expects byte order: Low, High
            low = op & 0xFF
            high = (op >> 8) & 0xFF
            # :02AAAA00LLHHCC
            # 02 = length
            # AAAA = address
            # 00 = record type
            payload = [0x02, (addr >> 8) & 0xFF, addr & 0xFF, 0x00, low, high]
            checksum = (0x100 - (sum(payload) & 0xFF)) & 0xFF
            f.write(f":02{addr:04X}00{low:02X}{high:02X}{checksum:02X}\n")
            addr += 2
        
        # EOF
        f.write(":00000001FF\n")

if __name__ == "__main__":
    # Test 1: LDI R16, 0x55; LDI R17, 0xAA; ADD R16, R17; BREAK
    # LDI R16, 0x55 -> 0xE505
    # LDI R17, 0xAA -> 0xEA1A
    # ADD R16, R17 -> 0x0F01
    # BREAK -> 0x9598
    create_hex("test_alu.hex", [0xE505, 0xEA1A, 0x0F01, 0x9598])
    print("Created test_alu.hex")

    # Test 2: SREG access via IN/OUT
    # LDI R16, 0x55
    # OUT 0x3F, R16 (SREG = 0x55)
    # IN R17, 0x3F  (R17 = 0x55)
    # BREAK
    # OUT 0x3F is 0xBF0F
    # IN R17, 0x3F is 0xB71F
    create_hex("test_sreg.hex", [0xE505, 0xBF0F, 0xB71F, 0x9598])
    print("Created test_sreg.hex")

    # Test 3: SP access via IN/OUT
    create_hex("test_sp.hex", [0xEA0A, 0xE015, 0xBE0D, 0xBF1E, 0xB62D, 0xB73E, 0x9598])
    print("Created test_sp.hex")

    # Test 4: EEPROM Write/Read
    # LDI R16, 0x05 (Addr Low)
    # LDI R17, 0x42 (Data)
    # OUT 0x21, R16 (EEARL)
    # OUT 0x20, R17 (EEDR)
    # SBI 0x1F, 2   (EECR.EEMPE = 1)
    # SBI 0x1F, 1   (EECR.EEPE = 1) -> Start write
    # loop: SBIC 0x1F, 1; RJMP loop  -> Wait for EEPE to clear
    # LDI R17, 0x00
    # SBI 0x1F, 0   (EECR.EERE = 1) -> Read
    # IN R18, 0x20  (R18 = EEDR)
    # BREAK
    create_hex("test_eeprom.hex", [
        0xE005, 0xE412, 0xBE01, 0xBE10, 0x9AFA, 0x9AF9, 
        0x99F9, 0xCFFF, 0xE010, 0x9AF8, 0xB620, 0x9598
    ])
    print("Created test_eeprom.hex")
