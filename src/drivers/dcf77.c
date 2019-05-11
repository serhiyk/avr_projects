#include <avr/io.h>
#include <avr/interrupt.h>
#include "dcf77.h"
#include "UART.h"

#define DCF77_PORT PORTD
#define DCF77_DDR DDRD
#define DCF77_PORT_IN PIND
#define DCF77_PIN PD3

#define DCF77_STATE_INIT 0

#define DCF77_BIT_TICKS (F_CPU/1024*150/256/1000)
#define DCF77_START_TICKS (F_CPU/1024*1050/256/1000)

typedef struct
{
    uint16_t _weather_broadcasts;
    uint8_t summer_time_announcement : 1;
    uint8_t time_zone : 2;
    uint8_t leap_second_announcement : 1;
    uint8_t start_of_encoded_time : 1;
    uint8_t minute_l : 4;
    uint8_t minute_h : 3;
    uint8_t minute_parity : 1;
    uint8_t hour_l : 4;
    uint8_t hour_h : 2;
    uint8_t hour_parity : 1;
    uint8_t day_l : 4;
    uint8_t day_h : 2;
    uint8_t dow : 3;
    uint8_t month_l : 4;
    uint8_t month_h : 1;
    uint8_t year_l : 4;
    uint8_t year_h : 4;
    uint8_t parity : 1;
} dcf77_time_t;

static volatile uint8_t _timer_flag = 0;
static volatile uint8_t _int_flag = 0;
static uint8_t _timer_couner = 0;
static dcf77_time_t _dcf77_time;
static uint8_t _dcf77_state = DCF77_STATE_INIT;
static uint8_t _dcf77_counter = 0;

static void _dcf77_bit_handler(void)
{
    uart_send_hex(_dcf77_counter);
    uart_send_byte('|');
    if (_timer_couner > DCF77_BIT_TICKS)
    {
        uint8_t *b = &((uint8_t *) &_dcf77_time)[_dcf77_counter >> 3];
        *b |= 1 << (_dcf77_counter & 0x07);
    }
    _dcf77_counter++;
    if (_dcf77_counter == 59)
    {
        for (uint8_t i=0; i<sizeof(_dcf77_time); i++)
        {
            uart_send_hex(((uint8_t *) &_dcf77_time)[i]);
        }
        uart_send_byte(' ');
        _dcf77_counter = 0;
    }
}

void dcf77_init(void)
{
    // timer2 init
    TCCR2A = 0;  // Normal mode
    TCCR2B = (1 << CS20) | (1 << CS21) | (1 << CS22);  // clk / 1024
    TIMSK2 = 1 << TOIE2;  // Overflow Interrupt Enable

    // external interrupt init
    EICRA |= 1 << ISC10;  // Any logical change
    EIMSK |= 1 << INT1;
}

void dcf77_handler(void)
{
    if (_timer_flag)
    {
        _timer_flag = 0;
        _timer_couner++;
        if (_timer_couner == DCF77_START_TICKS)
        {
            for (uint8_t i=0; i<sizeof(_dcf77_time); i++)
            {
                ((uint8_t *) &_dcf77_time)[i] = 0;
            }
            _timer_couner = 0;
            uart_send_byte('B');
        }
    }
    if (_int_flag)
    {
        _int_flag = 0;
        if (DCF77_PORT_IN & (1 << DCF77_PIN))
        {
            _dcf77_bit_handler();
        }
        else
        {
            _timer_couner = 0;
        }
    }
}

SIGNAL(TIMER2_OVF_vect)
{
    _timer_flag = 1;
}

SIGNAL(INT1_vect)
{
    _int_flag = 1;
}
