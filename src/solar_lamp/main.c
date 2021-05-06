#include <stdint.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include "timer.h"
#include "UART.h"
#include "serial.h"

// #define LED_CONTROL_PORT PORTB
// #define LED_CONTROL_DDR DDRB
// #define LED_CONTROL_PIN PB5
#define LED_CONTROL_PORT PORTC
#define LED_CONTROL_DDR DDRC
#define LED_CONTROL_PIN PC3
#define LED_ON() (LED_CONTROL_PORT |= 1 << LED_CONTROL_PIN)
#define LED_OFF() (LED_CONTROL_PORT &= ~(1 << LED_CONTROL_PIN))

#define SECOND_TIMER_ID 0
#define SECOND_TIMER_PERIOD 10  // 1 second

#define VOLTAGE (36)  // 3.6V
#define DUSK_VOLTAGE (12)  // 1.2V
#define DAWN_VOLTAGE (40)  // 4.0V
#define LAMP_ON_TIMEOUT (2*60*60)

#define ADC_PIN 1

typedef enum
{
    STATE_INIT = 0,
    STATE_LAMP_ON,
    STATE_LAMP_OFF
} state_enum_t;

static uint8_t state = STATE_INIT;
static uint16_t lamp_on_timer;

uint8_t adc_read(void)
{
    // starts the conversion
    ADCSRA |= 1 << ADSC;

    while ((ADCSRA & (1 << ADSC)));

    uint32_t vin = ADCW;
    vin *= VOLTAGE;
    vin /= (1024 / 2);
    return vin;
}

void state_timer(void)
{
    uint8_t vin = adc_read();
    uart_send_byte('0' + vin / 10);
    uart_send_byte('.');
    uart_send_byte('0' + vin % 10);
    uart_send_byte('-');
    uart_send_byte('0' + state);
    uart_send_byte('|');

    switch (state)
    {
        case STATE_INIT:
            if (vin < DUSK_VOLTAGE)
            {
                state = STATE_LAMP_ON;
                lamp_on_timer = 0;
                LED_ON();
            }
            break;
        case STATE_LAMP_ON:
            lamp_on_timer++;
            if (lamp_on_timer == LAMP_ON_TIMEOUT)
            {
                state = STATE_LAMP_OFF;
                LED_OFF();
            }
        case STATE_LAMP_OFF:
            if (vin > DAWN_VOLTAGE)
            {
                state = STATE_INIT;
                LED_OFF();
            }
            break;
    }
}

void adc_init(void)
{
    // AREF = AVcc
    ADMUX = ADC_PIN + (1 << REFS0);
    DIDR0 = (1 << ADC0D);
    ADCSRA = (1 << ADEN);
}

void setup(void)
{
    LED_CONTROL_DDR |= 1 << LED_CONTROL_PIN;
    LED_OFF();
    asm("sei");
    timer_init();
    serial_init();
    adc_init();
    //_delay_ms(5);
}

int main(void)
{
    setup();
    uart_send_byte('S');
    timer_register(SECOND_TIMER_ID, SECOND_TIMER_PERIOD, state_timer);
    set_sleep_mode(SLEEP_MODE_IDLE);
    while(1)
    {
        serial_handler();
        timer_handler();
        sleep_enable();
        sleep_cpu();
    }
}
