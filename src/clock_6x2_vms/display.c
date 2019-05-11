#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdint.h>
#include "config.h"
#include "max7219.h"
#include "fonts.h"
#include "cyrillicfont.h"
#include "text.h"
#include "UART.h"
#include "neo7m.h"
#include "timer.h"
#include "display.h"

#define DISPLAY_TIMER_ID 0

#define MAX7219_PER_ROW 6

#define DISPLAY_VIRTUAL_ROWS 32
#define DISPLAY_ROWS 16
#define DISPLAY_BOTTOM_2_SHIFT 64

const uint8_t bit_reverse_table256[] PROGMEM =
{
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};
#define reverse(byte) pgm_read_byte(&bit_reverse_table256[byte])

const uint8_t img_table[16][6] PROGMEM =
{
    0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x9,0x74,0x4c,0x60,0x0,
    0x0,0x9,0x46,0xd2,0x80,0x0,
    0x0,0xf,0x75,0x52,0xe0,0x0,
    0x0,0x9,0x44,0x5e,0x80,0x0,
    0x0,0x9,0x74,0x52,0x60,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x64,0xba,0x4c,0x35,0x0,
    0x0,0x94,0xa2,0x52,0x55,0x0,
    0x0,0x85,0xa3,0xd2,0x53,0x0,
    0x0,0x96,0xa2,0x5e,0x51,0x0,
    0x0,0x64,0xa2,0x52,0x96,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0
};

static uint8_t display_full_buf[DISPLAY_ROWS][MAX7219_PER_ROW];
static uint8_t display_top_buf[MAX7219_ROWS][MAX7219_PER_ROW];
static uint8_t display_bottom_buf[MAX7219_ROWS][32];
static uint8_t *display_buf[DISPLAY_VIRTUAL_ROWS] =
{
    display_full_buf[0], display_full_buf[1], display_full_buf[2], display_full_buf[3], display_full_buf[4], display_full_buf[5], display_full_buf[6], display_full_buf[7],
    display_full_buf[8], display_full_buf[9], display_full_buf[10], display_full_buf[11], display_full_buf[12], display_full_buf[13], display_full_buf[14], display_full_buf[15],
    display_top_buf[0], display_top_buf[1], display_top_buf[2], display_top_buf[3], display_top_buf[4], display_top_buf[5], display_top_buf[6], display_top_buf[7],
    display_bottom_buf[0], display_bottom_buf[1], display_bottom_buf[2], display_bottom_buf[3], display_bottom_buf[4], display_bottom_buf[5], display_bottom_buf[6], display_bottom_buf[7]
};

static uint8_t display_row_shift=16;
static uint8_t display_col_shift=0;

#define DISPLAY_STATE_IDLE 0
#define DISPLAY_STATE_ACTIVATING 1
#define DISPLAY_STATE_WAITING 2
#define DISPLAY_STATE_ACTIVE 3
#define DISPLAY_STATE_INACTIVE 4
static uint8_t display_state=DISPLAY_STATE_IDLE;

void print_img(void);
void print_top_time(void);
void print_bottom_dow(void);
void print_bottom_date(void);
void display_wait_down(void);
void display_wait_right(void);
void display_scroll_up(void);
void display_scroll_down(void);
void display_scroll_right(void);
void display_clear_buf(void);

void display_init(void)
{
    display_clear_buf();
    print_img();
    max7219_init();
    max7219_update();
}

void print_full_time(void)
{
    uint8_t h_l = get_hour_bcd_l();
    uint8_t h_h = get_hour_bcd_h();
    uint8_t m_l = get_minute_bcd_l();
    uint8_t m_h = get_minute_bcd_h();

    for (uint8_t i=0; i<30; i+=2)
    {
        display_full_buf[i>>1][0] = tdDigital15table(h_h, i);
        display_full_buf[i>>1][1] = tdDigital15table(h_h, i+1) | (tdDigital15table(h_l, i) >> 4);
        display_full_buf[i>>1][2] = (tdDigital15table(h_l, i) << 4) | (tdDigital15table(h_l, i+1) >> 4);
        display_full_buf[i>>1][3] = tdDigital15table(m_h, i) >> 1;
        display_full_buf[i>>1][4] = (tdDigital15table(m_h, i) << 7) | (tdDigital15table(m_h, i+1) >> 1) | (tdDigital15table(m_l, i) >> 5);
        display_full_buf[i>>1][5] = (tdDigital15table(m_l, i) << 3) | (tdDigital15table(m_l, i+1) >> 5);
    }
}

