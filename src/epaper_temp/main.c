#include <stdint.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "UART.h"
#include "SPI.h"
#include "timer.h"
#include "serial.h"
#include "epaper217.h"
#include "custom_font.h"
#include "ds18b20.h"
#include "onewire.h"

#define PRINT_TEMPERATURE_HISTORY 1
// #define PRINT_MESSAGE 1

#define ONEWIRE_VDD_PORT PORTB
#define ONEWIRE_VDD_DDR DDRB
#define ONEWIRE_VDD_PIN PB1
#define ONEWIRE_GND_PORT PORTD
#define ONEWIRE_GND_DDR DDRD
#define ONEWIRE_GND_PIN PD7

#define TEMPERATURE_X_SHIFT 64
#define MESSAGE_X_SHIFT 16
#define HISTORY_X_SHIFT 16
#define HISTORY_LINE_X_SHIFT 8
#define HISTORY_Y_SHIFT 10
#define HISTORY_WIDTH 240  // 24 h every 6 min
#define HISTORY_HEIGHT 32

extern custom_font_t font_40_1040_1103;
extern custom_font_t font_48_44_57;
extern custom_font_t font_48_176_176;
extern custom_font_t font_8_44_57;

static custom_font_t *font_ua = &font_40_1040_1103;
static custom_font_t *font_digits = &font_48_44_57;
static custom_font_t *font_degree = &font_48_176_176;
static custom_font_t *font_digits_small = &font_8_44_57;

static uint8_t cold_str[] = " Шось зимно ";
static uint8_t hot_str[] = " Тепленько ";
static uint8_t cold_str_width;
static uint8_t hot_str_width;
static uint16_t temperature_raw;
static uint8_t temperature_is_negative;
static uint8_t temperature_value;
static uint8_t temperature_value_fractional;
static uint8_t temperature_updated;
static int8_t temperature_history_6m[6];
static int8_t temperature_history_24h[HISTORY_WIDTH];
static uint8_t temperature_history_6m_id;
static uint8_t temperature_history_24h_id;

void convert_temperature(void);

static uint8_t str_width(custom_font_t *font, uint8_t *str)
{
    uint8_t width = 0;
    do {
        uint8_t c_shift = *str - font->char_start;
        if (c_shift < font->size)
        {
            width += pgm_read_byte(&font->shift_table[c_shift].width);
        }
        else
        {
            width += font->height / 2;
        }
    } while (*(++str));
    return width;
}

static uint8_t print_char(custom_font_t *font, uint8_t x, uint16_t *y, uint8_t c)
{
    uint8_t height = font->height;
    uint8_t c_shift = c - font->char_start;
    if (c_shift >= font->size)
    {
        return 1;
    }
    uint8_t width = pgm_read_byte(&font->shift_table[c_shift].width);
    uint16_t shift = pgm_read_word(&font->shift_table[c_shift].shift);
    epaper_update_frame_progmem(&font->char_table[shift], x, *y, x + height, *y + width, 0);
    *y += width;
    return 0;
}

static void print_str(custom_font_t *font, uint8_t x, uint16_t *y, uint8_t *str)
{
    do {
        if (print_char(font, x, y, *str))
        {
            epaper_clear_frame(x, *y, x + font->height, *y + font_ua->height / 2, 0);
            *y += font_ua->height / 2;
        }
    } while (*(++str));
}

static void print_msg(void)
{
    uint16_t y = EPD_HEIGHT;
    uint8_t *str;
    if (temperature_value < 22 || temperature_is_negative)
    {
        y -= cold_str_width;
        str = cold_str;
    }
    else
    {
        y -= hot_str_width;
        str = hot_str;
    }
    y /= 2;
    print_str(font_ua, MESSAGE_X_SHIFT, &y, str);
}

static void print_localtime(void)
{
    uint8_t time_str[12];
    uint8_t i = 0;
    uint32_t localtime = get_localtime_ds() / (10 * 60);  // min
    uint8_t t = localtime % 60;
    uint16_t  localtime1 = localtime / 60;  // hour
    time_str[i++] = '0' + t % 10;
    time_str[i++] = '0' + t / 10;
    time_str[i++] = '.';
    t = localtime1 % 24;
    localtime1 /= 24;
    time_str[i++] = '0' + t % 10;
    time_str[i++] = '0' + t / 10;
    time_str[i++] = ' ';
    while (localtime1)
    {
        time_str[i++] = '0' + localtime1 % 10;
        localtime1 /= 10;
    }
    for (int j = 0; j < i / 2; j++)  // reverse
    {
        uint8_t temp = time_str[j];
        time_str[j] = time_str[i - 1 - j];
        time_str[i - 1 - j] = temp;
    }
    time_str[i] = 0;
    uint16_t y = 0;
    print_str(font_digits_small, 112, &y, time_str);
}

