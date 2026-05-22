#include <avr/io.h>
#include <stdint.h>

int main(void)
{
    TWBR = 72;
    TWSR = 0;
    TWCR = (1 << TWEN);

    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

    for (;;) { }
}
