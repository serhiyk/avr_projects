#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include "onewire.h"
#include "ds18b20.h"
#include "UART.h"
#include "SPI.h"
#include "timer.h"
#include "max7219.h"
#include "display.h"

#define TEMPERATURE_TIMER_ID 0
#define GPS_TIMEOUT_TIMER_ID 1

int16_t temperature;

void convert_temperature(void);

void temperature_cb(uint16_t data)
{
    temperature = data;
    if (data & 0x0800)
    {
        temperature |= 0xF000;
    }
    uart_send_hex(data>>8);
    uart_send_hex(data&0xFF);
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
    print_screensaver();
}

static void _time_cb(void)
{
    uint8_t second = (get_second_bcd_h() << 4) | get_second_bcd_l();
    if ((second > 0x14 && second < 0x20) || (second > 0x44 && second < 0x50))
    {
        print_ext_temperature(temperature);
    }
    else
    {
        time_update_handler();
    }
    timer_register(GPS_TIMEOUT_TIMER_ID, 50, gps_timeout);
}

void setup(void)
{
    spi_master_init();
    timer_init();
    onewire_init();
    asm("sei");
    neo7m_init(_time_cb);
    //_delay_ms(5);
    display_init();
}

int main(void)
{
    setup();
    uart_send_byte('S');
    convert_temperature();
    while(1)
    {
        max7219_handler();
        neo7m_handler();
        timer_handler();
    }
}
