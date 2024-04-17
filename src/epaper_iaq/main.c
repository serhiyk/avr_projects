#include <stdint.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "UART.h"
#include "SPI.h"
#include "TWI.h"
#include "timer.h"
#include "serial.h"
#include "bme68x.h"
#include "epaper217.h"
#include "custom_font.h"

#define SENSOR_STATE_IDLE 0
#define SENSOR_STATE_READ_DATA 1
#define SENSOR_STATE_DATA_READY 2

#define GAS_LOWER_LIMIT 5000  // Bad air quality limit
#define GAS_UPPER_LIMIT 50000  // Good air quality limit

#define TEMPERATURE_X_SHIFT 64
#define PRESSURE_X_SHIFT 0
#define PRESSURE_Y_SHIFT 24
#define HUMIDITY_X_SHIFT 0
#define HUMIDITY_Y_SHIFT 120
#define GAS_X_SHIFT 0
#define GAS_Y_SHIFT 216
#define MESSAGE_X_SHIFT 24
#define MESSAGE_Y_SHIFT 16

extern struct bme68x_data sensor_data;

extern custom_font_t font_16_1040_1103;
extern custom_font_t font_16_37_57;
extern custom_font_t font_48_44_57;
extern custom_font_t font_48_176_176;
extern custom_font_t font_8_44_57;

static custom_font_t *font_ua = &font_16_1040_1103;
static custom_font_t *font_digits_mid = &font_16_37_57;
static custom_font_t *font_digits = &font_48_44_57;
static custom_font_t *font_degree = &font_48_176_176;
static custom_font_t *font_digits_small = &font_8_44_57;

static uint8_t static_str[] = "Тиск       Вол       Пов";
static uint8_t tmp_str[12];
static volatile uint8_t sensor_state;

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
            epaper_clear_frame(x, *y, x + font->height, *y + font->height / 2, 0);
            *y += font->height / 2;
        }
    } while (*(++str));
}

static void print_localtime(void)
{
    uint8_t i = 0;
    uint32_t localtime = get_localtime_ds() / (10 * 60);  // min
    // uart_send_hex(localtime >> 24);
    // uart_send_hex(localtime >> 16);
    // uart_send_hex(localtime >> 8);
    // uart_send_hex(localtime >> 0);
    uint8_t t = localtime % 60;
    uint16_t  localtime1 = localtime / 60;  // hour
    tmp_str[i++] = '0' + t % 10;
    tmp_str[i++] = '0' + t / 10;
    tmp_str[i++] = '.';
    t = localtime1 % 24;
    localtime1 /= 24;
    tmp_str[i++] = '0' + t % 10;
    tmp_str[i++] = '0' + t / 10;
    tmp_str[i++] = ' ';
    while (localtime1)
    {
        tmp_str[i++] = '0' + localtime1 % 10;
        localtime1 /= 10;
    }
    for (int j = 0; j < i / 2; j++)  // reverse
    {
        uint8_t temp = tmp_str[j];
        tmp_str[j] = tmp_str[i - 1 - j];
        tmp_str[i - 1 - j] = temp;
    }
    tmp_str[i] = 0;
    uint16_t y = 0;
    print_str(font_digits_small, 112, &y, tmp_str);
}

static void print_temperature(void)
{
    uint8_t temperature_value, temperature_value_fractional, i = 0;
    temperature_value = sensor_data.temperature / 100;
    temperature_value_fractional = sensor_data.temperature % 100;
    temperature_value_fractional /= 10;
    uint8_t t = temperature_value / 10;
    if (t)
    {
        tmp_str[i++] = t + '0';
        t = temperature_value - t * 10;
    }
    else
    {
        t = temperature_value;
    }
    tmp_str[i++] = t + '0';
    tmp_str[i++] = '.';
    tmp_str[i++] = temperature_value_fractional + '0';
    tmp_str[i++] = 0;

    epaper_clear_frame(TEMPERATURE_X_SHIFT, 0, TEMPERATURE_X_SHIFT + font_digits->height, EPD_WIDTH, 0);
    uint16_t y = EPD_HEIGHT - str_width(font_digits, tmp_str);
    y /= 2;
    print_str(font_digits, TEMPERATURE_X_SHIFT, &y, tmp_str);
    print_char(font_degree, TEMPERATURE_X_SHIFT, &y, '°');
}

static void print_pressure(void)
{
    uint16_t p = (sensor_data.pressure * 75) / 10000;
    tmp_str[0] = '0' + p / 100;
    uint8_t p1 = p % 100;
    tmp_str[1] = '0' + p1 / 10;
    tmp_str[2] = '0' + p1 % 10;
    tmp_str[3] = 0;
    uint16_t y = PRESSURE_Y_SHIFT;
    print_str(font_digits_mid, PRESSURE_X_SHIFT, &y, tmp_str);
}