void print_img(void)
{
    for (uint8_t i=0; i<6; i++)
    {
        for (uint8_t j=0; j<16; j++)
        {
            uint8_t tmp = pgm_read_byte(&img_table[j][i]);
            display_full_buf[j][i] = tmp;
        }
    }
}

void print_top_time(void)
{
    uint8_t h_l = get_hour_bcd_l();
    uint8_t h_h = get_hour_bcd_h();
    uint8_t m_l = get_minute_bcd_l();
    uint8_t m_h = get_minute_bcd_h();
    uint8_t s_l = get_second_bcd_l();
    uint8_t s_h = get_second_bcd_h();

    if(h_h == 0)
    {
        for (uint8_t i=0; i<7; i++)
        {
            display_top_buf[i][0] = 0;
        }
    }
    else
    {
        for (uint8_t i=0; i<7; i++)
        {
            display_top_buf[i][0] = dDigital7table(h_h, i) >> 1;
        }
    }

    for (uint8_t i=0; i<7; i++)
    {
        display_top_buf[i][1] = dDigital7table(h_l, i);
        display_top_buf[i][2] = dDigital7table(m_h, i) >> 2;
        display_top_buf[i][3] = dDigital7table(m_l, i) >> 1;
        display_top_buf[i][4] = dDigital7table(s_h, i) >> 3;
        display_top_buf[i][5] = dDigital7table(s_l, i) >> 2;
    }

    if(s_l & 0x01)
    {
        display_top_buf[1][1] |= 0x01;
        display_top_buf[5][1] |= 0x01;
        display_top_buf[1][4] |= 0x80;
        display_top_buf[5][4] |= 0x80;
    }
}

void print_bottom_dow(void)
{
    uint8_t dow = get_dow();
    uint8_t shift=get_dow_shift(dow);
    uint8_t col, col_shift, c, c_len;

    for(uint8_t i=0; i<MAX7219_ROWS; i++)
        for(uint8_t j=0; j<MAX7219_PER_ROW; j++)
            display_bottom_buf[i][j] = 0;

    for(uint8_t i=0; i<get_dow_len(dow); i++)
    {
        c = get_dow_str(dow, i);
        c_len = cyr7width(c);
        col = shift >> 3;
        col_shift = shift & 0x07;
        if(col_shift)
        {
            for (uint8_t j=0; j<7; j++)
            {
                display_bottom_buf[j][col] |= cyr7table(c, j) >> col_shift;
            }
            if((col_shift + c_len) > 7)
            {
                col++;
                col_shift = 8 - col_shift;
                for (uint8_t j=0; j<7; j++)
                {
                    display_bottom_buf[j][col] = cyr7table(c, j) << col_shift;
                }
            }
        }
        else
        {
            for (uint8_t j=0; j<7; j++)
            {
                display_bottom_buf[j][col] = cyr7table(c, j);
            }
        }
        shift += c_len + 1;
    }
}

