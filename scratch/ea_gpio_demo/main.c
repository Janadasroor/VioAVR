#define PORTA_DIR  (*(volatile unsigned char*)0x400)
#define PORTA_OUT  (*(volatile unsigned char*)0x404)

void delay(void) {
    __asm__ volatile (
        "    ldi r18, 0\n"
        "1:  subi r18, 1\n"
        "    brne 1b\n"
        "    ldi r18, 0\n"
        "2:  subi r18, 1\n"
        "    brne 2b\n"
        : : : "r18", "cc"
    );
}

int main(void) {
    PORTA_DIR |= 0x01;
    while (1) {
        PORTA_OUT ^= 0x01;
        delay();
    }
}
