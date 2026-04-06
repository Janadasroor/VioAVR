/* test_mcusr.c - Test MCUSR reset cause flags for SimAVR vs VioAVR comparison */
#include <avr/io.h>
#include <stdint.h>

volatile uint8_t reset_flags = 0;
volatile uint8_t after_clear = 0;

int main(void) {
    /* Read MCUSR to check reset cause */
    reset_flags = MCUSR;
    
    /* Clear MCUSR flags (writing 0 clears all flags) */
    MCUSR = 0;
    after_clear = MCUSR;
    
    /* On first run after power-on, reset_flags should have PORF set */
    /* after_clear should be 0 */
    
    while (1) {
        asm volatile("nop");
    }
    
    return 0;
}
