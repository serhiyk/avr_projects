#include <stdint.h>
#include "nrf24l01.h"
#include "ds18b20.h"
#include "UART.h"
#include "sensors.h"

static uint8_t rx_address[5] = {0xD7,0xA1,0x17,0x0A,0xD7};
static uint16_t _temperature=DS18B20_INVALID_TEMPERATURE;

void external_temperature_cb(uint8_t *buf)
{
    // uart_send_byte(buf[0]);
    // uart_send_byte(buf[1]);
    // uart_send_byte(buf[2]);
    // uart_send_byte(buf[3]);
    uart_send_hex(buf[0]);
    uart_send_hex(buf[1]);
    _temperature = (buf[0] << 8) | buf[1];
}

void sensors_init(void)
{
	nrf24_register_cb(1, rx_address, sizeof(rx_address), 5, external_temperature_cb);
}

uint16_t sensors_get_temperature(void)
{
	return _temperature;
}
