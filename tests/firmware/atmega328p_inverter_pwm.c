#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

#define TOP_VAL     799
#define DEAD_TIME   8
#define HALF_TOP    400

#define LED_PIN     PB0

#define VOLTAGE_ADC_CH  0
#define CURRENT_ADC_CH  1
#define CURRENT_LIMIT   900

static void init_pwm(void)
{
    // Fast PWM Mode 14: ICR1=TOP, COM1A=2 (non-inverting), COM1B=3 (inverting)
    TCCR1A = (2 << COM1A0) | (3 << COM1B0) | (1 << WGM11);  // 0xB2
    ICR1    = TOP_VAL;
    OCR1A   = HALF_TOP - DEAD_TIME;  // OutA end
    OCR1B   = HALF_TOP + DEAD_TIME;  // OutB start
    TCCR1B  = (1 << WGM13) | (1 << WGM12) | (1 << CS10);   // 0x19

    DDRB   |= (1 << PB1) | (1 << PB2);   // OC1A=PB1, OC1B=PB2
}

static void set_duty(uint16_t percent)
{
    if (percent > 100) percent = 100;
    uint16_t pw = (uint32_t)percent * (HALF_TOP - DEAD_TIME) / 100;
    OCR1A = pw;
}

static void init_adc(void)
{
    ADMUX  = (1 << 6);                  // REFS0=1: AVCC
    ADCSRA = (1 << 7)                   // ADEN
           | (1 << 2) | (1 << 1) | (1 << 0);  // prescaler /128
}

static uint16_t read_adc(uint8_t channel)
{
    ADMUX = (1 << 6) | (channel & 0x0F);
    ADCSRA |= (1 << 6);                 // ADSC
    while (ADCSRA & (1 << 6));
    return ADC;
}

static void init_timer_led(void)
{
    TCCR0A = (1 << 1);                  // CTC
    TCCR0B = (1 << 2) | (1 << 0);      // prescaler /1024
    OCR0A  = 255;
    TIMSK0 = (1 << 1);                  // OCIE0A
}

static volatile uint16_t led_timer_count = 0;

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

    init_pwm();
    init_adc();
    init_timer_led();

    sei();

    for (;;) {
        voltage = read_adc(VOLTAGE_ADC_CH);
        current = read_adc(CURRENT_ADC_CH);

        if (current > CURRENT_LIMIT) {
            TCCR1B &= ~7;               // stop timer (CS=0)
            PORTB |= (1 << LED_PIN);
            _delay_ms(1000);
            current = read_adc(CURRENT_ADC_CH);
            if (current <= CURRENT_LIMIT) {
                TCCR1B |= (1 << CS10);  // restart timer
                PORTB &= ~(1 << LED_PIN);
            }
        } else {
            duty = (uint32_t)voltage * 95 / 1023 + 5;
            set_duty(duty);
        }

        _delay_ms(100);
    }
}
