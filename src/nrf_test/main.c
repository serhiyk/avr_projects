#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include "DS3231.h"
#include "UART.h"
#include "timer.h"
#include <stdint.h>
#include "serial.h"
#include "nrf24l01.h"

// uint8_t tx_address[5] = {0xD7,0xD7,0xD7,0xD7,0xD7};
uint8_t tx_address[5] = {0xE7,0xE7,0xE7,0xE7,0xE7};
uint8_t rx_address[5] = {0xE7,0xE7,0xE7,0xE7,0xE7};

void nrf_test_cb(uint8_t *buf)
{
    uart_send_byte(buf[0]);
    uart_send_byte(buf[1]);
    uart_send_byte(buf[2]);
    uart_send_byte(buf[3]);
}

void setup(void)
{
    spi_master_init();
    serial_init();
    timer_init();
    asm("sei");
    //_delay_ms(5);
    nrf24l01_init();
    nrf24_register_cb(1, rx_address, sizeof(rx_address), 4, nrf_test_cb);
    nrf24_set_tx_addr(tx_address, 5);
}

void test_nrf_send(void)
{
    static uint8_t i=0;
    static uint8_t buf[4];
    uart_send_byte('S');
    buf[0] = 0x30 + ((i >> 6) & 0x03);
    buf[1] = 0x30 + ((i >> 4) & 0x03);
    buf[2] = 0x30 + ((i >> 2) & 0x03);
    buf[3] = 0x30 + (i & 0x03);
    nrf24_send(buf, 4);
    i++;
}

int main(void)
{
    setup();
    timer_register(0, 30, test_nrf_send);
    uart_send_byte('*');
    while(1)
    {
        serial_handler();
        nrf24_handler();
        timer_handler();
    }
}
