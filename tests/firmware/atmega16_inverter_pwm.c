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
    TCCR1A = (2 << COM1A0) | (3 << COM1B0) | (1 << WGM11);
    ICR1    = TOP_VAL;
    OCR1A   = HALF_TOP - DEAD_TIME;
    OCR1B   = HALF_TOP + DEAD_TIME;
    TCCR1B  = (1 << WGM13) | (1 << WGM12) | (1 << CS10);

    DDRD   |= (1 << PD5) | (1 << PD4);
}

static void set_duty(uint16_t percent)
{
    if (percent > 100) percent = 100;
    uint16_t pw = (uint32_t)percent * (HALF_TOP - DEAD_TIME) / 100;
    OCR1A = pw;
}

static void init_adc(void)
{
    ADMUX  = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

static uint16_t read_adc(uint8_t channel)
{
    ADMUX = (1 << REFS0) | (channel & 0x07);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

static void init_timer_led(void)
{
    TCCR0 = (1 << WGM01) | (1 << CS02) | (1 << CS00);
    OCR0 = 255;
    TIMSK |= (1 << OCIE0);
}

static volatile uint16_t led_timer_count = 0;

ISR(TIMER0_COMP_vect)
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
            TCCR1B &= ~7;
            PORTB |= (1 << LED_PIN);
            _delay_ms(1000);
            current = read_adc(CURRENT_ADC_CH);
            if (current <= CURRENT_LIMIT) {
                TCCR1B |= (1 << CS10);
                PORTB &= ~(1 << LED_PIN);
            }
        } else {
            duty = (uint32_t)voltage * 95 / 1023 + 5;
            set_duty(duty);
        }

        _delay_ms(100);
    }
}
