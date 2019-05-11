#include <util/delay.h>
#include "config.h"
#include "TWI.h"
#include "timer.h"
#include "bmp180.h"

#define REG_AC1 0xAA
#define REG_AC2 0xAC
#define REG_AC3 0xAE
#define REG_AC4 0xB0
#define REG_AC5 0xB2
#define REG_AC6 0xB4
#define REG_B1 0xB6
#define REG_B2 0xB8
#define REG_MB 0xBA
#define REG_MC 0xBC
#define REG_MD 0xBE
#define REG_ID 0xD0
#define REG_SOFT_RESET 0xE0
#define REG_CTRL_MEAS 0xF4
#define REG_OUT_MSB 0xF6
#define REG_OUT_LSB 0xF7
#define REG_OUT_XLSB 0xF8

#define BMP_PRES_OSS 3
#define CMD_TEMP_CONVERSION 0x2E
#define CMD_PRES_CONVERSION (0x34|(BMP_PRES_OSS<<6))
#define CMD_SOFT_RESET 0xB6

#define BMPx8x_ADDRESS (0x77 << 1)

static int16_t bmp_ac1;
static int16_t bmp_ac2;
static int16_t bmp_ac3;
static uint16_t bmp_ac4;
static uint16_t bmp_ac5;
static uint16_t bmp_ac6;
static int16_t bmp_b1;
static int16_t bmp_b2;
static int16_t bmp_b5;
static int16_t bmp_mb;
static int16_t bmp_mc;
static int16_t bmp_md;

typedef enum {
    BMP_IDLE = 0,
    BMP_TEMP_CONVERSION,
    BMP_TEMP_READ,
    BMP_PRES_CONVERSION,
    BMP_PRES_READ,
    BMP_PRES_READY
} bmp_state_t;

static uint8_t bmp_buf[3];
static uint8_t bmp_state=BMP_IDLE;
static bmp180_temperature_cb temperature_cb;
static bmp180_pressure_cb pressure_cb;

static void bmp_read(void)
{
    timer_stop(BMP180_TIMER_ID);
    bmp_state = BMP_TEMP_CONVERSION;
}

static void bmp_temp_read(void)
{
    timer_stop(BMP180_TIMER_ID);
    bmp_state = BMP_TEMP_READ;
}

static void bmp_temp_ready(void)
{
    bmp_state = BMP_PRES_CONVERSION;
}

static void bmp_pres_read(void)
{
    timer_stop(BMP180_TIMER_ID);
    bmp_state = BMP_PRES_READ;
}

static void bmp_pres_ready(void)
{
    bmp_state = BMP_PRES_READY;
}

void bmp180_init(bmp180_temperature_cb t_cb, bmp180_pressure_cb p_cb)
{
    uint8_t tmp_buf[22];
    while (!twi_ready());
    tmp_buf[0] = REG_SOFT_RESET;
    tmp_buf[1] = CMD_SOFT_RESET;
    if (twi_master_send(BMPx8x_ADDRESS, tmp_buf, 2, 0) != 0)
    {
        return;
    }
    while (!twi_ready());
    _delay_ms(5);
    tmp_buf[0] = REG_AC1;
    if (twi_master_transfer(BMPx8x_ADDRESS, tmp_buf, tmp_buf, 1, 22, 0) != 0)
    {
        return;
    }
    while (!twi_ready());
    bmp_ac1 = ((tmp_buf[0]<<8) | tmp_buf[1]);
    bmp_ac2 = ((tmp_buf[2]<<8) | tmp_buf[3]);
    bmp_ac3 = ((tmp_buf[4]<<8) | tmp_buf[5]);
    bmp_ac4 = ((tmp_buf[6]<<8) | tmp_buf[7]);
    bmp_ac5 = ((tmp_buf[8]<<8) | tmp_buf[9]);
    bmp_ac6 = ((tmp_buf[10]<<8) | tmp_buf[11]);
    bmp_b1 = ((tmp_buf[12]<<8) | tmp_buf[13]);
    bmp_b2 = ((tmp_buf[14]<<8) | tmp_buf[15]);
    bmp_mb = ((tmp_buf[16]<<8) | tmp_buf[17]);
    bmp_mc = ((tmp_buf[18]<<8) | tmp_buf[19]);
    bmp_md = ((tmp_buf[20]<<8) | tmp_buf[21]);
    temperature_cb = t_cb;
    pressure_cb = p_cb;
    timer_register(BMP180_TIMER_ID, BMP180_READ_PERIOD_S*10, bmp_read);
}

