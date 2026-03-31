#include <avr/io.h>
#include <avr/eeprom.h>

int main(void) {
    uint8_t data = 0;
    uint8_t read_data;
    
    DDRB = 0xFF; // Output for reading data back

    while (1) {
        // Write data to address 0x05
        eeprom_write_byte((uint8_t*)0x05, data);

        // Read it back
        read_data = eeprom_read_byte((uint8_t*)0x05);

        // Display to PORTB
        PORTB = read_data;

        data++;
        
        // Wait for EEPROM to be ready before next cycle (normally write takes time)
        // eeprom_write_byte already waits for ready.
    }
}
