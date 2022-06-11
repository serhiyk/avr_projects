#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include "onewire.h"
#include "ds18b20.h"
#include "UART.h"
#include "SPI.h"
#include "timer.h"
#include "max7219.h"
#include "display.h"
#include "serial.h"
#include "neo7m.h"

#define GPS_POWER_PORT PORTB
#define GPS_POWER_DDR DDRB
#define GPS_POWER_PIN PB0
#define GPS_POWER_ON() (GPS_POWER_PORT |= 1 << GPS_POWER_PIN)
#define GPS_POWER_OFF() (GPS_POWER_PORT &= ~(1 << GPS_POWER_PIN))

#define LED_POWER_PORT PORTB
#define LED_POWER_DDR DDRB
#define LED_POWER_PIN PB5
#define LED_POWER_ON() (LED_POWER_PORT |= 1 << LED_POWER_PIN)
#define LED_POWER_OFF() (LED_POWER_PORT &= ~(1 << LED_POWER_PIN))

#define LCD_POWER_PORT PORTC
#define LCD_POWER_DDR DDRC
#define LCD_POWER_PIN PC1
#define LCD_POWER_ON() (LCD_POWER_PORT |= 1 << LCD_POWER_PIN)
#define LCD_POWER_OFF() (LCD_POWER_PORT &= ~(1 << LCD_POWER_PIN))

#define ADC_PIN 0

#define RESISTOR1 99300UL
#define RESISTOR2 6500UL
#define VOLTAGE (11UL)  // 1.1V
#define ON_VOLTAGE_MIN (70)  // 7.0V
#define GPS_ON_VOLTAGE (123)  // 10.0V
#define GPS_OFF_VOLTAGE (111)  // 8.0V
#define TIME_MIN_VOLTAGE (50)  // 5.0V

#define TEMPERATURE_TIMER_ID 0
#define SECOND_TIMER_ID 1
#define SECOND_TIMER_PERIOD 10  // 1 second

#define STARTUP_DELAY_SEC 5

typedef enum
{
    STATE_INIT = 0,
    STATE_SETUP,
    STATE_RUNNING
} state_enum_t;

static volatile uint8_t state = STATE_INIT;
static volatile uint8_t vin;
static int16_t temperature;
static uint8_t second_bcd_l;
static uint8_t second_bcd_h;
static uint8_t minute_bcd_l;
static uint8_t minute_bcd_h;
static uint8_t hour_bcd_l;
static uint8_t hour_bcd_h;

void convert_temperature(void);

uint8_t adc_read(void)
{
    // starts the conversion
    ADCSRA |= 1 << ADSC;

    while ((ADCSRA & (1 << ADSC)));

    uint32_t vin = ADCW;
    vin *= VOLTAGE * (RESISTOR1 + RESISTOR2);
    vin /= 1024UL * RESISTOR2;
    return vin;
}

void temperature_cb(uint16_t data)
{
    temperature = data;
    if (data & 0x0800)
    {
        temperature |= 0xF000;
    }
    uart_send_hex(data>>8);
    uart_send_hex(data&0xFF);
    uart_send_byte('|');
}

void read_temperature(void)
{
    ds18b20_read_temperature(temperature_cb);
    timer_register(TEMPERATURE_TIMER_ID, 200, convert_temperature);
}

void convert_temperature(void)
{
    ds18b20_convert();
    timer_register(TEMPERATURE_TIMER_ID, 10, read_temperature);
}

static void _time_cb(void)
{
    second_bcd_l = get_second_bcd_l();
    second_bcd_h = get_second_bcd_h();
    minute_bcd_l = get_minute_bcd_l();
    minute_bcd_h = get_minute_bcd_h();
    hour_bcd_l = get_hour_bcd_l();
    hour_bcd_h = get_hour_bcd_h();
    GPS_POWER_OFF();
}

