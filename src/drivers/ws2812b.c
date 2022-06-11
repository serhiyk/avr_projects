#include <avr/io.h>
#include <avr/interrupt.h>
#include "config.h"
#include "ws2812b.h"

#if !defined(WS2812B_PORT)
    #error WS2812B_PORT is not defined
#endif

#if !defined(WS2812B_DDR)
    #error WS2812B_DDR is not defined
#endif

#if !defined(WS2812B_PIN)
    #error WS2812B_PIN is not defined
#endif

#define NOP __asm__("nop\n\t")

static volatile uint8_t *ws2812b_buf;
static volatile uint8_t *ws2812b_buf_end;

void ws2812b_init(ws2812b_data_t *buf, int leds)
{
    WS2812B_DDR |= 1 << WS2812B_PIN;
    WS2812B_PORT &= ~(1 << WS2812B_PIN);
    ws2812b_buf = (uint8_t *)buf;
    ws2812b_buf_end = (uint8_t *)buf + leds * sizeof(ws2812b_data_t);
}

void ws2812b_send(void)
{
    cli();
    register uint8_t *p = ws2812b_buf;
    register uint8_t *e = ws2812b_buf_end;
    // uint8_t *p = ws2812b_buf;
    // uint8_t *e = ws2812b_buf_end;
    register uint8_t b;
    while(p != e)
    {//PORTB ^= 1 << PB4;
        b = *p++;    // Current byte value
        // *p++;
        uint8_t i=8;
        do
        {
            if ((b & 0x80) == 0)
            {
                // Send a '0'
                if (F_CPU == 16000000L)
                {
                    WS2812B_PORT |= 1 << WS2812B_PIN;
                    NOP;
                    NOP;
                    WS2812B_PORT &= ~(1 << WS2812B_PIN);
                    NOP;// Lo (250ns)
                    NOP;NOP;            // Lo
                    NOP;NOP;            // Lo (500ns)
                }
                // else if (F_CPU == 8000000L)
                // {
                //     LED_PIN = LED_BIT;  // Hi (start)
                //     NOP;                // Hi
                //     LED_PIN = LED_BIT;  // Lo (250ns)
                //     NOP;                // Lo
                //     NOP;                // Lo (500ns)
                //     NOP;                // Lo (data bit here!)
                //     NOP;                // Lo (750ns)
                // }
            }
            else
            {
                // Send a '1'
                if (F_CPU == 16000000L)
                {
                    WS2812B_PORT |= 1 << WS2812B_PIN;
                    //NOP;// Hi (start)
                    NOP;NOP;            // Hi
                    NOP;NOP;            // Hi (250ns)
                    NOP;NOP;            // Hi
                    NOP;NOP;            // Hi (500ns)
                    WS2812B_PORT &= ~(1 << WS2812B_PIN);
                        // Lo (625ns)
                }
                // else if (F_CPU == 8000000L)
                // {
                //     LED_PIN = LED_BIT;  // Hi (start)
                //     NOP;                // Hi
                //     NOP;                // Hi (250ns)
                //     NOP;                // Hi
                //     NOP;                // Hi (500ns)
                //     NOP;                // Hi (data bit here!)
                //     LED_PIN = LED_BIT;  // Lo (750ns)
                // }
            }
            b = b + b;
        } while (--i != 0);
    }
    sei();
}
