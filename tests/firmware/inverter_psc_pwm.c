#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define TOP_VAL    800
#define DEAD_TIME  8
#define HALF_TOP   (TOP_VAL / 2)

#define LED_PIN    PB3

#define VOLTAGE_ADC_CH  0
#define CURRENT_ADC_CH  1
#define CURRENT_LIMIT   900

volatile uint16_t led_timer_count = 0;

static void init_psc(void)
{
    PSOC0 = (1 << 0) | (1 << 2);    // POENA | POENB

    PFRC0A = (1 << 2);              // PISEL=1: level-triggered fault

    PCNF0 = 0x00;                   // One-Ramp mode, system clock

    OCR0RBH = (TOP_VAL >> 8);
    OCR0RBL = (TOP_VAL & 0xFF);

    OCR0SAH = 0; OCR0SAL = 0;
    OCR0RAH = (HALF_TOP - DEAD_TIME) >> 8;
    OCR0RAL = (HALF_TOP - DEAD_TIME) & 0xFF;
    OCR0SBH = (HALF_TOP + DEAD_TIME) >> 8;
    OCR0SBL = (HALF_TOP + DEAD_TIME) & 0xFF;

    PCTL0 = (1 << 0);               // PRUN=1, prescaler /1
}

static void stop_psc(void)
{
    PCTL0 &= ~(1 << 0);
}

static void start_psc(void)
{
    PCTL0 |= (1 << 0);
}

static void set_duty(uint16_t percent)
{
    uint16_t pulse_width, start_a;

    if (percent > 100) percent = 100;

    pulse_width = ((uint32_t)percent * (HALF_TOP - 2 * DEAD_TIME)) / 100;

    start_a = (pulse_width < (uint16_t)(HALF_TOP - DEAD_TIME))
              ? (HALF_TOP - DEAD_TIME - pulse_width) : 0;

    OCR0SAH = start_a >> 8; OCR0SAL = start_a & 0xFF;
    // OCR0RA  = HALF_TOP - DEAD_TIME  (fixed — dead-time boundary)
    // OCR0SB  = HALF_TOP + DEAD_TIME  (fixed — dead-time boundary)
    // OCR0RB  = TOP_VAL               (fixed — PWM period)
}

static void init_adc(void)
{
    ADMUX = (1 << 6);               // REFS0=1: AVCC reference
    ADCSRA = (1 << 7)               // ADEN
           | (1 << 2) | (1 << 1) | (1 << 0);
}

static uint16_t read_adc(uint8_t channel)
{
    ADMUX = (1 << 6) | (channel & 0x0F);
    ADCSRA |= (1 << 6);             // ADSC: start conversion
    while (ADCSRA & (1 << 6));
    return ADC;
}

static void init_timer_led(void)
{
    TCCR0A = (1 << 1);              // WGM01=1: CTC mode
    TCCR0B = (1 << 2) | (1 << 0);  // CS02|CS00: prescaler /1024
    OCR0A = 255;                    // ~16ms period
    TIMSK0 = (1 << 1);              // OCIE0A
}

ISR(TIMER0_COMP_A_vect)
{
    if (++led_timer_count >= 122) {
        led_timer_count = 0;
        PORTB ^= (1 << LED_PIN);
    }
}

int main(void)
{
    uint16_t voltage, current;
    uint16_t duty;

    DDRB |= (1 << LED_PIN);
    PORTB &= ~(1 << LED_PIN);

    init_psc();
    init_adc();
    init_timer_led();

    sei();

    for (;;) {
        voltage = read_adc(VOLTAGE_ADC_CH);
        current = read_adc(CURRENT_ADC_CH);

        if (current > CURRENT_LIMIT) {
            stop_psc();
            PORTB |= (1 << LED_PIN);
            _delay_ms(1000);
            current = read_adc(CURRENT_ADC_CH);
            if (current <= CURRENT_LIMIT) {
                start_psc();
                PORTB &= ~(1 << LED_PIN);
            }
        } else {
            duty = (uint32_t)voltage * 95 / 1023 + 5;
            set_duty(duty);
        }

        _delay_ms(100);
    }
}
