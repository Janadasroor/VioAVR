/*
 * Multi-MCU Co-Simulation: Slave Firmware
 *
 * ATmega328P implements a bit-banged I2C slave on PD0(SCL)/PD1(SDA).
 * When the master writes a byte to address 0x42, the slave outputs
 * that byte to PORTB (driving the external R-2R ladder DAC).
 *
 * I2C pins: SCL=PD0 (ext ID 24), SDA=PD1 (ext ID 25)
 * DAC port: PORTB (ext IDs 8-15)
 *
 * Build: avr-gcc -mmcu=atmega328p -Os -DF_CPU=16000000UL -o slave.elf slave.c
 *
 * 2026-06-03
 */

#include <avr/io.h>
#include <util/delay.h>

#define I2C_ADDR   0x42

/* --- I2C pin helpers (PD0=SCL, PD1=SDA) --- */
#define SCL_bp     0
#define SDA_bp     1
#define SCL_mask   (1 << SCL_bp)
#define SDA_mask   (1 << SDA_bp)

static inline uint8_t scl_read(void)  { return (PIND >> SCL_bp) & 1; }
static inline uint8_t sda_read(void)  { return (PIND >> SDA_bp) & 1; }

static inline void sda_release(void) {
    DDRD  &= ~SDA_mask;       /* Input = high-Z, pull-up holds high */
    PORTD |=  SDA_mask;
}

static inline void sda_drive_low(void) {
    DDRD  |=  SDA_mask;       /* Output, drive low */
    PORTD &= ~SDA_mask;
}

/* --- Bit-banged I2C slave receive --- */
/*
 * Receives one byte from the I2C bus after address match.
 * The master generates SCL; we sample SDA on SCL rising edges.
 * Returns the received byte.
 */
static uint8_t i2c_recv_byte(void)
{
    uint8_t data = 0;
    for (int8_t i = 7; i >= 0; i--) {
        /* Wait for SCL rising edge */
        while (scl_read() == 0);             /* Wait for SCL high */
        data |= (sda_read() << i);           /* Sample SDA */
        while (scl_read() != 0);             /* Wait for SCL low */
    }
    return data;
}

/* Send ACK (drive SDA low during ACK clock) */
static void i2c_send_ack(void)
{
    sda_drive_low();                         /* Drive SDA low */
    /* Wait for master to release SCL (rising edge) */
    while (scl_read() == 0);
    while (scl_read() != 0);                 /* Wait for SCL low */
    sda_release();                           /* Release SDA */
}

/* Send NACK (keep SDA high during ACK clock) */
static void i2c_send_nack(void)
{
    sda_release();
    while (scl_read() == 0);
    while (scl_read() != 0);
}

/* Detect START condition: SDA falling while SCL high */
static uint8_t i2c_detect_start(void)
{
    /* Wait for any SDA edge or SCL change */
    uint8_t last_sda = sda_read();

    for (;;) {
        uint8_t cur_sda = sda_read();
        uint8_t cur_scl = scl_read();

        if (cur_scl && last_sda && !cur_sda) {
            return 1;  /* START: SDA↓ while SCL high */
        }
        /* STOP would be SDA↑ while SCL high — not expected for slave RX only */
        last_sda = cur_sda;
    }
}

/* Detect STOP condition: SDA rising while SCL high */
static uint8_t i2c_detect_stop(void)
{
    uint8_t last_sda = sda_read();

    for (;;) {
        uint8_t cur_sda = sda_read();
        uint8_t cur_scl = scl_read();

        if (cur_scl && !last_sda && cur_sda) {
            return 1;  /* STOP: SDA↑ while SCL high */
        }
        last_sda = cur_sda;
    }
}

int main(void)
{
    /* PORTB = all output (R-2R ladder DAC) */
    DDRB  = 0xFF;
    PORTB = 0;

    /* PD0(SCL), PD1(SDA) = inputs with pull-up */
    DDRD  &= ~(SCL_mask | SDA_mask);
    PORTD |=  (SCL_mask | SDA_mask);

    uint8_t received_byte = 0;

    for (;;) {
        /* Wait for a START condition on the I2C bus */
        if (!i2c_detect_start()) continue;

        /* Receive address byte (8 bits) */
        uint8_t addr_byte = i2c_recv_byte();

        /* I2C address is top 7 bits; LSB = R/W (0 = write) */
        uint8_t addr    = addr_byte >> 1;
        uint8_t rw_bit  = addr_byte & 1;

        /* Check if this is a write to our address */
        if (addr != I2C_ADDR || rw_bit != 0) {
            /* Not for us — NACK and wait for STOP */
            i2c_send_nack();
            i2c_detect_stop();
            continue;
        }

        /* ACK the address */
        i2c_send_ack();

        /* Receive data byte */
        received_byte = i2c_recv_byte();

        /* ACK the data */
        i2c_send_ack();

        /* Output received value to R-2R DAC on PORTB */
        PORTB = received_byte;

        /* Wait for STOP condition */
        i2c_detect_stop();
    }
}
