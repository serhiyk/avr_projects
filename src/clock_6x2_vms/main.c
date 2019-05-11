#include "display.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "onewire.h"
#include "ds18b20.h"
#include "bmp180.h"
#include "UART.h"
#include "SPI.h"
#include "TWI.h"
#include "timer.h"
#include <stdint.h>
#include "neo7m.h"
#include "max7219.h"

#define DISPLAY_ENABLED

#define TEMPERATURE_TIMER_ID 1
#define GPS_TIMEOUT_TIMER_ID 2

int16_t temperature;

void convert_temperature(void);

void print_temperature(void)
{
    if (temperature == DS18B20_INVALID_TEMPERATURE)
    {
        clear_ext_temperature();
    }
    else
    {
        if (temperature & 0x0800)
        {
            temperature |= 0xF000;
        }
        print_ext_temperature(temperature);
    }
    timer_register(TEMPERATURE_TIMER_ID, 200, convert_temperature);
}

void temperature_cb(uint16_t data)
{
    temperature = data;
    timer_register(TEMPERATURE_TIMER_ID, 10, print_temperature);
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

static void gps_timeout(void)
{
    timer_stop(GPS_TIMEOUT_TIMER_ID);
    display_activate();
}

static void _time_cb(void)
{
    time_update_handler();
    timer_register(GPS_TIMEOUT_TIMER_ID, 50, gps_timeout);
}

void setup(void)
{
    twi_init();
    spi_master_init();
    timer_init();
    onewire_init();
    asm("sei");
    neo7m_init(_time_cb);
    bmp180_init(print_int_temperature, print_pressure);
    //_delay_ms(5);
#ifdef DISPLAY_ENABLED
    display_init();
#endif

    asm("sei");

    display_activate();
}

int main(void)
{
    setup();
    uart_send_byte('S');
    convert_temperature();
    while(1)
    {
#ifdef DISPLAY_ENABLED
        max7219_handler();
#endif
        neo7m_handler();
        bmp180_handler();
        timer_handler();
    }
}
