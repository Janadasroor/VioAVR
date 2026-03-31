#ifndef F_CPU
#define F_CPU 16000000UL // Tell the compiler the CPU runs at 16MHz
#endif

#include <avr/io.h>      // Contains register definitions (DDRB, PORTB, etc.)
#include <util/delay.h>  // Contains the _delay_ms() function

int main(void) {
    // 1. Set PB5 as an OUTPUT
    // DDRB is the Data Direction Register for Port B.
    // Logic 1 = Output, Logic 0 = Input.
    DDRB |= (1 << DDB5);

    while (1) {
        // 2. Toggle the LED
        // PINB is the Input Pins Address.
        // Writing a '1' to a bit in PINB toggles the corresponding bit in PORTB.
        PINB |= (1 << PINB5);

        // 3. Wait for 500 milliseconds
        _delay_ms(500);
    }

    return 0;
}