static void print_static(void)
{
    uint8_t value;
    for (uint16_t i=HISTORY_Y_SHIFT; i<HISTORY_Y_SHIFT + 240; i++)
    {
        if (i == HISTORY_Y_SHIFT + 0 ||
            i == HISTORY_Y_SHIFT + 6 * 10 ||
            i == HISTORY_Y_SHIFT + 12 * 10 ||
            i == HISTORY_Y_SHIFT + 18 * 10)
        {
            value = 0x81;
        }
        else
        {
            value = 0xfd;
        }
        epaper_update_frame_buf(&value, HISTORY_LINE_X_SHIFT, i, HISTORY_LINE_X_SHIFT + 8, i + 1, 0);
    }
    uint8_t buf[4] = {};
    epaper_update_frame_buf(buf, HISTORY_X_SHIFT, HISTORY_Y_SHIFT, HISTORY_X_SHIFT + 32, HISTORY_Y_SHIFT + 1, 0);
    static uint8_t str_6[] = "-6";
    static uint8_t str_12[] = "-12";
    static uint8_t str_18[] = "-18";
    static uint8_t str_24[] = "-24";
    uint16_t y = 0;
    print_str(font_digits_small, 0, &y, str_24);
    y = 60;
    print_str(font_digits_small, 0, &y, str_18);
    y = 120;
    print_str(font_digits_small, 0, &y, str_12);
    y = 180;
    print_str(font_digits_small, 0, &y, str_6);
}

static void print_temperature_history(void)
{
    int8_t temperature_min = 0;
    int8_t temperature_max = 10 << 1;  // 1 fractional bit
    static uint8_t buf[4] = {0xff, 0xff, 0xff, 0xff};
    for (uint8_t i = 0; i < HISTORY_WIDTH; i++)
    {
        int8_t value = temperature_history_24h[i];
        if (value < temperature_min)
        {
            temperature_min = value;
        }
        if (value > temperature_max)
        {
            temperature_max = value;
        }
    }
    int8_t temperature_diff = temperature_max - temperature_min;
    for (uint8_t i = 0; i < HISTORY_WIDTH; i++)
    {
        uint8_t value_id = i + temperature_history_24h_id;
        if (value_id >= HISTORY_WIDTH)
        {
            value_id -= HISTORY_WIDTH;
        }
        int16_t value = temperature_history_24h[value_id];
        value -= temperature_min;
        value *= 31;
        value /= temperature_diff;
        uint8_t buf_shift = value >> 3;
        value &= 7;
        uint8_t history_value = 0x80 >> value;
        buf[buf_shift] &= ~history_value;
        epaper_update_frame_buf(buf, HISTORY_X_SHIFT, i + HISTORY_Y_SHIFT, HISTORY_X_SHIFT + HISTORY_HEIGHT, i + HISTORY_Y_SHIFT + 1, 1);
        buf[buf_shift] |= history_value;
    }
    temperature_max >>= 1;
    uint16_t y = 0;
    print_char(font_digits_small, HISTORY_X_SHIFT + HISTORY_HEIGHT, &y, '0' + temperature_max / 10);
    print_char(font_digits_small, HISTORY_X_SHIFT + HISTORY_HEIGHT, &y, '0' + temperature_max % 10);
    y = 0;
    temperature_min >>= 1;
    print_char(font_digits_small, HISTORY_X_SHIFT, &y, '0' + temperature_min % 10);
}

