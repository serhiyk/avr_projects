#include <stdint.h>
#include <stdlib.h>
#include "UART.h"
#include "neo7m.h"

#define TIME_ZONE 2

#define PACKET_MAX_LEN 64
#define PACKET_MIN_LEN 24
#define START_CODE '$'
#define END_CODE '*'

static uint8_t _buf[PACKET_MAX_LEN];
static uint8_t _index;
static time_cb _time_cb = NULL;

static uint8_t days_in_month[13] = {0x00, 0x32, 0x29,0x32, 0x31, 0x32, 0x31, 0x32, 0x32, 0x31, 0x32, 0x31, 0x32};

uint8_t second_bcd_l;
uint8_t second_bcd_h;
uint8_t minute_bcd_l;
uint8_t minute_bcd_h;
uint8_t hour_bcd_l;
uint8_t hour_bcd_h;
uint8_t dow;
uint8_t date_bcd_l;
uint8_t date_bcd_h;
uint8_t month_bcd_l;
uint8_t month_bcd_h;
uint8_t year;
uint16_t year_full;

static void _calculate_dow(void)
{
    int16_t _year = year + 2000;
    int8_t _month = month_bcd_h * 10 + month_bcd_l;
    int8_t _date = date_bcd_h * 10 + date_bcd_l;

    int8_t a = _month < 3 ? 1 : 0;
    int16_t y = _year - a;
    int8_t m = _month + 12 * a - 2;
    dow = (7000 + (_date + y + y / 4 - y / 100 + y / 400 + (31 * m) / 12)) % 7 + 1;
}

static inline void _correct_february_day_number(void)
{
    if (year & 3 || year == 0)
    {
        days_in_month[2] = 0x29;
    }
    else
    {
        days_in_month[2] = 0x30;
    }
}

