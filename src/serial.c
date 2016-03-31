#include <stdint.h>
#include "UART.h"
#include "serial.h"

#define BOOTLOADER_START_ADDRESS 0x3800

typedef void (*command_cb)(void);
void command_goto_bootloader(void);

#define SERIAL_FIRST_COMMAND 0x30
#define SERIAL_COMMAND_END 0x20

#define SERIAL_COMMAND_COUNT 1
static command_cb serial_command_list[SERIAL_COMMAND_COUNT] =
{
    command_goto_bootloader
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
