#include <avr/io.h>
#include <avr/interrupt.h>
#include "time_async.h"

static time_async_cb time_update_cb;

static uint8_t days_in_month[13] = {0x00, 0x32, 0x29,0x32, 0x31, 0x32, 0x31, 0x32, 0x32, 0x31, 0x32, 0x31, 0x32};

static uint8_t curr_second;
static uint8_t curr_minute;
static uint8_t curr_hour;
static uint8_t curr_dow;
static uint8_t curr_date;
static uint8_t curr_month;
static uint16_t curr_year;

void time_async_init(time_async_cb cb)
{
    time_update_cb = cb;
    // 1 Hz
    ASSR = (1 << AS2);
    TCNT2 = 0;
    OCR2 = 32;
    TCCR2 = (1 << WGM21) | (1 << CS20) | (1 << CS21) | (1 << CS22);
    while (ASSR & ((1 << TCN2UB) | (1 << OCR2UB) | (1 << TCR2UB)));
    TIFR = ((0 << OCF2) | (0 << TOV2));
    TIMSK |= 1 << OCIE2;
}

void time_async_set(
    uint8_t second,
    uint8_t minute,
    uint8_t hour,
    uint8_t dow,
    uint8_t date,
    uint8_t month,
    uint16_t year)
{
    curr_second = second;
    curr_minute = minute;
    curr_hour = hour;
    curr_dow = dow;
    curr_date = date;
    curr_month = month;
    curr_year = year;
}

void time_async_get_hms(uint8_t *h, uint8_t *m, uint8_t *s)
{
    *h = curr_hour;
    *m = curr_minute;
    *s = curr_second;
}

SIGNAL(TIMER2_COMP_vect)
{
    curr_second++;
    if (curr_second == 60)
    {
        curr_second = 0;
        curr_minute++;
        if (curr_minute == 60)
        {
            curr_minute = 0;
            curr_hour++;
            if (curr_hour == 24)
            {
                curr_hour = 0;
                curr_date++;
                if (curr_date >= days_in_month[curr_month])
                {
                    if (!(curr_month == 2 &&
                        (curr_year & 3) == 0 &&
                        curr_date == 29))
                    {
                        curr_date = 1;
                        curr_month++;
                        if (curr_month == 13)
                        {
                            curr_month = 1;
                            curr_year++;
                        }
                    }
                }
            }
        }
    }

    time_update_cb();
}
