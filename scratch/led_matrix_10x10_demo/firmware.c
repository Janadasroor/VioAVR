#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

int main(void) {
    // Rows: PB0-PB7 (8) + PC0-PC1 (2) = 10 outputs
    DDRB  = 0xFF;
    DDRC  = 0x0F;  // PC0-PC3 as outputs (PC4-PC5 unused)
    PORTB = 0x00;
    PORTC = 0x00;

    // Columns: PD0-PD7 (8) + PC2-PC3 (2) = 10 outputs
    DDRD  = 0xFF;
    PORTD = 0xFF;  // all columns HIGH = off
    PORTC |= 0x0C; // PC2,PC3 HIGH = off

    while (1) {
        // Scan all row × column intersections
        for (int row = 0; row < 10; row++) {
            // Drive row HIGH
            if (row < 8)
                PORTB |= (1 << row);
            else
                PORTC |= (1 << (row - 8));

            for (int col = 0; col < 10; col++) {
                // Drive column LOW (active)
                if (col < 8)
                    PORTD &= ~(1 << col);
                else
                    PORTC &= ~(1 << (col - 8 + 2)); // PC2, PC3

                _delay_us(50);

                // Restore column HIGH (inactive)
                if (col < 8)
                    PORTD |= (1 << col);
                else
                    PORTC |= (1 << (col - 8 + 2));
            }

            // Drive row LOW
            if (row < 8)
                PORTB &= ~(1 << row);
            else
                PORTC &= ~(1 << (row - 8));
        }
    }
    return 0;
}
