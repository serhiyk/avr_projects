#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include "ws2812b.h"
#include "UART.h"
#include "timer.h"
#include "serial.h"

#define MOTION_PIN (PIND & (1 << PD6))
#define LED_TIMER_ID 0
#define LED_NUMBER 100
#define LED_LEVEL_MAX 0x7F
#define LED_WHITE_MIN_TIMEOUT_SEC 60
#define LED_WHITE_MAX_TIMEOUT_SEC (60*10)
#define LED_RED_TIMEOUT_SEC (60*60)

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
            for (int i=0; i<LED_NUMBER; i++)
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
            for (int i=0; i<LED_NUMBER; i++)
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
            for (int i=0; i<LED_NUMBER; i++)
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
    if (done)
    {
        led_state = LED_STATE_INIT;
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
            timeout = LED_WHITE_MIN_TIMEOUT_SEC;
            led_state = LED_STATE_TO_WHITE;
        }
        else
        {
            if ((led_state == LED_STATE_RED && timeout < LED_RED_TIMEOUT_SEC) ||
                (led_state == LED_STATE_WHITE && timeout < LED_WHITE_MAX_TIMEOUT_SEC))
            {
                timeout++;
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
}

void setup(void)
{
    serial_init();
    timer_init();
    ws2812b_init(led_buf, LED_NUMBER);
    asm("sei");
}

int main(void)
{
    setup();
    uart_send_byte('S');
    timer_register(LED_TIMER_ID, 10, led_timer_handler);
    while(1)
    {
        serial_handler();
        timer_handler();
        led_handler();
    }
}
