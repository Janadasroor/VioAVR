#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

int main(void) {
    // 1. Configure all pins of PORTD as digital outputs
    DDRD = 0xFF;
    PORTD = 0x00;

    // 2. Configure ADC
    // - REFS0 = 1: Use AVcc as reference voltage (5V)
    // - ADLAR = 1: Left-adjust result (puts the 8 MSBs directly into the ADCH register)
    // - MUX3:0 = 0000: Select ADC Channel 0 (Pin PC0)
    ADMUX = (1 << REFS0) | (1 << ADLAR);

    // - ADEN = 1: Enable ADC
    // - ADPS2=1, ADPS1=1, ADPS0=0: Prescaler = 64 (16 MHz / 64 = 250 kHz ADC clock)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);

    // Continuous conversion loop
    while (1) {
        // Start conversion
        ADCSRA |= (1 << ADSC);

        // Busy-wait until conversion is complete (ADSC cleared by hardware)
        while (ADCSRA & (1 << ADSC));

        // Read the 8-bit MSB conversion result from ADCH and output to PORTD
        PORTD = ADCH;

        // Small delay between conversions to keep simulation highly stable
        _delay_us(10);
    }

    return 0;
}
