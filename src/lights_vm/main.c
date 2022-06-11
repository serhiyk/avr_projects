#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include "ws2812b.h"
#include "UART.h"
#include "timer.h"
#include "serial.h"

#define ALS_PIN (PINC & (1 << PC1))
#define MOTION_PIN (PINC & (1 << PC2))
#define LED_TIMER_ID 0
#define LED_NUMBER (82+1)
#define LED_LEVEL_MAX 0x2F
#define LED_WHITE_MIN_TIMEOUT_SEC 60
#define LED_WHITE_MAX_TIMEOUT_SEC (60*10)
#define LED_RED_TIMEOUT_SEC (60*1)

typedef enum
{
    LED_STATE_INIT = 0,
    LED_STATE_TO_WHITE,
    LED_STATE_TO_RED,
    LED_STATE_TO_OFF,
    LED_STATE_WHITE,
    LED_STATE_RED
} led_state_t;

static ws2812b_data_t led_buf[LED_NUMBER];
static uint8_t led_state = LED_STATE_INIT;

static void led_handler(void)
{
    uint8_t done=1;
    switch (led_state)
    {
        case LED_STATE_TO_WHITE:
            _delay_ms(1);
            for (uint8_t i=0; i<LED_NUMBER; i++)
            {
                if (led_buf[i].red < LED_LEVEL_MAX)
                {
                    led_buf[i].red++;
                    done = 0;
                }
                if (led_buf[i].green < LED_LEVEL_MAX)
                {
                    led_buf[i].green++;
                    done = 0;
                }
                if (led_buf[i].blue < LED_LEVEL_MAX)
                {
                    led_buf[i].blue++;
                    done = 0;
                }
            }
            if (done)
            {
                led_state = LED_STATE_WHITE;
            }
            break;

        case LED_STATE_TO_RED:
            _delay_ms(20);
            for (uint8_t i=0; i<LED_NUMBER; i++)
            {
                if (led_buf[i].red < LED_LEVEL_MAX)
                {
                    led_buf[i].red++;
                    done = 0;
                }
                if (led_buf[i].green > 0)
                {
                    led_buf[i].green--;
                    done = 0;
                }
                if (led_buf[i].blue > 0)
                {
                    led_buf[i].blue--;
                    done = 0;
                }
            }
            if (done)
            {
                led_state = LED_STATE_RED;
            }
            break;

        case LED_STATE_TO_OFF:
            _delay_ms(20);
            for (uint8_t i=0; i<LED_NUMBER; i++)
            {
                if (led_buf[i].red > 0)
                {
                    led_buf[i].red--;
                    done = 0;
                }
                if (led_buf[i].green > 0)
                {
                    led_buf[i].green--;
                    done = 0;
                }
                if (led_buf[i].blue > 0)
                {
                    led_buf[i].blue--;
                    done = 0;
                }
            }
            if (done)
            {
                led_state = LED_STATE_INIT;
            }
            break;

        default:
            return;
    }
    ws2812b_send();
}

static void led_timer_handler(void)
{
    static uint16_t timeout = 0;
    if (MOTION_PIN)
    {
        if (timeout == 0)
        {
            if (ALS_PIN)
            {
                timeout = LED_WHITE_MIN_TIMEOUT_SEC;
                led_state = LED_STATE_TO_WHITE;
            }
        }
        else
        {
            if (led_state == LED_STATE_WHITE)
            {
                if (timeout < LED_WHITE_MAX_TIMEOUT_SEC)
                {
                    timeout++;
                }
            }
            else if (led_state == LED_STATE_RED)
            {
                timeout = LED_WHITE_MIN_TIMEOUT_SEC;
                led_state = LED_STATE_TO_WHITE;
            }
        }
    }
    else if (timeout > 0)
    {
        timeout--;
        if (timeout == 0)
        {
            if (led_state == LED_STATE_RED)
            {
                led_state = LED_STATE_TO_OFF;
            }
            else if (led_state == LED_STATE_WHITE)
            {
                timeout = LED_RED_TIMEOUT_SEC;
                led_state = LED_STATE_TO_RED;
            }
        }
    }
    uart_send_byte(ALS_PIN ? '1' : '0');
    uart_send_byte(MOTION_PIN ? '1' : '0');
    uart_send_byte('0'+led_state);
    uart_send_hex(timeout);
    uart_send_byte('|');
}

void setup(void)
{
    serial_init();
    timer_init();
    ws2812b_init(led_buf, LED_NUMBER);
    asm("sei");
    for (uint8_t i=0; i<LED_NUMBER; i++)
    {
        for (uint8_t j=0; j<LED_NUMBER; j++)
        {
            led_buf[j].red = 0;
            led_buf[j].green = 0;
            led_buf[j].blue = 0;
        }
        led_buf[i].red = LED_LEVEL_MAX;
        led_buf[i].green = LED_LEVEL_MAX;
        led_buf[i].blue = LED_LEVEL_MAX;
        ws2812b_send();
        _delay_ms(20);
    }
}

int main(void)
{
    setup();
    uart_send_byte('S');
    timer_register(LED_TIMER_ID, 10, led_timer_handler);
    set_sleep_mode(SLEEP_MODE_IDLE);
    while(1)
    {
        serial_handler();
        timer_handler();
        led_handler();
        sleep_cpu();
    }
}