void print_bottom_date(void)
{
    uint8_t date_l = get_date_bcd_l();
    uint8_t date_h = get_date_bcd_h();
    uint8_t mon = get_month() - 1;
    uint8_t width = _months_width(mon) + 6;
    if (date_h > 0)
    {
        width += 6;
    }
    uint8_t shift = DISPLAY_BOTTOM_2_SHIFT + (48 - width)/2;
    uint8_t col, col_shift;

    for(uint8_t i=0; i<8; i++)
        for(uint8_t j=8; j<14; j++)
            display_bottom_buf[i][j] = 0;

    if(date_h)
    {
        col = shift >> 3;
        col_shift = shift & 0x07;
        for (uint8_t j=0; j<7; j++)
        {
            display_bottom_buf[j][col] = dDigital7table(date_h, j) >> col_shift;
        }
        if((col_shift + 5) > 7)
        {
            col++;
            col_shift = 8 - col_shift;
            for (uint8_t j=0; j<7; j++)
            {
                display_bottom_buf[j][col] = dDigital7table(date_h, j) << col_shift;
            }
        }
        shift += 5 + 1;
    }

    col = shift >> 3;
    col_shift = shift & 0x07;
    if(col_shift)
    {
        for (uint8_t j=0; j<7; j++)
        {
            display_bottom_buf[j][col] |= dDigital7table(date_l, j) >> col_shift;
        }
        if((col_shift + 5) > 7)
        {
            col++;
            col_shift = 8 - col_shift;
            for (uint8_t j=0; j<7; j++)
            {
                display_bottom_buf[j][col] = dDigital7table(date_l, j) << col_shift;
            }
        }
    }
    else
    {
        for (uint8_t j=0; j<7; j++)
        {
            display_bottom_buf[j][col] = dDigital7table(date_l, j);
        }
    }
    shift += 5 + 2;

    col = shift / 8;
    col_shift = shift & 0x07;
    for (uint8_t i=0; i<4; i++) {
        for (uint8_t j=0; j<5; j++) {
            uint8_t tmp = _months_table(mon, j, i);
            if (col_shift) {
                display_bottom_buf[j+2][col] |= tmp >> col_shift;
                display_bottom_buf[j+2][col+1] = tmp << (8 - col_shift);
            } else {
                display_bottom_buf[j+2][col] = tmp;
            }
        }
        col++;
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
        display_bottom_buf[i][19] = dDigital7table(temp_l, i) >> 6;
        display_bottom_buf[i][20] = (dDigital7table(temp_l, i) << 2) | (dDigital7table(temp_f, i) >> 6);
        display_bottom_buf[i][21] = dDigital7table(temp_f, i) << 2;
    }
    display_bottom_buf[3][18] = 0;
    display_bottom_buf[6][20] |= 0x08;
    if (temp_h > 0)
    {
        for (uint8_t i=0; i<7; i++)
        {
            display_bottom_buf[i][19] |= dDigital7table(temp_h, i);
        }
        if (sign)
        {
            display_bottom_buf[3][18] = 0x0E;
        }
    }
    else if (sign)
    {
        display_bottom_buf[3][19] |= 0x38;
    }
    display_bottom_buf[0][21] |= 0x06;
    display_bottom_buf[3][21] |= 0x06;
    display_bottom_buf[1][21] |= 0x09;
    display_bottom_buf[2][21] |= 0x09;
}

void clear_ext_temperature(void)
{
    for (uint8_t i=0; i<7; i++)
    {
        display_bottom_buf[i][18] = 0;
        display_bottom_buf[i][19] = 0;
        display_bottom_buf[i][20] = 0;
        display_bottom_buf[i][21] = 0;
    }
}

void print_pressure(int32_t pressure)
{
    pressure *= 10;
    pressure /= 133;
    uint8_t p1 = pressure / 1000;
    pressure -= (uint16_t)p1 * 1000;
    uint8_t p2 = pressure / 100;
    pressure -= (uint16_t)p2 * 100;
    uint8_t p3 = pressure / 10;
    uint8_t p4 = (uint8_t)pressure - p3 * 10;
    for (uint8_t i=0; i<7; i++)
    {
        display_bottom_buf[i][25] = dDigital7table(p1, i) >> 4;
        display_bottom_buf[i][26] = (dDigital7table(p1, i) << 4) | (dDigital7table(p2, i) >> 2);
        display_bottom_buf[i][27] = dDigital7table(p3, i);
        display_bottom_buf[i][28] = dDigital7table(p4, i);
    }
    display_bottom_buf[6][27] |= 0x02;
}

void print_int_temperature(int16_t t)
{
    t /= 10;
    uint8_t temp_l = t % 10;
    uint8_t temp_h = t / 10;
    for (uint8_t i=0; i<7; i++)
    {
        display_bottom_buf[i][16] = dDigital7table(temp_h, i) | (dDigital7table(temp_l, i) >> 6);
        display_bottom_buf[i][17] = (dDigital7table(temp_l, i) << 2);
    }
    display_bottom_buf[0][17] |= 0x06;
    display_bottom_buf[3][17] |= 0x06;
    display_bottom_buf[1][17] |= 0x09;
    display_bottom_buf[2][17] |= 0x09;
}

