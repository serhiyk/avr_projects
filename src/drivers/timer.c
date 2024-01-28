#include <avr/io.h>
#include <avr/interrupt.h>
#include "config.h"
#include "timer.h"

#if !defined(TIMER_NUMBER)
    #error TIMER_NUMBER is not defined
#endif

typedef struct {
    uint8_t timeout;
    uint8_t counter;
    timer_cb callback;
} timer_descriptor_t;

static timer_descriptor_t timer_descriptor[TIMER_NUMBER];
static volatile uint8_t timer_flag=0;
#ifdef USE_LOCALTIME
static volatile uint32_t localtime_ds;
#endif

void timer_init(void)
{
    for (uint8_t i=0; i<TIMER_NUMBER; i++)
    {
        timer_descriptor[i].timeout = 0;
    }
    // 10 Hz
    OCR1A = 6249;
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS12);
    TIMSK1 |= 1 << OCIE1A;
}

SIGNAL(TIMER1_COMPA_vect)
{
    timer_flag = 1;
#ifdef USE_LOCALTIME
    localtime_ds++;
#endif
}

uint8_t timer_register(uint8_t timer, uint8_t timeout, timer_cb callback)
{
    if (timer >= TIMER_NUMBER || !callback)
    {
        return 1;
    }
    timer_descriptor[timer].counter = 0;
    timer_descriptor[timer].callback = callback;
    timer_descriptor[timer].timeout = timeout;
    return 0;
}

uint8_t timer_stop(uint8_t timer)
{
    if (timer >= TIMER_NUMBER)
    {
        return 1;
    }
    timer_descriptor[timer].counter = 0;
    timer_descriptor[timer].timeout = 0;
    return 0;
}

uint8_t timer_start(uint8_t timer, uint8_t timeout)
{
    if (timer >= TIMER_NUMBER)
    {
        return 1;
    }
    timer_descriptor[timer].counter = 0;
    timer_descriptor[timer].timeout = timeout;
    return 0;
}

void timer_handler(void)
{
    if (timer_flag)
    {
        timer_flag = 0;
        for (uint8_t i=0; i<TIMER_NUMBER; i++)
        {
            if (timer_descriptor[i].timeout == 0)
            {
                continue;
            }
            timer_descriptor[i].counter++;
            if (timer_descriptor[i].counter == timer_descriptor[i].timeout)
            {
                timer_descriptor[i].counter = 0;
                timer_descriptor[i].callback();
            }
        }
    }
}

#ifdef USE_LOCALTIME
uint32_t get_localtime_ds(void)
{
    return localtime_ds;
}
#endif
