#include <avr/io.h>
#include <avr/interrupt.h>
#include "config.h"
#include "onewire.h"

#if !defined(ONEWIRE_PORT)
    #error ONEWIRE_PORT is not defined
#endif

#if !defined(ONEWIRE_DDR)
    #error ONEWIRE_DDR is not defined
#endif

#if !defined(ONEWIRE_PORT_IN)
    #error ONEWIRE_PORT_IN is not defined
#endif

#if !defined(ONEWIRE_PIN)
    #error ONEWIRE_PIN is not defined
#endif

#define HIGH_PULL_UP_PERIOD (85*(F_CPU/1000000UL)/8)
#define HIGH_PULL_DOWN_PERIOD (10*(F_CPU/1000000UL)/8)
#define LOW_PULL_UP_PERIOD (15*(F_CPU/1000000UL)/8)
#define LOW_PULL_DOWN_PERIOD (80*(F_CPU/1000000UL)/8)
#define READ_PERIOD (10*(F_CPU/1000000UL)/8)

#define RESET_WRITE_PERIOD (500*(F_CPU/1000000UL)/64)
#define RESET_READ_PERIOD (50*(F_CPU/1000000UL)/64)

static volatile uint8_t onewire_data;
static volatile uint8_t onewire_counter;
static onewire_cb onewire_callback;

void onewire_init(void)
{
    ONEWIRE_PORT &= ~(1 << ONEWIRE_PIN);
    ONEWIRE_DDR &= ~(1 << ONEWIRE_PIN);
    TCCR0A = 1 << WGM01; // CTC mode
    TCCR0B = 0;
    TIMSK0 = 0;
}

void onewire_master_pull_down(onewire_cb callback)
{
    ONEWIRE_DDR |= 1 << ONEWIRE_PIN;
    ONEWIRE_PORT &= ~(1 << ONEWIRE_PIN);
    onewire_counter = 0;
    onewire_callback = callback;
    TCNT0 = 0;
    OCR0A = RESET_WRITE_PERIOD;
    TIMSK0 = (1 << OCIE0B) | (1 << OCIE0A);
    TCCR0B = (1 << CS01) | (1 << CS00); // clk/64
}

void onewire_master_pull_up(onewire_cb callback)
{
    ONEWIRE_DDR &= ~(1 << ONEWIRE_PIN);
    ONEWIRE_PORT &= ~(1 << ONEWIRE_PIN);
    onewire_counter = 0;
    onewire_data = 0;
    onewire_callback = callback;
    TCNT0 = 0;
    OCR0A = RESET_WRITE_PERIOD;
    OCR0B = RESET_READ_PERIOD;
    TIMSK0 = (1 << OCIE0B) | (1 << OCIE0A);
    TCCR0B = (1 << CS01) | (1 << CS00); // clk/64
}

void onewire_master_write(uint8_t data, onewire_cb callback)
{
    ONEWIRE_DDR &= ~(1 << ONEWIRE_PIN);
    ONEWIRE_PORT &= ~(1 << ONEWIRE_PIN);
    onewire_counter = 8;
    onewire_data = data;
    onewire_callback = callback;
    TCNT0 = 0;
    OCR0A = HIGH_PULL_DOWN_PERIOD;
    TIMSK0 = 1 << OCIE0A;
    TCCR0B = 1 << CS01; // clk/8
}

void onewire_master_read(onewire_cb callback)
{
    ONEWIRE_DDR &= ~(1 << ONEWIRE_PIN);
    ONEWIRE_PORT &= ~(1 << ONEWIRE_PIN);
    onewire_counter = 8;
    onewire_data = 0xFF;
    onewire_callback = callback;
    TCNT0 = 0;
    OCR0A = HIGH_PULL_DOWN_PERIOD;
    OCR0B = READ_PERIOD;
    TIMSK0 = (1 << OCIE0B) | (1 << OCIE0A);
    TCCR0B = 1 << CS01; // clk/8
}

SIGNAL(TIMER0_COMPA_vect)
{
    if (onewire_counter == 0)
    {
        TCCR0B = 0;
        TIMSK0 = 0;
        if (onewire_callback)
        {
            onewire_callback(onewire_data);
        }
        return;
    }
    if (ONEWIRE_DDR & (1 << ONEWIRE_PIN))
    {
        ONEWIRE_DDR &= ~(1 << ONEWIRE_PIN);
        if (1 & onewire_data)
        {
            OCR0A = HIGH_PULL_UP_PERIOD;
        }
        else
        {
            OCR0A = LOW_PULL_UP_PERIOD;
        }
        onewire_counter--;
        onewire_data >>= 1;
    }
    else
    {
        ONEWIRE_DDR |= 1 << ONEWIRE_PIN;
        if (1 & onewire_data)
        {
            OCR0A = HIGH_PULL_DOWN_PERIOD;
        }
        else
        {
            OCR0A = LOW_PULL_DOWN_PERIOD;
        }
    }
}

SIGNAL(TIMER0_COMPB_vect)
{
    if (ONEWIRE_PORT_IN & (1 << ONEWIRE_PIN))
    {
        onewire_data |= 0x80;
    }
}