static void _update_time(void)
{
    second_bcd_l++;
    if (second_bcd_l == 10)
    {
        second_bcd_l = 0;
        second_bcd_h++;
        if (second_bcd_h == 6)
        {
            second_bcd_h = 0;
            minute_bcd_l++;
            if (minute_bcd_l == 10)
            {
                minute_bcd_l = 0;
                minute_bcd_h++;
                if (minute_bcd_h == 6)
                {
                    minute_bcd_h = 0;
                    hour_bcd_l++;
                    if (hour_bcd_l == 10)
                    {
                        hour_bcd_l = 0;
                        hour_bcd_h++;
                    }
                    if (hour_bcd_h == 2 && hour_bcd_l == 4)
                    {
                        hour_bcd_h = 0;
                        hour_bcd_l = 0;
                    }
                }
            }
        }
    }
    uint8_t second = (second_bcd_h << 4) | second_bcd_l;
    if ((second > 0x14 && second < 0x20) || (second > 0x44 && second < 0x50) || (vin < TIME_MIN_VOLTAGE))
    {
        print_ext_temperature(temperature);
    }
    else
    {
        time_update_handler(hour_bcd_h, hour_bcd_l, minute_bcd_h, minute_bcd_l);
    }
    // print_screensaver();
}

void state_timer(void)
{
    vin = adc_read();
    uint8_t vin_h = vin / 10;
    if (vin_h < 10)
    {
        uart_send_byte('0' + vin / 10);
    }
    else
    {
        uart_send_byte('1');
        uart_send_byte('0' + vin_h - 10);
    }
    uart_send_byte('.');
    uart_send_byte('0' + vin % 10);
    uart_send_byte('-');
    uart_send_byte('0' + state);
    uart_send_byte('|');

    switch (state)
    {
        case STATE_INIT:
            if (vin < ON_VOLTAGE_MIN)
            {
                second_bcd_l = 0;
            }
            else if (second_bcd_l > STARTUP_DELAY_SEC)
            {
                LED_POWER_OFF();
                second_bcd_l = 0;
                uart_update_baudrate(9600);
                GPS_POWER_ON();
                state = STATE_SETUP;
            }
            else
            {
                second_bcd_l++;
            }
            break;
        case STATE_SETUP:
            // print_ext_temperature(temperature);
            if (second_bcd_l)
            {
                if (vin < GPS_ON_VOLTAGE)
                {
                    GPS_POWER_OFF();
                }
                LCD_POWER_ON();
                _delay_ms(10);
                display_init();
                state = STATE_RUNNING;
            }
            break;
        case STATE_RUNNING:
            // if (vin < GPS_OFF_VOLTAGE)
            // {
            //     GPS_POWER_OFF();
            // }
            // else
            if (vin > GPS_ON_VOLTAGE &&
                second_bcd_l == 0 &&
                second_bcd_h == 0 &&
                minute_bcd_l == 0 &&
                minute_bcd_h == 0)
            {
                GPS_POWER_ON();
            }
            _update_time();
            break;
    }
}

void adc_init(void)
{
    // Internal 1.1V
    ADMUX = ADC_PIN + (1 << REFS0) + (1 << REFS1);
    DIDR0 = (1 << ADC0D);
    ADCSRA = (1 << ADEN);
}

void setup(void)
{
    GPS_POWER_DDR |= 1 << GPS_POWER_PIN;
    GPS_POWER_OFF();
    LCD_POWER_DDR |= 1 << LCD_POWER_PIN;
    LCD_POWER_OFF();
    LED_POWER_DDR |= 1 << LED_POWER_PIN;
    LED_POWER_ON();
    adc_init();
    // spi_master_init();
    timer_init();
    onewire_init();
    asm("sei");
    neo7m_init(_time_cb);
    serial_init();
    //_delay_ms(5);
    // display_init();
}

int main(void)
{
    setup();
    uart_send_byte('S');
    timer_register(SECOND_TIMER_ID, SECOND_TIMER_PERIOD, state_timer);
    convert_temperature();
    while(1)
    {
        if (state == STATE_INIT)
        {
            serial_handler();
        }
        else
        {
            max7219_handler();
            neo7m_handler();
        }
        timer_handler();
        sleep_enable();
        sleep_cpu();
    }
}
