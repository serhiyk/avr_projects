#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "SPI.h"
#include "epaper217.h"
#include "config.h"

#if !defined(EPAPER_RST_PORT)
    #error EPAPER_RST_PORT is not defined
#endif

#if !defined(EPAPER_RST_DDR)
    #error EPAPER_RST_DDR is not defined
#endif

#if !defined(EPAPER_RST_PIN)
    #error EPAPER_RST_PIN is not defined
#endif

#if !defined(EPAPER_DC_PORT)
    #error EPAPER_DC_PORT is not defined
#endif

#if !defined(EPAPER_DC_DDR)
    #error EPAPER_DC_DDR is not defined
#endif

#if !defined(EPAPER_DC_PIN)
    #error EPAPER_DC_PIN is not defined
#endif

#if !defined(EPAPER_CS_PORT)
    #error EPAPER_CS_PORT is not defined
#endif

#if !defined(EPAPER_CS_DDR)
    #error EPAPER_CS_DDR is not defined
#endif

#if !defined(EPAPER_CS_PIN)
    #error EPAPER_CS_PIN is not defined
#endif

#if !defined(EPAPER_BUSY_PORT)
    #error EPAPER_BUSY_PORT is not defined
#endif

#if !defined(EPAPER_BUSY_DDR)
    #error EPAPER_BUSY_DDR is not defined
#endif

#if !defined(EPAPER_BUSY_PIN)
    #error EPAPER_BUSY_PIN is not defined
#endif

#if !defined(EPAPER_BUSY_PORT_IN)
    #error EPAPER_BUSY_PORT_IN is not defined
#endif

#define EPD_FRAME_WIDTH       (120)
#define EPD_FRAME_HEIGHT      (248)
#define EPD_WIDTH_X     (EPD_FRAME_WIDTH/8)
#define EPD_HEIGHT_Y    (EPD_FRAME_HEIGHT)

static inline void _cs_pin_high(void)
{
    EPAPER_CS_PORT |= 1 << EPAPER_CS_PIN;
}

static inline void _cs_pin_low(void)
{
    EPAPER_CS_PORT &= ~(1 << EPAPER_CS_PIN);
}

static inline void _dc_pin_high(void)
{
    EPAPER_DC_PORT |= 1 << EPAPER_DC_PIN;
}

static inline void _dc_pin_low(void)
{
    EPAPER_DC_PORT &= ~(1 << EPAPER_DC_PIN);
}

static inline uint8_t _busy_pin_read(void)
{
    return EPAPER_BUSY_PORT_IN & (1 << EPAPER_BUSY_PIN);
}

static inline void _rst_pin_high(void)
{
    EPAPER_RST_PORT |= 1 << EPAPER_RST_PIN;
}

static inline void _rst_pin_low(void)
{
    EPAPER_RST_PORT &= ~(1 << EPAPER_RST_PIN);
}

static void _spi_transfer(uint8_t data)
{
    _cs_pin_low();
    spi_master_transfer_blocking(&data, 0, 1);
    _cs_pin_high();
}

static void _send_command(uint8_t command)
{
    _dc_pin_low();
    _spi_transfer(command);
}

static void _send_data(uint8_t data)
{
    _dc_pin_high();
    _spi_transfer(data);
}

static void _wait_until_idle(void)
{
    uint16_t timeout = 1000;  // 10s
    while (_busy_pin_read() && --timeout)
    {
        _delay_ms(10);
    }
     _delay_ms(1);
}

static void _reset(void)
{
    _rst_pin_high();
    _delay_ms(20);
    _rst_pin_low();
    _delay_ms(2);
    _rst_pin_high();
    _delay_ms(20);
}

static void _set_windows(uint8_t x_start, uint16_t y_start, uint8_t x_end, uint16_t y_end)
{
    _send_command(0x44);  // SET_RAM_X_ADDRESS_START_END_POSITION
    _send_data(x_start >> 3);
    x_end--;
    _send_data(x_end >> 3);

    _send_command(0x45);  // SET_RAM_Y_ADDRESS_START_END_POSITION
    _send_data(y_start & 0xFF);
    _send_data((y_start >> 8) & 0xFF);
    y_end--;
    _send_data(y_end & 0xFF);
    _send_data((y_end >> 8) & 0xFF);
}

static void _set_cursor(uint8_t x_start, uint16_t y_start)
{
    _send_command(0x4E);  // SET_RAM_X_ADDRESS_COUNTER
    _send_data(x_start >> 3);

    _send_command(0x4F);  // SET_RAM_Y_ADDRESS_COUNTER
    _send_data(y_start & 0xFF);
    _send_data((y_start >> 8) & 0xFF);
}

