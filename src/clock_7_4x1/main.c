#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include "ds18b20.h"
#include "UART.h"
#include "SPI.h"
#include "time_async.h"
#include "serial.h"
#include "neo7m.h"

#define DISPLAY_LE_PORT PORTB
#define DISPLAY_LE_DDR DDRB
#define DISPLAY_LE_PIN PB1
#define DISPLAY_LE_ON() (DISPLAY_LE_PORT |= 1 << DISPLAY_LE_PIN)
#define DISPLAY_LE_OFF() (DISPLAY_LE_PORT &= ~(1 << DISPLAY_LE_PIN))

#define GPS_POWER_PORT PORTC
#define GPS_POWER_DDR DDRC
#define GPS_POWER_PIN PC2
#define GPS_POWER_ON() (GPS_POWER_PORT |= 1 << GPS_POWER_PIN)
#define GPS_POWER_OFF() (GPS_POWER_PORT &= ~(1 << GPS_POWER_PIN))

#define BUZZER_PORT PORTD
#define BUZZER_DDR DDRD
#define BUZZER_PIN PD4
#define BUZZER_ON() (BUZZER_PORT |= 1 << BUZZER_PIN)
#define BUZZER_OFF() (BUZZER_PORT &= ~(1 << BUZZER_PIN))
#define BUZZER_SWITCH() (BUZZER_PORT ^= 1 << BUZZER_PIN)

#define ADC_PIN 1

#define RESISTOR1 47000UL
#define RESISTOR2 10000UL
#define VOLTAGE (256UL)  // 2.56V
#define ON_VOLTAGE_MIN (100)  // 10.0V
#define GPS_ON_VOLTAGE (100)  // 10.0V
#define GPS_OFF_VOLTAGE (80)  // 8.0V

typedef enum
{
    STATE_INIT = 0,
    STATE_SETUP,
    STATE_UPDATE,
    STATE_RUNNING
} state_enum_t;

static uint8_t display_map[2][12] = {
    {
        0b11111010,  // 0
        0b00110000,  // 1
        0b11011001,  // 2
        0b01111001,  // 3
        0b00110011,  // 4
        0b01101011,  // 5
        0b11101011,  // 6
        0b00111000,  // 7
        0b11111011,  // 8
        0b01111011,  // 9
        0b00000100,  // .
        0b00000001   // -
    },
    {
        0b11010111,  // 0
        0b10000001,  // 1
        0b11001110,  // 2
        0b11001011,  // 3
        0b10011001,  // 4
        0b01011011,  // 5
        0b01011111,  // 6
        0b11000001,  // 7
        0b11011111,  // 8
        0b11011011,  // 9
        0b00100000,  // .
        0b00001000   // -
    }
};

static uint8_t display_buf[4];
static volatile uint8_t state = STATE_INIT;
static volatile uint8_t vin;
static int16_t temperature;

static volatile uint8_t new_second;

static void display_cb(void)
{
    DISPLAY_LE_ON();
    DISPLAY_LE_OFF();
}

static void print_time(void)
{
    uint8_t h, m, s, h_h, m_h;
    time_async_get_hms(&h, &m, &s);
    h_h = h / 10;
    h -= h_h * 10;
    m_h = m / 10;
    m -= m_h * 10;
    display_buf[0] = display_map[0][m];
    display_buf[1] = display_map[1][m_h];
    display_buf[2] = display_map[0][h];
    if (h_h)
    {
        display_buf[3] = display_map[1][h_h];
    }
    else
    {
        display_buf[3] = 0;
    }
    if (s & 1 &&
        vin > 90)
    {
        display_buf[2] |= display_map[0][10];
    }
    spi_master_transfer(display_buf, NULL, 4, display_cb);
}

static void print_temperature(void)
{
    uint8_t temp_h, temp_l, sign=0;
    if (temperature < 0)
    {
        temperature *= -1;
        sign = 1;
    }
    temp_l = temperature >> 4;
    temp_h = temp_l / 10;
    temp_l -= temp_h * 10;

    display_buf[0] = display_map[0][temp_l];
    display_buf[1] = 0;
    display_buf[2] = 0;
    display_buf[3] = 0;
    if (temp_h)
    {
        display_buf[1] = display_map[1][temp_h];
        if (sign)
        {
            display_buf[2] = display_map[0][11];
        }
    }
    else
    {
        if (sign)
        {
            display_buf[1] = display_map[1][11];
        }

    }
    spi_master_transfer(display_buf, NULL, 4, display_cb);
}

