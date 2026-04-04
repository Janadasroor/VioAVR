/*
 * alu_bare.c - Minimal ALU test with direct assembly for precise opcode control
 * 
 * Uses inline assembly to guarantee specific opcodes are generated.
 * This avoids compiler optimization issues.
 */

void __attribute__((naked)) main(void) {
    __asm__ volatile (
        // Section 0: Addition (PC=0-4)
        "ldi r16, 0x0F\n\t"      // PC=0: r16 = 0x0F
        "ldi r17, 0x01\n\t"      // PC=1: r17 = 0x01
        "add r16, r17\n\t"       // PC=2: r16 = 0x10 (with half-carry)
        "mov r18, r16\n\t"       // PC=3: r18 = 0x10
        "sub r18, r17\n\t"       // PC=4: r18 = 0x0F
        
        // Section 1: Multiplication (PC=5-11)
        "mul r16, r17\n\t"       // PC=5: r1:r0 = 0x10 * 0x01 = 0x0010
        "ldi r20, 0xFE\n\t"      // PC=6: r20 = -2 (signed)
        "ldi r21, 0x02\n\t"      // PC=7: r21 = 2
        "muls r20, r21\n\t"      // PC=8: r1:r0 = -2 * 2 = -4 (0xFFFC)
        "ldi r22, 0xFE\n\t"      // PC=9: r22 = -2 (signed)
        "ldi r23, 0x03\n\t"      // PC=10: r23 = 3
        "mulsu r22, r23\n\t"     // PC=11: r1:r0 = -2 * 3 = -6 (0xFFFA)
        
        // Section 2: Immediate operations (PC=12-17)
        "ldi r18, 0x0F\n\t"      // PC=12: Reset r18
        "subi r18, 0x0F\n\t"     // PC=13: r18 = 0x00, Z=1
        "ldi r21, 0x00\n\t"      // PC=14: r21 = 0
        "subi r21, 0x01\n\t"     // PC=15: r21 = 0xFF, C=1
        "ldi r22, 0x01\n\t"      // PC=16: r22 = 1
        "ldi r23, 0x01\n\t"      // PC=17: r23 = 1
        
        // Section 3: ADC with carry (PC=18-19)
        "sec\n\t"                 // Set carry flag
        "adc r22, r23\n\t"       // PC=18: r22 = 1 + 1 + 1 = 3
        "ldi r20, 0x7F\n\t"      // PC=19: r20 = 0x7F
        
        // Section 4: Overflow test (PC=20-21)
        "ldi r17, 0x01\n\t"      // r17 = 1
        "add r20, r17\n\t"       // PC=20: r20 = 0x80, V=1
        "ldi r24, 0xAA\n\t"      // PC=21: r24 = 0xAA
        
        // Section 5: Logic operations (PC=22-24)
        "and r24, r22\n\t"       // PC=22: r24 &= 0x03
        "or r24, r21\n\t"        // PC=23: r24 |= 0xFF
        "eor r24, r24\n\t"       // PC=24: r24 = 0, Z=1
        
        // Section 6: Branch tests (PC=25-27)
        "breq 1f\n\t"            // PC=25: Branch taken (Z=1)
        "ldi r25, 0x77\n\t"      // PC=26: Skipped
        "1: brne 2f\n\t"         // PC=27: Branch not taken (Z=1)
        "ldi r26, 0x55\n\t"      // PC=28: Executed
        "2:\n\t"
        
        // Section 7: More branches (PC=29-38)
        "or r24, r22\n\t"        // PC=29: r24 = 0
        "brne 3f\n\t"            // PC=30: Not taken (Z=1)
        "ldi r27, 0x66\n\t"      // PC=31: Executed
        "3: rjmp 4f\n\t"         // PC=32: Jump forward
        "ldi r28, 0xF0\n\t"      // PC=33: Skipped
        "4: andi r28, 0x0F\n\t"  // PC=34: r28 = 0
        "ori r28, 0x05\n\t"      // PC=35: r28 = 5
        "cpi r28, 0x06\n\t"      // PC=36: Compare 5-6, C=1
        "brcs 5f\n\t"            // PC=37: Branch taken (C=1)
        "ldi r29, 0x11\n\t"      // PC=38: Skipped
        "5: ldi r30, 0x80\n\t"   // PC=39: Executed
        
        // Section 8: Negative test (PC=40-42)
        "ori r30, 0x00\n\t"      // PC=40: No change, N=1
        "brmi 6f\n\t"            // PC=41: Branch taken (N=1)
        "ldi r31, 0x22\n\t"      // PC=42: Skipped
        "6: brpl 7f\n\t"         // PC=43: Not taken (N=1)
        "mov r31, r22\n\t"       // PC=44: r31 = r22
        "7: brcc 8f\n\t"         // PC=45: Branch not taken (C=0)
        "ldi r29, 0x33\n\t"      // PC=46: Executed
        "8:\n\t"
        
        // Halt: infinite loop
        "9: rjmp 9b\n\t"
    );
}
