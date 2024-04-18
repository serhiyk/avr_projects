#include <Wire.h>
#include <SPI.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include "epd2in13_V4.h"
#include "custom_font.h"

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

static uint8_t static_str[] = {0xD2, 0xE8, 0xF1, 0xEA, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xC2, 0xEE, 0xEB, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xCF, 0xEE, 0xE2, 0x00};
static uint8_t tmp_str[12];

static volatile uint8_t time_updated;
static uint8_t time_s, time_m, time_h;
static uint16_t time_d;

Epd epd;
Adafruit_BME680 bme;

ISR(TIMER1_COMPA_vect)
{
    time_updated = 1;
}

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
    epd.DisplayPartFrame(&font->char_table[shift], x, *y, x + height, *y + width, 0);
    *y += width;
    return 0;
}

static void print_str(custom_font_t *font, uint8_t x, uint16_t *y, uint8_t *str)
{
    do {
        if (print_char(font, x, y, *str))
        {
            epd.DisplayClearPartFrame(x, *y, x + font->height, *y + font->height / 2, 0);
            *y += font->height / 2;
        }
    } while (*(++str));
}

static void print_localtime(void)
{
    uint8_t i = 0;
    tmp_str[i++] = '0' + time_m % 10;
    tmp_str[i++] = '0' + time_m / 10;
    tmp_str[i++] = '.';
    tmp_str[i++] = '0' + time_h % 10;
    tmp_str[i++] = '0' + time_h / 10;
    tmp_str[i++] = ' ';
    uint16_t d = time_d;
    while (d)
    {
        tmp_str[i++] = '0' + d % 10;
        d /= 10;
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
    uint16_t temperature = bme.temperature * 10.0;
    uint8_t i = 0;
    uint8_t t = temperature / 100;
    if (t)
    {
        tmp_str[i++] = '0' + t;
        t = temperature - t * 100;
    }
    else
    {
        t = temperature;
    }
    tmp_str[i++] = '0' + t / 10;
    tmp_str[i++] = '.';
    tmp_str[i++] = '0' + t % 10;
    tmp_str[i++] = 0;

    epd.DisplayClearPartFrame(TEMPERATURE_X_SHIFT, 0, TEMPERATURE_X_SHIFT + font_digits->height, EPD_HEIGHT, 0);
    uint16_t y = EPD_HEIGHT - str_width(font_digits, tmp_str);
    y /= 2;
    print_str(font_digits, TEMPERATURE_X_SHIFT, &y, tmp_str);
    print_char(font_degree, TEMPERATURE_X_SHIFT, &y, '°');
}

static void print_pressure(void)
{
    uint16_t p = (bme.pressure * 75) / 10000;
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
    uint8_t h = bme.humidity;
    tmp_str[0] = '0' + h / 10;
    tmp_str[1] = '0' + h % 10;
    tmp_str[2] = 0;
    uint16_t y = HUMIDITY_Y_SHIFT;
    print_str(font_digits_mid, HUMIDITY_X_SHIFT, &y, tmp_str);
}

static void print_gas(void)
{
    uint8_t g = (bme.gas_resistance * 100) / GAS_UPPER_LIMIT;
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
    epd.Wake();
    print_temperature();
    epd.DisplayClearPartFrame(0, 0, PRESSURE_X_SHIFT + font_digits_mid->height, EPD_HEIGHT, 0);
    print_pressure();
    print_humidity();
    print_gas();

    print_localtime();
    epd.DisplayFrame();
    epd.Sleep();
}

static void print_static(void)
{
    uint16_t y = MESSAGE_Y_SHIFT;
    print_str(font_ua, MESSAGE_X_SHIFT, &y, static_str);
}

static void print_log(void)
{
    Serial.print("t=");
    Serial.println(bme.temperature);
    Serial.print("h=");
    Serial.println(bme.humidity);
    Serial.print("p=");
    Serial.println(bme.pressure);
    Serial.print("g=");
    Serial.println(bme.gas_resistance);
}

static void update_time(void)
{
    time_s++;
    if (time_s == 60)
    {
        time_s = 0;
        time_m++;
        if (time_m == 60)
        {
            time_m = 0;
            time_h++;
            if (time_h == 24)
            {
                time_h = 0;
                time_d++;
            }
        }
    }
}

void setup()
{
    // Timer1: 1 Hz
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    OCR1A = 15624;
    TCCR1B |= (1 << WGM12);
    TCCR1B |= (1 << CS12) | (1 << CS10);  // 1/1024
    TIMSK1 |= 1 << OCIE1A;

    Serial.begin(57600);
    Serial.println("Started");

    if (!bme.begin())
    {
        Serial.println("BME680 begin failed");
    }
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150);

    epd.Init(FULL);
    // delay(2000);
    epd.Clear();
    print_static();
    epd.DisplayFrame();
    epd.Sleep();

    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();
}

void loop()
{
    if (time_updated)
    {
        time_updated = 0;
        update_time();
        if (time_s == 0)
        {
            bme.performReading();
            data_update_handler();
            print_log();
        }
    }
    sleep_mode();
}
