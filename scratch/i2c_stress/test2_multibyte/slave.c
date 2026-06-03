#include <avr/io.h>

#define I2C_ADDR   0x42
#define SCL_bp     0
#define SDA_bp     1
#define SCL_mask   (1 << SCL_bp)
#define SDA_mask   (1 << SDA_bp)

static inline uint8_t scl_read(void) { return (PIND >> SCL_bp) & 1; }
static inline uint8_t sda_read(void) { return (PIND >> SDA_bp) & 1; }

static inline void sda_release(void) {
    DDRD  &= ~SDA_mask;
    PORTD |=  SDA_mask;
}
static inline void sda_drive_low(void) {
    DDRD  |=  SDA_mask;
    PORTD &= ~SDA_mask;
}

static uint8_t i2c_recv_byte(void)
{
    uint8_t data = 0;
    for (int8_t i = 7; i >= 0; i--) {
        while (scl_read() == 0);
        data |= (sda_read() << i);
        while (scl_read() != 0);
    }
    return data;
}

static void i2c_send_ack(void)
{
    sda_drive_low();
    while (scl_read() == 0);
    while (scl_read() != 0);
    sda_release();
}

static void i2c_send_nack(void)
{
    sda_release();
    while (scl_read() == 0);
    while (scl_read() != 0);
}

static uint8_t i2c_detect_start(void)
{
    uint8_t last_sda = sda_read();
    for (;;) {
        uint8_t cur_sda = sda_read();
        uint8_t cur_scl = scl_read();
        if (cur_scl && last_sda && !cur_sda) return 1;
        last_sda = cur_sda;
    }
}

static void i2c_detect_stop(void)
{
    uint8_t last_sda = sda_read();
    for (;;) {
        uint8_t cur_sda = sda_read();
        uint8_t cur_scl = scl_read();
        if (cur_scl && !last_sda && cur_sda) return;
        last_sda = cur_sda;
    }
}

int main(void)
{
    DDRB  = 0xFF;
    PORTB = 0;

    DDRD  &= ~(SCL_mask | SDA_mask);
    PORTD |=  (SCL_mask | SDA_mask);

    uint8_t checksum = 0;

    for (;;) {
        if (!i2c_detect_start()) continue;

        uint8_t addr_byte = i2c_recv_byte();
        uint8_t addr = addr_byte >> 1;
        uint8_t rw_bit = addr_byte & 1;

        if (addr != I2C_ADDR || rw_bit != 0) {
            i2c_send_nack();
            i2c_detect_stop();
            continue;
        }

        i2c_send_ack();

        checksum = 0;
        for (int i = 0; i < 16; i++) {
            uint8_t byte = i2c_recv_byte();
            checksum ^= byte;
            i2c_send_ack();
        }

        PORTB = checksum;

        i2c_detect_stop();
    }
}
