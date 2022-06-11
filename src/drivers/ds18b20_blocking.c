#include <stdint.h>
#include "onewire_blocking.h"
#include "ds18b20.h"

#define CMD_SKIP_ROM 0xCC
#define CMD_CONVERT_T 0x44
#define CMD_READ_SCRATCHPAD 0xBE

static uint8_t scratch[9];

static uint8_t calc_crc(uint16_t adress)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
        crc = crc ^ (*(uint16_t *)(adress + i));
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x01)
                crc = (crc >> 1) ^ 0x8C;
            else
                crc >>= 1;
        }
    }
    return crc;
}

uint8_t ds18b20_convert(void)
{
    if (onewire_master_reset() != 0)
    {
        return 1;
    }
    onewire_master_write(CMD_SKIP_ROM);
    onewire_master_write(CMD_CONVERT_T);
    return 0;
}

uint8_t ds18b20_read_temperature(ds18b20_cb callback)
{
    if (onewire_master_reset() != 0)
    {
        return 1;
    }
    onewire_master_write(CMD_SKIP_ROM);
    onewire_master_write(CMD_READ_SCRATCHPAD);
    for (uint8_t i = 0; i < 9; i++)
    {
        scratch[i] = onewire_master_read();
    }
    if (calc_crc((uint16_t)&scratch[0]) != scratch[8])
    {
        return 1;
    }
    uint16_t temperature = scratch[0];
    temperature |= scratch[1] << 8;
    callback(temperature);
    return 0;
}

// FF074B467FFF01102F
