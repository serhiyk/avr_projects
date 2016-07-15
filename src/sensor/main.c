#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include "UART.h"
#include "SPI.h"
#include "timer.h"
#include <stdint.h>
#include "serial.h"
#include "ds18b20.h"
#include "nrf24l01.h"
#include "onewire.h"

#define ONEWIRE_VDD_PORT PORTB
#define ONEWIRE_VDD_DDR DDRB
#define ONEWIRE_VDD_PIN PB1
#define ONEWIRE_GND_PORT PORTD
#define ONEWIRE_GND_DDR DDRD
#define ONEWIRE_GND_PIN PD7

uint8_t tx_address[5] = {0xD7,0xA1,0x17,0x0A,0xD7};
static uint8_t tx_buf[5];

void convert_temperature(void);

void setup(void)
{
    ONEWIRE_VDD_DDR = 1 << ONEWIRE_VDD_PIN;
    ONEWIRE_VDD_PORT = 1 << ONEWIRE_VDD_PIN;
    ONEWIRE_GND_DDR |= 1 << ONEWIRE_GND_PIN;
    ONEWIRE_GND_PORT &= ~(1 << ONEWIRE_GND_PIN);
    spi_master_init();
    serial_init();
    timer_init();
    onewire_init();
    asm("sei");
    //_delay_ms(5);
    nrf24l01_init();
    nrf24_set_tx_addr(tx_address, sizeof(tx_address));
    set_sleep_mode(SLEEP_MODE_IDLE);
}

void temperature_cb(uint16_t data)
{
    tx_buf[0] = data >> 8;
    tx_buf[1] = data & 0xFF;
    nrf24_send(tx_buf, sizeof(tx_buf));
    if (data & 0x0800)
    {
        data = ~data;
        data++;
        data |= 0x0800;
    }
    uart_send_hex(data>>4);
    uart_send_hex(data&0x0F);
}

void read_temperature(void)
{
    ds18b20_read_temperature(temperature_cb);
    timer_register(0, 10, convert_temperature);
}

void convert_temperature(void)
{
    ds18b20_convert();
    timer_register(0, 10, read_temperature);
}

int main(void)
{
    setup();
    // timer_register(0, 10, convert_temperature);
    convert_temperature();
    uart_send_byte('*');
    while(1)
    {
        serial_handler();
        nrf24_handler();
        timer_handler();
        sleep_mode();
    }
}
