#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "config.h"
#include "onewire.h"
#include "ds18b20.h"

#define CMD_SKIP_ROM 0xCC
#define CMD_CONVERT_T 0x44
#define CMD_READ_SCRATCHPAD 0xBE

#define DS18B20_READY 0
#define DS18B20_BUSY 1

static uint8_t state=DS18B20_READY;
static ds18b20_cb ds18b20_callback;
static uint16_t temperature=0;

void _convert_t_cb(uint8_t data)
{
#ifdef PARASITE_POWER
    ONEWIRE_PORT |= 1 << ONEWIRE_PIN;
    ONEWIRE_DDR |= 1 << ONEWIRE_PIN;
#endif
    state = DS18B20_READY;
}

void _convert_skip_rom_cb(uint8_t data)
{
    onewire_master_write(CMD_CONVERT_T, _convert_t_cb);
}

void _pull_up2_cb(uint8_t data)
{
    if (data)
    {
        state = DS18B20_READY;
        return;
    }
    onewire_master_write(CMD_SKIP_ROM, _convert_skip_rom_cb);
}

void _pull_down2_cb(uint8_t data)
{
    onewire_master_pull_up(_pull_up2_cb);
}

uint8_t ds18b20_convert(void)
{
    if (state != DS18B20_READY)
    {
        return 1;
    }
    state = DS18B20_BUSY;
    onewire_master_pull_down(_pull_down2_cb);
    return 0;
}

void _read_temperature_2_cb(uint8_t data)
{
    temperature |= data << 8;
    if (ds18b20_callback)
    {
        ds18b20_callback(temperature);
    }
    state = DS18B20_READY;
}

void _read_temperature_1_cb(uint8_t data)
{
    temperature = data;
    onewire_master_read(_read_temperature_2_cb);
}

void _read_scratchpad_cb(uint8_t data)
{
    onewire_master_read(_read_temperature_1_cb);
}

void _read_skip_rom_cb(uint8_t data)
{
    onewire_master_write(CMD_READ_SCRATCHPAD, _read_scratchpad_cb);
}

void _pull_up1_cb(uint8_t data)
{
    if (data)
    {
        if (ds18b20_callback)
        {
            ds18b20_callback(DS18B20_INVALID_TEMPERATURE);
        }
        state = DS18B20_READY;
        return;
    }
    onewire_master_write(CMD_SKIP_ROM, _read_skip_rom_cb);
}

void _pull_down1_cb(uint8_t data)
{
    onewire_master_pull_up(_pull_up1_cb);
}

uint8_t ds18b20_read_temperature(ds18b20_cb callback)
{
    if (state != DS18B20_READY)
    {
        return 1;
    }
    state = DS18B20_BUSY;
    ds18b20_callback = callback;
    onewire_master_pull_down(_pull_down1_cb);
    return 0;
}
