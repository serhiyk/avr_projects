#include <avr/io.h>
#include <stdint.h>
#include "SPI.h"
#include "config.h"
#include "max7219.h"

#if !defined(MAX7219_NUMBER)
    #error MAX7219_NUMBER is not defined
#endif

#if !defined(MAX7219_CS_PORT)
    #error MAX7219_CS_PORT is not defined
#endif

#if !defined(MAX7219_CS_DDR)
    #error MAX7219_CS_DDR is not defined
#endif

#if !defined(MAX7219_CS_PIN)
    #error MAX7219_CS_PIN is not defined
#endif

#define MAX7219_REG_NOOP 0x00
#define MAX7219_REG_DIGIT0 0x01
#define MAX7219_REG_DIGIT1 0x02
#define MAX7219_REG_DIGIT2 0x03
#define MAX7219_REG_DIGIT3 0x04
#define MAX7219_REG_DIGIT4 0x05
#define MAX7219_REG_DIGIT5 0x06
#define MAX7219_REG_DIGIT6 0x07
#define MAX7219_REG_DIGIT7 0x08
#define MAX7219_REG_DECODE_MODE 0x09
#define MAX7219_REG_INTENSITY 0x0A
#define MAX7219_REG_SCAN_LIMIT 0x0B
#define MAX7219_REG_SHUTDOWN 0x0C
#define MAX7219_REG_DISPLAY_TEST 0x0F

#define MAX7219_REG_DECODE_MODE_VALUE 0x00  // using an led matrix (not digits)
#define MAX7219_REG_INTENSITY_VALUE 0x01    // the first 0x0f is the value you can set range: 0x00 to 0x0f
#define MAX7219_REG_SCAN_LIMIT_VALUE 0x07
#define MAX7219_REG_SHUTDOWN_VALUE 0x01     // not in shutdown mode
#define MAX7219_REG_DISPLAY_TEST_VALUE 0x00 // no display test

static uint8_t spi_buf[MAX7219_NUMBER*2];
static uint8_t max7219_row;

extern void max7219_load_row(uint8_t row, uint8_t *buf);

static void max7219_cs_cb(void)
{
    MAX7219_CS_PORT &= ~(1 << MAX7219_CS_PIN);
    MAX7219_CS_PORT |= 1 << MAX7219_CS_PIN;
}

static void max7219_send_all(uint8_t reg, uint8_t data)
{
    uint8_t *buf=spi_buf;
    for (uint8_t i=0; i<MAX7219_NUMBER; i++)
    {
        *buf++ = reg;
        *buf++ = data;
    }
    spi_master_send(spi_buf, MAX7219_NUMBER*2, max7219_cs_cb);
}

void max7219_init(void)
{
    MAX7219_CS_DDR |= 1 << MAX7219_CS_PIN;
    max7219_send_all(MAX7219_REG_SCAN_LIMIT, MAX7219_REG_SCAN_LIMIT_VALUE);
    while (!spi_ready());
    max7219_send_all(MAX7219_REG_DECODE_MODE, MAX7219_REG_DECODE_MODE_VALUE);
    while (!spi_ready());
    max7219_send_all(MAX7219_REG_SHUTDOWN, MAX7219_REG_SHUTDOWN_VALUE);
    while (!spi_ready());
    max7219_send_all(MAX7219_REG_DISPLAY_TEST, MAX7219_REG_DISPLAY_TEST_VALUE);
    while (!spi_ready());
    max7219_send_all(MAX7219_REG_INTENSITY, 0x0f & MAX7219_REG_INTENSITY_VALUE);
    while (!spi_ready());
}

void max7219_update(void)
{
    max7219_row = MAX7219_ROWS;
}

void max7219_update_with_config(void)
{
    max7219_row = MAX7219_REG_DISPLAY_TEST + 1;
}

void max7219_handler(void)
{
    if (max7219_row == 0)
    {
        return;
    }
    if (!spi_ready())
    {
        return;
    }
    max7219_row--;
    if (max7219_row < MAX7219_ROWS)
    {
        max7219_load_row(max7219_row, spi_buf);
        spi_master_send(spi_buf, MAX7219_NUMBER*2, max7219_cs_cb);
    }
    else
    {
        uint8_t data;
        switch (max7219_row)
        {
            case MAX7219_REG_SCAN_LIMIT:
                data = MAX7219_REG_SCAN_LIMIT_VALUE;
                break;
            case MAX7219_REG_DECODE_MODE:
                data = MAX7219_REG_DECODE_MODE_VALUE;
                break;
            case MAX7219_REG_SHUTDOWN:
                data = MAX7219_REG_SHUTDOWN_VALUE;
                break;
            case MAX7219_REG_DISPLAY_TEST:
                data = MAX7219_REG_DISPLAY_TEST_VALUE;
                break;
            case MAX7219_REG_INTENSITY:
                data = MAX7219_REG_INTENSITY_VALUE;
                break;
            default:
                return;
        }
        max7219_send_all(max7219_row, data);
    }
}