static void calculate_temperature(void)
{
    int32_t x1, x2;
    x1 = (uint16_t)((bmp_buf[0]<<8) | bmp_buf[1]);
    x1 -= (int32_t)bmp_ac6;
    x1 *= (int32_t)bmp_ac5;
    x1 >>= 15;
    x2 = bmp_mc;
    x2 <<= 11;
    x2 /= x1 + bmp_md;
    bmp_b5 = x1 + x2;
    int16_t t = ((bmp_b5 + 8) >> 4);  // temp in 0.1
    if (temperature_cb)
    {
        temperature_cb(t);
    }
}

static void calculate_pressure(void)
{
    int32_t up, x1, x2, x3, b3, b6, p;
    uint32_t b4, b7;
    up = bmp_buf[0];
    up <<= 8;
    up |= bmp_buf[1];
    up <<= 8;
    up |= bmp_buf[2];
    up >>= 8 - BMP_PRES_OSS;

    b6 = bmp_b5 - 4000;
    x1 = (bmp_b2 * (b6 * b6) >> 12) >> 11;
    x2 = (bmp_ac2 * b6) >> 11;
    x3 = x1 + x2;
    b3 = (((((int32_t)bmp_ac1) * 4 + x3) << BMP_PRES_OSS) + 2) >> 2;

    x1 = (bmp_ac3 * b6) >> 13;
    x2 = (bmp_b1 * ((b6 * b6) >> 12)) >> 16;
    x3 = ((x1 + x2) + 2) >> 2;
    b4 = (bmp_ac4 * (uint32_t)(x3 + 32768)) >> 15;

    b7 = ((uint32_t)(up - b3) * (50000 >> BMP_PRES_OSS));
    if (b7 < 0x80000000)
    {
        p = (b7 << 1) / b4;
    }
    else
    {
        p = (b7 / b4) << 1;
    }

    x1 = (p >> 8) * (p >> 8);
    x1 = (x1 * 3038) >> 16;
    x2 = (-7357 * p) >> 16;
    p += (x1 + x2 + 3791) >> 4;
    if (pressure_cb)
    {
        pressure_cb(p);
    }
}

void bmp180_handler(void)
{
    if (!twi_ready())
    {
        return;
    }
    switch (bmp_state)
    {
        case BMP_IDLE:
            return;
        case BMP_TEMP_CONVERSION:
            bmp_buf[0] = REG_CTRL_MEAS;
            bmp_buf[1] = CMD_TEMP_CONVERSION;
            if (twi_master_send(BMPx8x_ADDRESS, bmp_buf, 2, 0) != 0)
            {
                return;
            }
            bmp_state = BMP_IDLE;
            timer_register(BMP180_TIMER_ID, 1, bmp_temp_read);
            return;
        case BMP_TEMP_READ:
            bmp_buf[0] = REG_OUT_MSB;
            if (twi_master_transfer(BMPx8x_ADDRESS, bmp_buf, bmp_buf, 1, 2, bmp_temp_ready) != 0)
            {
                return;
            }
            bmp_state = BMP_IDLE;
            return;
        case BMP_PRES_CONVERSION:
            calculate_temperature();
            bmp_buf[0] = REG_CTRL_MEAS;
            bmp_buf[1] = CMD_PRES_CONVERSION;
            if (twi_master_send(BMPx8x_ADDRESS, bmp_buf, 2, 0) != 0)
            {
                return;
            }
            bmp_state = BMP_IDLE;
            timer_register(BMP180_TIMER_ID, 1, bmp_pres_read);
            return;
        case BMP_PRES_READ:
            bmp_buf[0] = REG_OUT_MSB;
            if (twi_master_transfer(BMPx8x_ADDRESS, bmp_buf, bmp_buf, 1, 3, bmp_pres_ready) != 0)
            {
                return;
            }
            bmp_state = BMP_IDLE;
            return;
        case BMP_PRES_READY:
            calculate_pressure();
            timer_register(BMP180_TIMER_ID, BMP180_READ_PERIOD_S*10, bmp_read);
            bmp_state = BMP_IDLE;
            return;
    }
}
