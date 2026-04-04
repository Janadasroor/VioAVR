/* load_store.c - Test load and store instructions (LD/ST/LDS/STS) */
#include <avr/io.h>

volatile uint8_t ram_data[16] = {0};
volatile uint8_t read_result;

int main(void) {
    /* Direct store (STS) */
    ram_data[0] = 0xAB;
    ram_data[1] = 0xCD;
    
    /* Direct load (LDS) */
    read_result = ram_data[0];
    
    /* Post-increment load (LD X+) */
    uint8_t *ptr = &ram_data[0];
    uint8_t v0 = *ptr++;
    uint8_t v1 = *ptr++;
    
    /* Pre-decrement store (ST -Y) */
    uint8_t src = 0x55;
    *--ptr = src;
    
    /* Indirect with displacement (LDD Y+q) */
    uint8_t offset_val = *(ptr + 2);
    
    (void)v0; (void)v1; (void)offset_val;
    
    while (1) {}
    return 0;
}
