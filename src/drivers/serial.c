#include <stdint.h>
#include "UART.h"
#include "DS3231.h"
#include "serial.h"

#define BOOTLOADER_START_ADDRESS 0x3800

typedef void (*command_cb)(void);
void command_goto_bootloader(void);
void command_set_time(void);

#define SERIAL_FIRST_COMMAND 0x30
#define SERIAL_COMMAND_END 0x20

#define SERIAL_COMMAND_COUNT 2
static command_cb serial_command_list[SERIAL_COMMAND_COUNT] =
{
    command_goto_bootloader,
    command_set_time
};

#define SERIAL_COMMAND_MAX_LEN 32
static uint8_t serial_command_buf[SERIAL_COMMAND_MAX_LEN];
static uint8_t serial_command_index=0;

void serial_init(void)
{
    uart_init();
}

void serial_handler(void)
{
    while (uart_check_receiver())
    {
        uint8_t tmp = uart_get_byte();
        if (tmp == SERIAL_COMMAND_END)
        {
            uint8_t cmd = serial_command_buf[0];
            if (cmd >= SERIAL_FIRST_COMMAND && cmd < SERIAL_FIRST_COMMAND + SERIAL_COMMAND_COUNT)
            {
                serial_command_list[cmd-SERIAL_FIRST_COMMAND]();
            }
            serial_command_index = 0;
        }
        else if (serial_command_index < SERIAL_COMMAND_MAX_LEN)
        {
            serial_command_buf[serial_command_index++] = tmp;
        }
        else
        {
            serial_command_index = 0;
        }
    }
}

void command_goto_bootloader(void)
{
    if (serial_command_index != 1)
    {
        return;
    }
    ((void (*)(void))BOOTLOADER_START_ADDRESS)();
}

void command_set_time(void)
{
    if (serial_command_index != 15)
    {
        return;
    }
    for (uint8_t i=1; i<15; i++)
    {
        if (serial_command_buf[i] < '0' || serial_command_buf[i] > '9')
        {
            return;
        }
        serial_command_buf[i] -= '0';
    }
    set_time_bcd((serial_command_buf[1] << 4) | serial_command_buf[2],
                 (serial_command_buf[3] << 4) | serial_command_buf[4],
                 (serial_command_buf[5] << 4) | serial_command_buf[6],
                 (serial_command_buf[7] << 4) | serial_command_buf[8],
                 (serial_command_buf[9] << 4) | serial_command_buf[10],
                 (serial_command_buf[11] << 4) | serial_command_buf[12],
                 (serial_command_buf[13] << 4) | serial_command_buf[14]);
}
