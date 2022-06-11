#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include "config.h"
#include "max7219.h"
#include "fonts.h"
#include "DS3231.h"
#include "display.h"

static uint8_t display_buf[MAX7219_ROWS][MAX7219_NUMBER];

static void print_time(void)
{
    uint8_t h = get_hour_bcd();
    uint8_t h_h = h >> 4;
    h &= 0x0F;
    uint8_t m = get_minute_bcd();
    uint8_t m_h = m >> 4;
    m &= 0x0F;
    uint8_t s = get_second_bcd();

    if(h_h == 0)
    {
        for (uint8_t i=0; i<7; i++)
        {
            display_buf[i][0] = 0;
        }
    }
    else
    {
        for (uint8_t i=0; i<7; i++)
        {
            display_buf[i][0] = dDigital7table(h_h, i) >> 1;
        }
    }

    for (uint8_t i=0; i<7; i++)
    {
        display_buf[i][1] = dDigital7table(h, i);
        display_buf[i][2] = dDigital7table(m_h, i) >> 2;
        display_buf[i][3] = dDigital7table(m, i) >> 1;
    }

    if(s & 0x01)
    {
        display_buf[1][1] |= 0x01;
        display_buf[5][1] |= 0x01;
    }
}

void print_ext_temperature(int16_t temperature)
{
    uint8_t temp_h, temp_l, temp_f, sign=0;
    if (temperature < 0)
    {
        temperature *= -1;
        sign = 1;
    }
    temp_l = temperature >> 4;
    temp_h = temp_l / 10;
    temp_l %= 10;
    temp_f = temperature & 0x0F;
    temp_f *= 10;
    temp_f /= 16;
    for (uint8_t i=0; i<7; i++)
    {
        display_buf[i][0] = 0;
        display_buf[i][1] = dDigital7table(temp_l, i) >> 6;
        display_buf[i][2] = (dDigital7table(temp_l, i) << 2) | (dDigital7table(temp_f, i) >> 6);
        display_buf[i][3] = dDigital7table(temp_f, i) << 2;
    }
    display_buf[6][2] |= 0x08;
    if (temp_h > 0)
    {
        for (uint8_t i=0; i<7; i++)
        {
            display_buf[i][1] |= dDigital7table(temp_h, i);
        }
        if (sign)
        {
            display_buf[3][0] = 0x0E;
        }
    }
    else if (sign)
    {
        display_buf[3][1] |= 0x38;
    }
    display_buf[0][3] |= 0x06;
    display_buf[3][3] |= 0x06;
    display_buf[1][3] |= 0x09;
    display_buf[2][3] |= 0x09;
    max7219_update();
}

static void display_clear_buf(void)
{
    for(uint8_t i=0; i<MAX7219_ROWS; i++)
        for(uint8_t j=0; j<MAX7219_NUMBER; j++)
            display_buf[i][j] = 0;
}

void display_init(void)
{
    display_clear_buf();
    print_time();
    max7219_init();
    max7219_update();
}

void max7219_load_row(uint8_t r, uint8_t *buf)
{
    for(int8_t m=MAX7219_NUMBER-1; m>=0; m--)
    {
        *buf++ = r + 1;
        *buf++ = display_buf[r][m];
    }
}

void time_update_handler(void)
{
    print_time();
    max7219_update();
}
