#include <avr/io.h>

int main(void) {
    // Set OC0A (PD6) as output
    DDRD |= (1 << DDD6);

    // Set Timer0 to CTC mode (WGM01 = 1, WGM00 = 0)
    // Toggle OC0A on compare match (COM0A0 = 1, COM0A1 = 0)
    TCCR0A = (1 << COM0A0) | (1 << WGM01);

    // OCR0A = 100 (Toggle every 101 cycles with CS00=1)
    OCR0A = 100;

    // Prescaler 1 (CS00 = 1)
    TCCR0B = (1 << CS00);

    while (1) {
        // Just loop
    }
}
