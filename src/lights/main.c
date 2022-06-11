#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include "ws2812b.h"
#include "UART.h"
#include "timer.h"
#include "serial.h"

#define LED_TIMER_ID 0
#define LED_NUMBER 100
#define LED_BRIGHTNESS 1
#define COLOR_LIST_SIZE 8

static ws2812b_data_t color_list[COLOR_LIST_SIZE] = {
    {.red=0xFF>>LED_BRIGHTNESS, .green=0x00>>LED_BRIGHTNESS, .blue=0x00>>LED_BRIGHTNESS},  // RED
    {.red=0x00>>LED_BRIGHTNESS, .green=0xFF>>LED_BRIGHTNESS, .blue=0x00>>LED_BRIGHTNESS},  // GREEN
    {.red=0x00>>LED_BRIGHTNESS, .green=0x00>>LED_BRIGHTNESS, .blue=0xFF>>LED_BRIGHTNESS},  // BLUE
    {.red=0xFF>>LED_BRIGHTNESS, .green=0xFF>>LED_BRIGHTNESS, .blue=0x00>>LED_BRIGHTNESS},  // YELLOW
    {.red=0x00>>LED_BRIGHTNESS, .green=0xFF>>LED_BRIGHTNESS, .blue=0xFF>>LED_BRIGHTNESS},  // CYAN
    {.red=0xFF>>LED_BRIGHTNESS, .green=0x00>>LED_BRIGHTNESS, .blue=0xFF>>LED_BRIGHTNESS},  // MAGENTA
    {.red=0x40>>LED_BRIGHTNESS, .green=0x00>>LED_BRIGHTNESS, .blue=0x80>>LED_BRIGHTNESS},  // PURPLE
    {.red=0xFF>>LED_BRIGHTNESS, .green=0x30>>LED_BRIGHTNESS, .blue=0x00>>LED_BRIGHTNESS}}; // ORANGE

static ws2812b_data_t led_buf[LED_NUMBER];

void led1(void)
{
    uint16_t t;
    for (int i=0; i<LED_NUMBER; i++)
    {
        if (led_buf[i].red | led_buf[i].green | led_buf[i].blue)
        {
            t = led_buf[i].red * 255;
            led_buf[i].red = t >> 8;
            t = led_buf[i].green * 255;
            led_buf[i].green = t >> 8;
            t = led_buf[i].blue * 255;
            led_buf[i].blue = t >> 8;
        }
    }
    int new_led = rand() & 0xFFF;
    if (new_led < LED_NUMBER && (led_buf[new_led].red | led_buf[new_led].green | led_buf[new_led].blue) == 0)
    {
            uint8_t new_color_index = rand() & (COLOR_LIST_SIZE - 1);
            led_buf[new_led].red = color_list[new_color_index].red;
            led_buf[new_led].green = color_list[new_color_index].green;
            led_buf[new_led].blue = color_list[new_color_index].blue;
    }
    ws2812b_send();
    _delay_ms(10);
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
    for (int i=0; i<LED_NUMBER; i++)
    {
        led_buf[i].red = 0x07;
        led_buf[i].green = 0;
        led_buf[i].blue = 0;
    }
    // timer_register(LED_TIMER_ID, 1, led1);
    while(1)
    {
        serial_handler();
        timer_handler();
        led1();
        // ws2812b_send();
    }
}
