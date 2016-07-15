#include <stdint.h>
#include "nrf24l01.h"
#include "ds18b20.h"
#include "UART.h"
#include "timer.h"
#include "sensors.h"

#define REMOTE_SENSOR_TIMER_ID 2
#define REMOTE_SENSOR_TIMER_PERIOD 200
#define REMOTE_SENSOR_TIMEOUT_S 600

static uint8_t rx_address[5] = {0xD7,0xA1,0x17,0x0A,0xD7};
static uint16_t _temperature=DS18B20_INVALID_TEMPERATURE;
static uint8_t sensor_time=0;

void external_temperature_cb(uint8_t *buf)
{
    // uart_send_byte(buf[0]);
    // uart_send_byte(buf[1]);
    // uart_send_byte(buf[2]);
    // uart_send_byte(buf[3]);
    uart_send_hex(buf[0]);
    uart_send_hex(buf[1]);
    _temperature = (buf[0] << 8) | buf[1];
    sensor_time = 0;
}

void remote_sensor_handler(void)
{
    if (sensor_time > (REMOTE_SENSOR_TIMEOUT_S * 10 / REMOTE_SENSOR_TIMER_PERIOD))
    {
        return;
    }
    else if (sensor_time == (REMOTE_SENSOR_TIMEOUT_S * 10 / REMOTE_SENSOR_TIMER_PERIOD))
    {
        _temperature=DS18B20_INVALID_TEMPERATURE;
    }
    sensor_time++;
}

void sensors_init(void)
{
    nrf24_register_cb(1, rx_address, sizeof(rx_address), 5, external_temperature_cb);
    timer_register(REMOTE_SENSOR_TIMER_ID, REMOTE_SENSOR_TIMER_PERIOD, remote_sensor_handler);
}

uint16_t sensors_get_temperature(void)
{
    return _temperature;
}