static inline uint8_t _correct_summer_time(void)
{
    uint8_t month_bcd = (month_bcd_h << 4) + month_bcd_l;
    uint8_t date = date_bcd_h * 10 + date_bcd_l;
    uint8_t hour_bcd = (hour_bcd_h << 4) + hour_bcd_l;
    if (month_bcd < 0x03 || month_bcd > 0x10) // it is Winter
    {
        return 0;
    }
    if (month_bcd > 0x03 && month_bcd < 0x10) // it is Summer
    {
        return 1;
    }
    if (month_bcd == 0x03) // March
    {
        if (23 + dow >= date) // before last Sunday
        {
            return 0;
        }
        if (dow == 1 && hour_bcd + TIME_ZONE < 3)  // Sunday before 3
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }
    else if (month_bcd == 0x10) // October
    {
        if (23 + dow >= date) //  before last Sunday
        {
            return 1;
        }
        if (dow == 1 && hour_bcd + TIME_ZONE <= 3)  // Sunday before 3
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
    return 0;
}

static void _correct_time_zone(void)
{
    uint8_t tz = TIME_ZONE;
    tz += _correct_summer_time();
    _correct_february_day_number();
    hour_bcd_l += tz;
    if (hour_bcd_l >= 10)
    {
        hour_bcd_l -= 10;
        hour_bcd_h++;
    }
    if (hour_bcd_h == 2 && hour_bcd_l == 4)
    {
        hour_bcd_l = 0;
        hour_bcd_h = 0;
        date_bcd_l++;
        if (date_bcd_l == 10)
        {
            date_bcd_l = 0;
            date_bcd_h++;
        }
        if ((date_bcd_h << 4) + date_bcd_l == days_in_month[(month_bcd_h << 4) + month_bcd_l])
        {
            date_bcd_l = 1;
            date_bcd_h = 0;
            month_bcd_l++;
            if (month_bcd_l == 10)
            {
                month_bcd_l = 0;
                month_bcd_h++;
            }
            if (month_bcd_h == 1 && month_bcd_l == 3)
            {
                month_bcd_l = 1;
                month_bcd_h = 0;
                year++;
            }
        }
    }
    _calculate_dow();
}

static uint8_t _goto_next_field(uint8_t **buf)
{
    while (*buf < _buf + _index)
    {
        (*buf)++;
        if (**buf == ',')
        {
            return 0;
        }
    }
    return 1;
}

inline uint8_t _read_bcd(uint8_t **buf)
{
    (*buf)++;
    return **buf - '0';
}

static void _packet_parser1(void)
{
    if (_buf[2] == 'Z' &&
        _buf[3] == 'D' &&
        _buf[4] == 'A')
    {
        uint8_t *_tmp_buf = _buf;
        if (_goto_next_field(&_tmp_buf))
        {
            return;
        }
        hour_bcd_h = _read_bcd(&_tmp_buf);
        hour_bcd_l = _read_bcd(&_tmp_buf);
        minute_bcd_h = _read_bcd(&_tmp_buf);
        minute_bcd_l = _read_bcd(&_tmp_buf);
        second_bcd_h = _read_bcd(&_tmp_buf);
        second_bcd_l = _read_bcd(&_tmp_buf);
        if (_goto_next_field(&_tmp_buf))
        {
            return;
        }
        date_bcd_h = _read_bcd(&_tmp_buf);
        date_bcd_l = _read_bcd(&_tmp_buf);
        if (_goto_next_field(&_tmp_buf))
        {
            return;
        }
        month_bcd_h = _read_bcd(&_tmp_buf);
        month_bcd_l = _read_bcd(&_tmp_buf);
        if (_goto_next_field(&_tmp_buf))
        {
            return;
        }
        year_full = _read_bcd(&_tmp_buf) * 1000;
        year_full += _read_bcd(&_tmp_buf) * 100;
        year = _read_bcd(&_tmp_buf) * 10;
        year += _read_bcd(&_tmp_buf);
        year_full += year;

        if (hour_bcd_h > 9 || hour_bcd_l > 9 ||
            minute_bcd_h > 9 || minute_bcd_l > 9 ||
            second_bcd_h > 9 || second_bcd_l > 9 ||
            date_bcd_h > 9 || date_bcd_l > 9 ||
            month_bcd_h > 9 || month_bcd_l > 9)
        {
            return;
        }

        _correct_time_zone();
        if (_time_cb != NULL)
        {
            _time_cb();
        }
    }
}

// $GPRMC,124123.00,V,,,,,,,160521,,,N*7B
static void _packet_parser(void)
{
    if (_buf[2] == 'R' &&
        _buf[3] == 'M' &&
        _buf[4] == 'C')
    {
        uint8_t *_tmp_buf = _buf;
        if (_goto_next_field(&_tmp_buf))
        {
            return;
        }
        hour_bcd_h = _read_bcd(&_tmp_buf);
        hour_bcd_l = _read_bcd(&_tmp_buf);
        minute_bcd_h = _read_bcd(&_tmp_buf);
        minute_bcd_l = _read_bcd(&_tmp_buf);
        second_bcd_h = _read_bcd(&_tmp_buf);
        second_bcd_l = _read_bcd(&_tmp_buf);
        for (uint8_t i=0; i<7; i++)
        {
            _goto_next_field(&_tmp_buf);
        }
        if (_goto_next_field(&_tmp_buf))
        {
            return;
        }
        date_bcd_h = _read_bcd(&_tmp_buf);
        date_bcd_l = _read_bcd(&_tmp_buf);
        month_bcd_h = _read_bcd(&_tmp_buf);
        month_bcd_l = _read_bcd(&_tmp_buf);
        year = _read_bcd(&_tmp_buf) * 10;
        year += _read_bcd(&_tmp_buf);

        if (hour_bcd_h > 9 || hour_bcd_l > 9 ||
            minute_bcd_h > 9 || minute_bcd_l > 9 ||
            second_bcd_h > 9 || second_bcd_l > 9 ||
            date_bcd_h > 9 || date_bcd_l > 9 ||
            month_bcd_h > 9 || month_bcd_l > 9)
        {
            return;
        }
        uart_send_byte('|');
        uart_send_byte('0' + hour_bcd_h);
        uart_send_byte('0' + hour_bcd_l);
        uart_send_byte(':');
        uart_send_byte('0' + minute_bcd_h);
        uart_send_byte('0' + minute_bcd_l);
        uart_send_byte(':');
        uart_send_byte('0' + second_bcd_h);
        uart_send_byte('0' + second_bcd_l);
        uart_send_byte(' ');
        uart_send_byte('0' + date_bcd_h);
        uart_send_byte('0' + date_bcd_l);
        uart_send_byte('.');
        uart_send_byte('0' + month_bcd_h);
        uart_send_byte('0' + month_bcd_l);
        uart_send_byte('.');
        uart_send_byte('0' + year / 10);
        uart_send_byte('0' + year % 10);
        uart_send_byte('|');

        _correct_time_zone();
        if (_time_cb != NULL)
        {
            _time_cb();
        }
    }
}

void neo7m_init(time_cb cb)
{
    _time_cb = cb;
    uart_init();
}

void neo7m_handler(void)
{
    while (uart_check_receiver())
    {
        uint8_t tmp = uart_get_byte();
        // uart_send_byte(tmp);
        // uart_send_hex(tmp);
        if (tmp == START_CODE)
        {
            _index = 0;
        }
        else if (_index < PACKET_MAX_LEN)
        {
            if (tmp == END_CODE)
            {
                if (_index > PACKET_MIN_LEN)
                {
                    _packet_parser();
                }
            }
            else
            {
                _buf[_index++] = tmp;
            }
        }
    }
}

uint8_t get_second_bcd_l(void)
{
    return second_bcd_l;
}

uint8_t get_second_bcd_h(void)
{
    return second_bcd_h;
}

uint8_t get_minute_bcd_l(void)
{
    return minute_bcd_l;
}

uint8_t get_minute_bcd_h(void)
{
    return minute_bcd_h;
}

uint8_t get_hour_bcd_l(void)
{
    return hour_bcd_l;
}

uint8_t get_hour_bcd_h(void)
{
    return hour_bcd_h;
}

uint8_t get_dow(void)
{
    return dow;
}

uint8_t get_date_bcd_l(void)
{
    return date_bcd_l;
}

uint8_t get_date_bcd_h(void)
{
    return date_bcd_h;
}

uint8_t get_month(void)
{
    return month_bcd_h * 10 + month_bcd_l;
}

uint8_t get_year(void)
{
    return year;
}