void display_clear_buf(void)
{
    for(uint8_t i=0; i<32; i++)
        for(uint8_t j=0; j<6; j++)
            display_buf[i][j] = 0;

    for(uint8_t i=24; i<32; i++)
        for(uint8_t j=0; j<32; j++)
            display_buf[i][j] = 0;
}

void max7219_load_row(uint8_t r, uint8_t *buf)
{
    uint8_t tmp, col, row;
    uint8_t col_shift = display_col_shift & 7;
    uint8_t matrix_shift = display_col_shift >> 3;

    for(uint8_t m=0; m<MAX7219_NUMBER; m++)
    {
        *buf++ = r + 1;

        if(m < MAX7219_PER_ROW)
        {
            row = r + display_row_shift;
            if(row < 24)
                tmp = display_buf[row][m];
            else
            {
                col = (m + matrix_shift) & 31;
                tmp = display_buf[row][col];
                if(col_shift)
                {
                    tmp <<= col_shift;
                    col++;
                    col &= 31;
                    tmp |= display_buf[row][col] >> (8 - col_shift);
                }
            }
            *buf++ = tmp;
        }
        else
        {
            row = 15 - r + display_row_shift;
            col = 11 - m;
            if(row < 24)
                tmp = display_buf[row][col];
            else
            {
                col += matrix_shift;
                col &= 31;
                tmp = display_buf[row][col];
                if (col_shift)
                {
                    tmp <<= col_shift;
                    col++;
                    col &= 31;
                    tmp |= display_buf[row][col] >> (8 - col_shift);
                }
            }
            *buf++ = reverse(tmp);
        }
    }
}

void display_wait_right(void)
{
    timer_register(DISPLAY_TIMER_ID, 1, display_scroll_right);
}

void display_wait_down(void)
{
    timer_register(DISPLAY_TIMER_ID, 1, display_scroll_down);
}

void display_scroll_up(void)
{
    if (display_row_shift > 0)
    {
        display_row_shift--;
    }
    if (display_row_shift == 0)
    {
        display_state = DISPLAY_STATE_WAITING;
        timer_stop(DISPLAY_TIMER_ID);
        // timer_register(DISPLAY_TIMER_ID, 50, display_wait_down);//////
    }
    max7219_update();
}

void display_scroll_down(void)
{
    if (display_row_shift < 16)
    {
        display_row_shift++;
        max7219_update();
    }
    if (display_row_shift == 16)
    {
        timer_register(DISPLAY_TIMER_ID, 50, display_wait_right);
    }
}

void display_scroll_right(void)
{
    if((++display_col_shift & 0x3F) == 0)
    {
        timer_register(DISPLAY_TIMER_ID, 50, display_wait_right);
    }
    max7219_update();
}

void display_activate(void)
{
    display_state = DISPLAY_STATE_ACTIVATING;
    print_img();
    max7219_update();
    timer_register(DISPLAY_TIMER_ID, 1, display_scroll_up);
}

void display_deactivate(void)
{
    display_state = DISPLAY_STATE_INACTIVE;
    display_clear_buf();
    display_row_shift = 16;
    display_col_shift = 0;
    max7219_update_with_config();
    timer_stop(DISPLAY_TIMER_ID);
}

void time_update_handler(void)
{
    if(display_state == DISPLAY_STATE_ACTIVE)
    {
        print_top_time();
        print_bottom_dow();
        print_bottom_date();
        if (get_second_bcd_l())
        {
            max7219_update();
        }
        else
        {
            max7219_update_with_config();
        }
    }
    else if(display_state == DISPLAY_STATE_ACTIVATING)
    {
        print_img();
        max7219_update();
    }
    else if(display_state == DISPLAY_STATE_WAITING)
    {
        print_top_time();
        print_bottom_dow();
        print_bottom_date();
        display_col_shift = 0;
        display_state = DISPLAY_STATE_ACTIVE;
        timer_register(DISPLAY_TIMER_ID, 1, display_wait_down);
    }
}
