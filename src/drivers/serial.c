#include <stdint.h>
#include "UART.h"
#include "config.h"
#include "serial.h"

#define BOOTLOADER_START_ADDRESS 0x3800

#if !defined(SERIAL_COMMAND_COUNT)
    #define SERIAL_COMMAND_COUNT 0
#endif
#define SERIAL_COMMAND_MAX_LEN 32

#define START_CODE 0x30
#define END_CODE 0x20
#define ESC_CODE 0xF0
#define START_REP_CODE 0xF1
#define END_REP_CODE 0xF2
#define ESC_REP_CODE 0xF3

static command_cb serial_command_list[SERIAL_COMMAND_COUNT];

void serial_init(void)
{
    uart_init();
}

uint8_t serial_register_command(uint8_t id, command_cb callback)
{
    if (id >= SERIAL_COMMAND_COUNT)
    {
        return 1;
    }
    serial_command_list[id] = callback;
    return 0;
}

void serial_handler(void)
{
    static uint8_t buf[SERIAL_COMMAND_MAX_LEN];
    static uint8_t index;
    static uint8_t esc;
    while (uart_check_receiver())
    {
        uint8_t tmp = uart_get_byte();
        if (tmp == START_CODE)
        {
            index = 0;
            esc = 0;
        }
        else if (tmp == END_CODE)
        {
            if (index > 0)
            {
                uint8_t cmd = buf[0];
                if (cmd < SERIAL_COMMAND_COUNT)
                {
                    serial_command_list[cmd](buf, index);
                }
            }
            else
            {
                ((void (*)(void))BOOTLOADER_START_ADDRESS)();  // goto bootloader
            }
        }
        else if (tmp == ESC_CODE)
        {
            esc = 1;
        }
        else if (index < SERIAL_COMMAND_MAX_LEN)
        {
            if (esc)
            {
                esc = 0;
                if (tmp == START_REP_CODE)
                {
                    tmp = START_CODE;
                }
                else if (tmp == END_REP_CODE)
                {
                    tmp = END_CODE;
                }
                else if (tmp == ESC_REP_CODE)
                {
                    tmp = ESC_CODE;
                }
            }
            buf[index++] = tmp;
        }
    }
}