static void _clear_frame(uint16_t size, uint8_t is_red)
{
    _send_command(is_red ? 0x26 : 0x24);
    for (uint16_t i = 0; i < size; i++)
    {
        _send_data(0xFF);
    }
}

static void _fill_frame(uint16_t size, uint8_t is_red)
{
    _send_command(is_red ? 0x26 : 0x24);
    for (uint16_t i = 0; i < size; i++)
    {
        _send_data(0);
    }
}

static void _init(void)
{
    _wait_until_idle();
    _send_command(0x12);  // SWRESET
    _wait_until_idle();

    _send_command(0x01);  // Driver output control
    _send_data(0xf9);
    _send_data(0x00);
    _send_data(0x00);

    _send_command(0x11);  // data entry mode
    _send_data(0x03);

    _send_command(0x3C);  // BorderWavefrom
    _send_data(0x05);

    _send_command(0x18);  // Read built-in temperature sensor
    _send_data(0x80);

    _send_command(0x21);  // Display update control
    _send_data(0x80);
    _send_data(0x80);

    _wait_until_idle();
}

void epaper_display_frame(void)
{
    _send_command(0x20);
    _wait_until_idle();
}

void epaper_init(void)
{
    EPAPER_CS_DDR |= 1 << EPAPER_CS_PIN;
    EPAPER_RST_DDR |= 1 << EPAPER_RST_PIN;
    EPAPER_DC_DDR |= 1 << EPAPER_DC_PIN;
    EPAPER_BUSY_DDR &= ~(1 << EPAPER_BUSY_PIN);

    _reset();
    _init();

    _set_windows(0, 0, EPD_WIDTH, EPD_HEIGHT);
    _set_cursor(0, 0);

    _clear_frame(((EPD_WIDTH + 8) * EPD_HEIGHT - 1) / 8 + 1, 0);
    _clear_frame(((EPD_WIDTH + 8) * EPD_HEIGHT - 1) / 8 + 1, 1);
}

void epaper_update_frame_buf(uint8_t* frame_buffer, uint8_t x_start, uint16_t y_start, uint8_t x_end, uint16_t y_end, uint8_t is_red)
{
    _set_windows(x_start, y_start, x_end, y_end);
    _set_cursor(x_start, y_start);
    x_end -= x_start;
    y_end -= y_start;
    _send_command(is_red ? 0x26 : 0x24);
    uint16_t n = y_end;
    n *= x_end;
    n /= 8;
    for (uint16_t i = 0; i < n; i++)
    {
        _send_data(frame_buffer[i]);
    }
}

void epaper_update_frame_progmem(const unsigned char* frame_buffer, uint8_t x_start, uint16_t y_start, uint8_t x_end, uint16_t y_end, uint8_t is_red)
{
    _set_windows(x_start, y_start, x_end, y_end);
    _set_cursor(x_start, y_start);
    x_end -= x_start;
    y_end -= y_start;
    _send_command(is_red ? 0x26 : 0x24);
    uint16_t n = y_end;
    n *= x_end;
    n /= 8;
    for (uint16_t i = 0; i < n; i++)
    {
        _send_data(pgm_read_byte(&frame_buffer[i]));
    }
}

void epaper_clear_frame(uint8_t x_start, uint16_t y_start, uint8_t x_end, uint16_t y_end, uint8_t is_red)
{
    _set_windows(x_start, y_start, x_end, y_end);
    _set_cursor(x_start, y_start);
    x_end -= x_start;
    y_end -= y_start;
    _clear_frame(y_end * x_end / 8, is_red);
}

void epaper_clear_frame_all(uint8_t is_red)
{
    _set_windows(0, 0, EPD_WIDTH, EPD_HEIGHT);
    _set_cursor(0, 0);
    _clear_frame((EPD_WIDTH * EPD_HEIGHT - 1) / 8 + 1, is_red);
}

void epaper_fill_frame(uint8_t x_start, uint16_t y_start, uint8_t x_end, uint16_t y_end, uint8_t is_red)
{
    _set_windows(x_start, y_start, x_end, y_end);
    _set_cursor(x_start, y_start);
    x_end -= x_start;
    y_end -= y_start;
    _fill_frame(y_end * x_end / 8, is_red);
}

void epaper_fill_frame_all(uint8_t is_red)
{
    _set_windows(0, 0, EPD_WIDTH, EPD_HEIGHT);
    _set_cursor(0, 0);
    _fill_frame((EPD_WIDTH * EPD_HEIGHT - 1) / 8 + 1, is_red);
}

void epaper_sleep(void)
{
    _send_command(0x10);  // enter deep sleep
    _send_data(0x01);
    _delay_ms(100);
}

void epaper_wake(void)
{
    // _reset();
    _init();
}