uint8_t adc_read(void)
{
    // starts the conversion
    ADCSRA |= 1 << ADSC;

    while ((ADCSRA & (1 << ADSC)));

    uint32_t vin = ADCW;
    vin *= VOLTAGE * ((RESISTOR1 + RESISTOR2) / 10);
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
    // uart_send_hex(data>>8);
    // uart_send_hex(data&0xFF);
    // uart_send_byte('|');
}

static void _time_cb(void)
{
    GPS_POWER_OFF();
    uint8_t second = (get_second_bcd_h() * 10) + get_second_bcd_l();
    uint8_t minute = (get_minute_bcd_h() * 10) + get_minute_bcd_l();
    uint8_t hour = (get_hour_bcd_h() * 10) + get_hour_bcd_l();
    uint8_t dow = get_dow();
    uint8_t date = (get_date_bcd_h() * 10) + get_date_bcd_l();
    uint8_t month = get_month();
    uint16_t year = get_year();
    time_async_set(second, minute, hour, dow, date, month, year);
    state = STATE_RUNNING;
    uart_send_byte('|');
    uart_send_hex(hour);
    uart_send_hex(minute);
    uart_send_byte('|');
}

static void _update_time(void)
{
    uint8_t hour, minute, second;
    time_async_get_hms(&hour, &minute, &second);
    if ((second > 14 && second < 20) || (second > 44 && second < 50))
    {
        print_temperature();
    }
    else
    {
        print_time();
    }

    if (second == 45)
    {
        if (ds18b20_convert() != 0)
        {
            uart_send_byte('E');
            uart_send_byte('1');
            uart_send_byte('|');
        }
    }
    else if (second == 46)
    {
        if (ds18b20_read_temperature(temperature_cb) != 0)
        {
            uart_send_byte('E');
            uart_send_byte('2');
            uart_send_byte('|');
        }
    }

    if (minute == 0)
    {
        if (second == 0)
        {
            BUZZER_ON();
            _delay_ms(20);
            BUZZER_OFF();
        }
        else if (second == 1)
        {
            if (vin > GPS_ON_VOLTAGE)
            {
                GPS_POWER_ON();
                state = STATE_UPDATE;
            }
        }
    }
    else if (minute == 59)
    {
        if (second > 56)
        {
            BUZZER_ON();
            _delay_ms(10);
            BUZZER_OFF();
        }
    }

}

static void state_timer(void)
{
    vin = adc_read();
    uint8_t vin_h = vin / 10;

    // uart_send_byte('|');
    // if (vin_h < 10)
    // {
    //     uart_send_byte('0' + vin / 10);
    // }
    // else
    // {
    //     uart_send_byte('1');
    //     uart_send_byte('0' + vin_h - 10);
    // }
    // uart_send_byte('.');
    // uart_send_byte('0' + vin % 10);
    // uart_send_byte('|');
    // uart_send_byte('0' + state);
    // uart_send_byte('|');

    switch (state)
    {
        case STATE_INIT:
            if (vin > ON_VOLTAGE_MIN)
            {
                GPS_POWER_ON();
                state = STATE_SETUP;
            }
            break;
        case STATE_SETUP:
            if (vin < GPS_OFF_VOLTAGE)
            {
                GPS_POWER_OFF();
                state = STATE_INIT;
            }
            break;
        case STATE_UPDATE:
            if (vin < GPS_OFF_VOLTAGE)
            {
                GPS_POWER_OFF();
                state = STATE_RUNNING;
            }
        case STATE_RUNNING:
            _update_time();
            break;
    }
}

void adc_init(void)
{
    // Internal 2.56V
    ADMUX = ADC_PIN + (1 << REFS0) + (1 << REFS1);
    // DIDR0 = (1 << ADC0D);
    ADCSRA = (1 << ADEN);
}

static void second_cb(void)
{
    new_second = 1;
    // uart_send_byte('1');
}

void setup(void)
{
    DISPLAY_LE_DDR |= 1 << DISPLAY_LE_PIN;
    DISPLAY_LE_OFF();
    BUZZER_DDR |= 1 << BUZZER_PIN;
    BUZZER_OFF();
    GPS_POWER_DDR |= 1 << GPS_POWER_PIN;
    GPS_POWER_OFF();

    time_async_init(second_cb);
    adc_init();
    spi_master_init();
    neo7m_init(_time_cb);
    serial_init();
    //_delay_ms(5);
    asm("sei");

    spi_master_transfer(display_buf, NULL, 4, display_cb);
}

int main(void)
{
    setup();
    uart_send_byte('S');
    while(1)
    {
        if (new_second)
        {
            new_second = 0;
            state_timer();
        }
        neo7m_handler();
        sleep_enable();
        sleep_cpu();
    }
}