static void print_temperature(void)
{
    uint8_t temperature_str[6];
    uint8_t i = 0;
    if (temperature_is_negative)
    {
        temperature_str[i++] = '-';
    }
    uint8_t t = temperature_value / 10;
    if (t)
    {
        temperature_str[i++] = t + '0';
        t = temperature_value - t * 10;
    }
    else
    {
        t = temperature_value;
    }
    temperature_str[i++] = t + '0';
    temperature_str[i++] = '.';
    temperature_str[i++] = temperature_value_fractional + '0';
    temperature_str[i++] = 0;

    epaper_clear_frame(TEMPERATURE_X_SHIFT, 0, TEMPERATURE_X_SHIFT + font_digits->height, EPD_WIDTH, 0);
    uint16_t y = EPD_HEIGHT - str_width(font_digits, temperature_str);
    y /= 2;
    print_str(font_digits, TEMPERATURE_X_SHIFT, &y, temperature_str);
    print_char(font_degree, TEMPERATURE_X_SHIFT, &y, '°');
}

static void temperature_update_handler(void)
{
    if (temperature_raw == 0)
    {
        uart_send_byte('e');
        return;
    }
    if (temperature_raw & 0x0800)
    {
        temperature_raw = ~temperature_raw;
        temperature_raw++;
        temperature_raw |= 0x0800;
        temperature_is_negative = 1;
    }
    else
    {
        temperature_is_negative = 0;
    }
    temperature_value = temperature_raw >> 4;
    temperature_value_fractional = ((temperature_raw & 0x0F) * 10) / 16;

    uart_send_byte('|');
    if (temperature_is_negative)
    {
        uart_send_byte('-');
    }
    uart_send_byte(temperature_value / 10 + '0');
    uart_send_byte(temperature_value % 10 + '0');
    uart_send_byte('.');
    uart_send_byte(temperature_value_fractional + '0');
    uart_send_byte('|');

    int8_t temperature_value_short = temperature_raw >> 3;
    if (temperature_is_negative)
    {
        temperature_value_short *= -1;
    }
    temperature_history_6m[temperature_history_6m_id++] = temperature_value_short;
    if (temperature_history_6m_id == 3 || temperature_history_6m_id == 6)
    {
        if (temperature_history_6m_id == 6)
        {
            temperature_history_6m_id = 0;
            int16_t temperature_6m_avg = 0;
            for (uint8_t i = 0; i < 6; i++)
            {
                temperature_6m_avg += temperature_history_6m[i];
            }
            temperature_6m_avg /= 6;
            temperature_history_24h[temperature_history_24h_id++] = temperature_6m_avg;
            if (temperature_history_24h_id == HISTORY_WIDTH)
            {
                temperature_history_24h_id = 0;
            }
        }
        // print every 3 min
        epaper_wake();
        print_temperature();
#if defined(PRINT_TEMPERATURE_HISTORY)
        print_temperature_history();
#elif defined(PRINT_MESSAGE)
        print_msg();
#endif
        print_localtime();
        epaper_display_frame();
        epaper_sleep();
    }
}

void temperature_cb(uint16_t data)
{
    temperature_raw = data;
    temperature_updated = 1;
}

void read_temperature(void)
{
    ds18b20_read_temperature(temperature_cb);
    timer_register(0, 10, convert_temperature);
}

void convert_temperature(void)
{
    static uint8_t timeout_s = 0;
    if (timeout_s)
    {
        timer_register(0, 10, convert_temperature);
        timeout_s--;
    }
    else
    {
        ds18b20_convert();
        timer_register(0, 10, read_temperature);
        timeout_s = 60;
    }
}

void setup(void)
{
    ONEWIRE_VDD_DDR |= 1 << ONEWIRE_VDD_PIN;
    ONEWIRE_VDD_PORT |= 1 << ONEWIRE_VDD_PIN;
    ONEWIRE_GND_DDR |= 1 << ONEWIRE_GND_PIN;
    ONEWIRE_GND_PORT &= ~(1 << ONEWIRE_GND_PIN);
    spi_master_init();
    serial_init();
    timer_init();
    onewire_init();
    asm("sei");
    // _delay_ms(5);
    epaper_init();
#if defined(PRINT_TEMPERATURE_HISTORY)
    print_static();
#endif
    epaper_display_frame();
    epaper_sleep();
    set_sleep_mode(SLEEP_MODE_IDLE);
}

int main(void)
{
    setup();
    cold_str_width = str_width(font_ua, cold_str);
    hot_str_width = str_width(font_ua, hot_str);
    convert_temperature();
    uart_send_byte('S');

    while (1)
    {
        if (temperature_updated)
        {
            temperature_updated = 0;
            temperature_update_handler();
        }
        serial_handler();
        timer_handler();
        sleep_mode();
    }
}
