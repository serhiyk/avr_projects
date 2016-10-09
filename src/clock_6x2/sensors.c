#include <stdint.h>
#include "nrf24l01.h"
#include "UART.h"
#include "timer.h"
#include "display.h"
#include "sensors.h"

#define REMOTE_SENSOR_TIMER_ID 2
#define REMOTE_SENSOR_TIMER_PERIOD 200
#define REMOTE_SENSOR_TIMEOUT_S 600

static uint8_t rx_address[5] = {0xD7,0xA1,0x17,0x0A,0xD7};
static uint8_t sensor_timer=0;

void external_temperature_cb(uint8_t *buf)
{
    if (buf[0] & 0x08)
    {
        buf[0] |= 0xF0;
    }
    print_ext_temperature((buf[0] << 8) | buf[1]);
    uart_send_hex(buf[0]);
    uart_send_hex(buf[1]);
    sensor_timer = REMOTE_SENSOR_TIMEOUT_S * 10 / REMOTE_SENSOR_TIMER_PERIOD;
}

void remote_sensor_handler(void)
{
    if (sensor_timer == 0)
    {
        return;
    }
    sensor_timer--;
    if (sensor_timer == 0)
    {
        clear_ext_temperature();
    }
}

void sensors_init(void)
{
    nrf24_register_cb(1, rx_address, sizeof(rx_address), 5, external_temperature_cb);
    timer_register(REMOTE_SENSOR_TIMER_ID, REMOTE_SENSOR_TIMER_PERIOD, remote_sensor_handler);
}