static void print_humidity(void)
{
    uint8_t h = sensor_data.humidity / 1000;
    tmp_str[0] = '0' + h / 10;
    tmp_str[1] = '0' + h % 10;
    tmp_str[2] = 0;
    uint16_t y = HUMIDITY_Y_SHIFT;
    print_str(font_digits_mid, HUMIDITY_X_SHIFT, &y, tmp_str);
}

static void print_gas(void)
{
    uint8_t g = (sensor_data.gas_resistance * 100) / GAS_UPPER_LIMIT;
    uint8_t i = 0;
    if (g >= 10)
    {
        tmp_str[i++] = '0' + g / 10;
    }
    tmp_str[i++] = '0' + g % 10;
    tmp_str[i] = 0;
    uint16_t y = GAS_Y_SHIFT;
    print_str(font_digits_mid, GAS_X_SHIFT, &y, tmp_str);
}

static void data_update_handler(void)
{
    epaper_wake();
    print_temperature();
    epaper_clear_frame(0, 0, PRESSURE_X_SHIFT + font_digits_mid->height, EPD_HEIGHT, 0);
    print_pressure();
    print_humidity();
    print_gas();

    print_localtime();
    epaper_display_frame();
    epaper_sleep();
}

static void print_log(void)
{
    uart_send_byte('\n');
    uart_send_byte('\r');
    uart_send_byte('t');
    uart_send_byte('=');
    uart_send_hex(sensor_data.temperature >> 8);
    uart_send_hex(sensor_data.temperature >> 0);
    uart_send_byte('\n');
    uart_send_byte('\r');
    uart_send_byte('h');
    uart_send_byte('=');
    uart_send_hex(sensor_data.humidity >> 24);
    uart_send_hex(sensor_data.humidity >> 16);
    uart_send_hex(sensor_data.humidity >> 8);
    uart_send_hex(sensor_data.humidity >> 0);
    uart_send_byte('\n');
    uart_send_byte('\r');
    uart_send_byte('p');
    uart_send_byte('=');
    uart_send_hex(sensor_data.pressure >> 24);
    uart_send_hex(sensor_data.pressure >> 16);
    uart_send_hex(sensor_data.pressure >> 8);
    uart_send_hex(sensor_data.pressure >> 0);
    uart_send_byte('\n');
    uart_send_byte('\r');
    uart_send_byte('g');
    uart_send_byte('=');
    uart_send_hex(sensor_data.gas_resistance >> 24);
    uart_send_hex(sensor_data.gas_resistance >> 16);
    uart_send_hex(sensor_data.gas_resistance >> 8);
    uart_send_hex(sensor_data.gas_resistance >> 0);
    uart_send_byte('\n');
    uart_send_byte('\r');
}

static void print_static(void)
{
    uint16_t y = MESSAGE_Y_SHIFT;
    print_str(font_ua, MESSAGE_X_SHIFT, &y, static_str);
}

void sensor_timer_handler(void)
{
    static uint8_t timeout_s = 20;
    if (timeout_s)
    {
        if (timeout_s == 10)
        {
            sensor_state = SENSOR_STATE_READ_DATA;
        }
        timeout_s--;
    }
    else
    {
        sensor_state = SENSOR_STATE_DATA_READY;
        timeout_s = 60;
    }
    timer_register(0, 10, sensor_timer_handler);
}

void sensor_handler(void)
{
    if (sensor_state)
    {
        if (sensor_state == SENSOR_STATE_READ_DATA)
        {
            bme68x_set_op_mode(BME68X_FORCED_MODE);
        }
        else
        {

            bme68x_get_data();
            bme68x_get_data_1();
            if (sensor_data.status & (BME68X_HEAT_STAB_MSK | BME68X_GASM_VALID_MSK))
            {
                print_log();
                data_update_handler();
            }
            else
            {
                uart_send_hex(sensor_data.status);
            }
        }
        sensor_state = SENSOR_STATE_IDLE;
    }
}

uint8_t sensor_init(void)
{
    int8_t result = bme68x_init();
    if (!result)
    {
        bme68x_set_conf();
        bme68x_set_heatr_conf();
        bme68x_set_op_mode(BME68X_FORCED_MODE);
    }
    return result;
}

void setup(void)
{
    spi_master_init();
    serial_init();
    twi_init();
    timer_init();
    asm("sei");
    // _delay_ms(5);

    epaper_init();
    epaper_clear_frame_all(0);
    print_static();
    epaper_display_frame();
    epaper_sleep();

    sensor_init();

    set_sleep_mode(SLEEP_MODE_IDLE);

    uart_send_byte('S');
}

int main(void)
{
    setup();
    sensor_timer_handler();

    while (1)
    {
        sensor_handler();
        serial_handler();
        timer_handler();
        sleep_mode();
    }
}
