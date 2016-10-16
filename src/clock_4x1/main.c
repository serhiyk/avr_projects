#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include "onewire.h"
#include "ds18b20.h"
#include "DS3231.h"
#include "UART.h"
#include "SPI.h"
#include "TWI.h"
#include "timer.h"
#include "serial.h"
#include "max7219.h"
#include "display.h"

#define DISPLAY_TIMER_ID 0
#define TEMPERATURE_TIMER_ID 1

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

void display_handler(void)
{
    uint8_t second = get_second_bcd();
    if ((second > 0x14 && second < 0x20) || (second > 0x44 && second < 0x50))
    {
        print_ext_temperature(temperature);
    }
    else
    {
        time_update_handler();
    }
}

void setup(void)
{
    twi_init();
    spi_master_init();
    serial_init();
    timer_init();
    onewire_init();
    asm("sei");
    ds3231_init();
    //_delay_ms(5);
    display_init();
}

int main(void)
{
    setup();
    uart_send_byte('S');
    // set_time(0,11,21,3,22,3,16);
    convert_temperature();
    timer_register(DISPLAY_TIMER_ID, 10, ds3231_read);
    while(1)
    {
        ds3231_handler(display_handler);
        max7219_handler();
        serial_handler();
        timer_handler();
    }
}
