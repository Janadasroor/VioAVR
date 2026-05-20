#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

int main(void) {
    // Configure pins as outputs
    // PA0 = COM0, PA4 = SEG0
    DDRA = (1 << DDA0) | (1 << DDA4);
    // PC5 = SEG11
    DDRC = (1 << DDC5);
    // PD7 = SEG19
    DDRD = (1 << DDD7);

    while (1) {
        // --- PATTERN 1: SEG0 and SEG11 active (ON), SEG19 inactive (OFF) ---
        // We repeat this for 2 cycles (1.0ms total)
        for (int i = 0; i < 2; i++) {
            // Phase 1 (250us): COM0=High (PA0=1), SEG0=Low (PA4=0), SEG11=Low (PC5=0), SEG19=High (PD7=1)
            PORTA = (1 << PORTA0);
            PORTC = 0x00;
            PORTD = (1 << PORTD7);
            _delay_us(250);

            // Phase 2 (250us): COM0=Low (PA0=0), SEG0=High (PA4=1), SEG11=High (PC5=1), SEG19=Low (PD7=0)
            PORTA = (1 << PORTA4);
            PORTC = (1 << PORTC5);
            PORTD = 0x00;
            _delay_us(250);
        }

        // --- PATTERN 2: SEG19 active (ON), SEG0 and SEG11 inactive (OFF) ---
        // We repeat this for 2 cycles (1.0ms total)
        for (int i = 0; i < 2; i++) {
            // Phase 1 (250us): COM0=High (PA0=1), SEG0=High (PA4=1), SEG11=High (PC5=1), SEG19=Low (PD7=0)
            PORTA = (1 << PORTA0) | (1 << PORTA4);
            PORTC = (1 << PORTC5);
            PORTD = 0x00;
            _delay_us(250);

            // Phase 2 (250us): COM0=Low (PA0=0), SEG0=Low (PA4=0), SEG11=Low (PC5=0), SEG19=High (PD7=1)
            PORTA = 0x00;
            PORTC = 0x00;
            PORTD = (1 << PORTD7);
            _delay_us(250);
        }
    }

    return 0;
}

