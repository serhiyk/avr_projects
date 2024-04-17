/**
* Copyright (c) 2021 Bosch Sensortec GmbH. All rights reserved.
*
* BSD-3-Clause
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its
*    contributors may be used to endorse or promote products derived from
*    this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
* @file       bme68x.c
* @date       2021-05-24
* @version    v4.4.6
*
*/

#include "bme68x.h"
#include <stdio.h>
#include <util/delay.h>
#include "TWI.h"

#define BMEx8x_ADDRESS (0x77 << 1)

static struct bme68x_conf _conf = {
    .filter = BME68X_FILTER_SIZE_3,
    .os_pres = BME68X_OS_4X,
    .os_hum = BME68X_OS_2X,
    .os_temp = BME68X_OS_8X,
    .odr = BME68X_ODR_NONE
};

static struct bme68x_heatr_conf _heatr_conf = {
    .enable = BME68X_ENABLE,
    .heatr_temp = 320,
    .heatr_dur = 150
};

static struct bme68x_dev _dev = {
    .amb_temp = 25,
};

struct bme68x_data sensor_data;
static uint8_t int_buff[BME68X_LEN_COEFF_ALL];

/* This internal API is used to read the calibration coefficients */
void get_calib_data(void);
void get_calib_data_1(void);
void get_calib_data_2(void);
void get_calib_data_3(void);
void get_calib_data_4(void);

/* This internal API is used to read variant ID information register status */
void read_variant_id(void);

/* This internal API is used to calculate the gas wait */
uint8_t calc_gas_wait(uint16_t dur);

/* This internal API is used to calculate the temperature in integer */
int16_t calc_temperature(uint32_t temp_adc);

/* This internal API is used to calculate the pressure in integer */
uint32_t calc_pressure(uint32_t pres_adc);

/* This internal API is used to calculate the humidity in integer */
uint32_t calc_humidity(uint16_t hum_adc);

/* This internal API is used to calculate the gas resistance high */
uint32_t calc_gas_resistance_high(uint16_t gas_res_adc, uint8_t gas_range);

/* This internal API is used to calculate the gas resistance low */
uint32_t calc_gas_resistance_low(uint16_t gas_res_adc, uint8_t gas_range);

/* This internal API is used to calculate the heater resistance using integer */
uint8_t calc_res_heat(uint16_t temp);

/* This internal API is used to set heater configurations */
void set_conf(void);

void _twi_wait(void)
{
    uint8_t i = 255;
    while (!twi_ready() && --i)
    {
        _delay_us(50);
    }
}

/******************************************************************************************/
/*                                 Global API definitions                                 */
/******************************************************************************************/

/* @brief This API reads the chip-id of the sensor which is the first step to
* verify the sensor and also calibrates the sensor
* As this API is the entry point, call this API before using other APIs.
*/
int8_t bme68x_init(void)
{
    bme68x_soft_reset();
    bme68x_get_regs(BME68X_REG_CHIP_ID, &_dev.chip_id, 1);
    if (_dev.chip_id == BME68X_CHIP_ID)
    {
        /* Read Variant ID */
        read_variant_id();

        /* Get the Calibration data */
        get_calib_data();
        get_calib_data_1();
        get_calib_data_2();
        get_calib_data_3();
        get_calib_data_4();
        return BME68X_OK;
    }
    else
    {
        return BME68X_E_DEV_NOT_FOUND;
    }
}

/*
 * @brief This API writes the given data to the register address of the sensor
 */
void bme68x_set_regs(const uint8_t *reg_addr, const uint8_t *reg_data, uint8_t len)
{
    /* Interleave the 2 arrays */
    for (uint8_t index = 0; index < len; index++)
    {
        int_buff[(2 * index)] = reg_addr[index];
        int_buff[(2 * index) + 1] = reg_data[index];
    }

    /* Write the interleaved array */
    _twi_wait();
    twi_master_send(BMEx8x_ADDRESS, int_buff, len * 2, 0);
    _twi_wait();
}

/*
 * @brief This API reads the data from the given register address of sensor.
 */
void bme68x_get_regs(uint8_t reg_addr, uint8_t *reg_data, uint8_t len)
{
    _twi_wait();
    twi_master_send(BMEx8x_ADDRESS, &reg_addr, 1, 0);
    _twi_wait();
    twi_master_receive(BMEx8x_ADDRESS, reg_data, len, 0);
    _twi_wait();
}

/*
 * @brief This API soft-resets the sensor.
 */
void bme68x_soft_reset(void)
{
    uint8_t reg_addr = BME68X_REG_SOFT_RESET;

    /* 0xb6 is the soft reset command */
    uint8_t soft_rst_cmd = BME68X_SOFT_RESET_CMD;

    bme68x_set_regs(&reg_addr, &soft_rst_cmd, 1);

    /* Wait for 5ms */
    _delay_ms(BME68X_PERIOD_RESET);
}

/*
 * @brief This API is used to set the oversampling, filter and odr configuration
 */
void bme68x_set_conf(void)
{
    uint8_t odr20 = 0, odr3 = 1;
    uint8_t current_op_mode;

    /* Register data starting from BME68X_REG_CTRL_GAS_1(0x71) up to BME68X_REG_CONFIG(0x75) */
    static uint8_t reg_array[BME68X_LEN_CONFIG] = { 0x71, 0x72, 0x73, 0x74, 0x75 };

    current_op_mode = bme68x_get_op_mode();
    if (current_op_mode != BME68X_SLEEP_MODE)
    {
        /* Configure only in the sleep mode */
        bme68x_set_op_mode(BME68X_SLEEP_MODE);
    }

    uint8_t data_array[BME68X_LEN_CONFIG];
    /* Read the whole configuration and write it back once later */
    bme68x_get_regs(reg_array[0], data_array, BME68X_LEN_CONFIG);

    data_array[4] = BME68X_SET_BITS(data_array[4], BME68X_FILTER, _conf.filter);
    data_array[3] = BME68X_SET_BITS(data_array[3], BME68X_OST, _conf.os_temp);
    data_array[3] = BME68X_SET_BITS(data_array[3], BME68X_OSP, _conf.os_pres);
    data_array[1] = BME68X_SET_BITS_POS_0(data_array[1], BME68X_OSH, _conf.os_hum);
    if (_conf.odr != BME68X_ODR_NONE)
    {
        odr20 = _conf.odr;
        odr3 = 0;
    }

    data_array[4] = BME68X_SET_BITS(data_array[4], BME68X_ODR20, odr20);
    data_array[0] = BME68X_SET_BITS(data_array[0], BME68X_ODR3, odr3);

    bme68x_set_regs(reg_array, data_array, BME68X_LEN_CONFIG);

    if (current_op_mode != BME68X_SLEEP_MODE)
    {
        bme68x_set_op_mode(current_op_mode);
    }
}

/*
 * @brief This API is used to set the operation mode of the sensor
 */
void bme68x_set_op_mode(const uint8_t op_mode)
{
    uint8_t tmp_pow_mode;
    uint8_t pow_mode = 0;
    uint8_t reg_addr = BME68X_REG_CTRL_MEAS;

    /* Call until in sleep */
    do
    {
        bme68x_get_regs(BME68X_REG_CTRL_MEAS, &tmp_pow_mode, 1);
        /* Put to sleep before changing mode */
        pow_mode = (tmp_pow_mode & BME68X_MODE_MSK);
        if (pow_mode != BME68X_SLEEP_MODE)
        {
            tmp_pow_mode &= ~BME68X_MODE_MSK; /* Set to sleep */
            bme68x_set_regs(&reg_addr, &tmp_pow_mode, 1);
            _delay_ms(BME68X_PERIOD_POLL);
        }
    } while (pow_mode != BME68X_SLEEP_MODE);

    /* Already in sleep */
    if (op_mode != BME68X_SLEEP_MODE)
    {
        tmp_pow_mode = (tmp_pow_mode & ~BME68X_MODE_MSK) | (op_mode & BME68X_MODE_MSK);
        bme68x_set_regs(&reg_addr, &tmp_pow_mode, 1);
    }
}

/*
 * @brief This API is used to get the operation mode of the sensor.
 */
uint8_t bme68x_get_op_mode(void)
{
    uint8_t mode;
    bme68x_get_regs(BME68X_REG_CTRL_MEAS, &mode, 1);
    /* Masking the other register bit info*/
    return mode & BME68X_MODE_MSK;
}

/*
 * @brief This API is used to set the gas configuration of the sensor.
 */
void bme68x_set_heatr_conf(void)
{
    uint8_t nb_conv = 0;
    static uint8_t ctrl_gas_addr[2] = { BME68X_REG_CTRL_GAS_0, BME68X_REG_CTRL_GAS_1 };

    if (bme68x_get_op_mode() != BME68X_SLEEP_MODE)
    {
        /* Configure only in the sleep mode */
        bme68x_set_op_mode(BME68X_SLEEP_MODE);
    }

    set_conf();

    uint8_t ctrl_gas_data[2];
    bme68x_get_regs(BME68X_REG_CTRL_GAS_0, ctrl_gas_data, 2);
    uint8_t hctrl, run_gas = 0;
    if (_heatr_conf.enable == BME68X_ENABLE)
    {
        hctrl = BME68X_ENABLE_HEATER;
        if (_dev.variant_id == BME68X_VARIANT_GAS_HIGH)
        {
            run_gas = BME68X_ENABLE_GAS_MEAS_H;
        }
        else
        {
            run_gas = BME68X_ENABLE_GAS_MEAS_L;
        }
    }
    else
    {
        hctrl = BME68X_DISABLE_HEATER;
        run_gas = BME68X_DISABLE_GAS_MEAS;
    }

    ctrl_gas_data[0] = BME68X_SET_BITS(ctrl_gas_data[0], BME68X_HCTRL, hctrl);
    ctrl_gas_data[1] = BME68X_SET_BITS_POS_0(ctrl_gas_data[1], BME68X_NBCONV, nb_conv);
    ctrl_gas_data[1] = BME68X_SET_BITS(ctrl_gas_data[1], BME68X_RUN_GAS, run_gas);
    bme68x_set_regs(ctrl_gas_addr, ctrl_gas_data, 2);
}

/*****************************INTERNAL APIs***********************************************/

/* @brief This internal API is used to calculate the temperature value. */
int16_t calc_temperature(uint32_t temp_adc)
{
    static int64_t var1;
    static int64_t var2;
    static int64_t var3;
    static int16_t calc_temp;

    /*lint -save -e701 -e702 -e704 */
    var1 = ((int32_t)temp_adc >> 3) - ((int32_t)_dev.calib.par_t1 << 1);
    var2 = (var1 * (int32_t)_dev.calib.par_t2) >> 11;
    var3 = ((var1 >> 1) * (var1 >> 1)) >> 12;
    var3 = ((var3) * ((int32_t)_dev.calib.par_t3 << 4)) >> 14;
    _dev.calib.t_fine = (int32_t)(var2 + var3);
    calc_temp = (int16_t)(((_dev.calib.t_fine * 5) + 128) >> 8);

    /*lint -restore */
    return calc_temp;
}

/* @brief This internal API is used to calculate the pressure value. */
uint32_t calc_pressure(uint32_t pres_adc)
{
    static int32_t var1;
    static int32_t var2;
    static int32_t var3;
    static int32_t pressure_comp;

    /* This value is used to check precedence to multiplication or division
     * in the pressure compensation equation to achieve least loss of precision and
     * avoiding overflows.
     * i.e Comparing value, pres_ovf_check = (1 << 31) >> 1
     */
    const int32_t pres_ovf_check = INT32_C(0x40000000);

    /*lint -save -e701 -e702 -e713 */
    var1 = (((int32_t)_dev.calib.t_fine) >> 1) - 64000;
    var2 = ((((var1 >> 2) * (var1 >> 2)) >> 11) * (int32_t)_dev.calib.par_p6) >> 2;
    var2 = var2 + ((var1 * (int32_t)_dev.calib.par_p5) << 1);
    var2 = (var2 >> 2) + ((int32_t)_dev.calib.par_p4 << 16);
    var1 = (((((var1 >> 2) * (var1 >> 2)) >> 13) * ((int32_t)_dev.calib.par_p3 << 5)) >> 3) +
           (((int32_t)_dev.calib.par_p2 * var1) >> 1);
    var1 = var1 >> 18;
    var1 = ((32768 + var1) * (int32_t)_dev.calib.par_p1) >> 15;
    pressure_comp = 1048576 - pres_adc;
    pressure_comp = (int32_t)((pressure_comp - (var2 >> 12)) * ((uint32_t)3125));
    if (pressure_comp >= pres_ovf_check)
    {
        pressure_comp = ((pressure_comp / var1) << 1);
    }
    else
    {
        pressure_comp = ((pressure_comp << 1) / var1);
    }

    var1 = ((int32_t)_dev.calib.par_p9 * (int32_t)(((pressure_comp >> 3) * (pressure_comp >> 3)) >> 13)) >> 12;
    var2 = ((int32_t)(pressure_comp >> 2) * (int32_t)_dev.calib.par_p8) >> 13;
    var3 =
        ((int32_t)(pressure_comp >> 8) * (int32_t)(pressure_comp >> 8) * (int32_t)(pressure_comp >> 8) *
         (int32_t)_dev.calib.par_p10) >> 17;
    pressure_comp = (int32_t)(pressure_comp) + ((var1 + var2 + var3 + ((int32_t)_dev.calib.par_p7 << 7)) >> 4);

    /*lint -restore */
    return (uint32_t)pressure_comp;
}

/* This internal API is used to calculate the humidity in integer */
uint32_t calc_humidity(uint16_t hum_adc)
{
    static int32_t var1;
    static int32_t var2;
    static int32_t var3;
    static int32_t var4;
    static int32_t var5;
    static int32_t var6;
    static int32_t temp_scaled;
    static int32_t calc_hum;

    /*lint -save -e702 -e704 */
    temp_scaled = (((int32_t)_dev.calib.t_fine * 5) + 128) >> 8;
    var1 = (int32_t)(hum_adc - ((int32_t)((int32_t)_dev.calib.par_h1 * 16))) -
           (((temp_scaled * (int32_t)_dev.calib.par_h3) / ((int32_t)100)) >> 1);
    var2 =
        ((int32_t)_dev.calib.par_h2 *
         (((temp_scaled * (int32_t)_dev.calib.par_h4) / ((int32_t)100)) +
          (((temp_scaled * ((temp_scaled * (int32_t)_dev.calib.par_h5) / ((int32_t)100))) >> 6) / ((int32_t)100)) +
          (int32_t)(1 << 14))) >> 10;
    var3 = var1 * var2;
    var4 = (int32_t)_dev.calib.par_h6 << 7;
    var4 = ((var4) + ((temp_scaled * (int32_t)_dev.calib.par_h7) / ((int32_t)100))) >> 4;
    var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
    var6 = (var4 * var5) >> 1;
    calc_hum = (((var3 + var6) >> 10) * ((int32_t)1000)) >> 12;
    if (calc_hum > 100000UL) /* Cap at 100%rH */
    {
        calc_hum = 100000UL;
    }
    else if (calc_hum < 0)
    {
        calc_hum = 0;
    }

    /*lint -restore */
    return (uint32_t)calc_hum;
}

/* This internal API is used to calculate the gas resistance low */
uint32_t calc_gas_resistance_low(uint16_t gas_res_adc, uint8_t gas_range)
{
    static int64_t var1;
    static uint64_t var2;
    static int64_t var3;
    static uint32_t calc_gas_res;
    static uint32_t lookup_table1[16] = {
        UINT32_C(2147483647), UINT32_C(2147483647), UINT32_C(2147483647), UINT32_C(2147483647), UINT32_C(2147483647),
        UINT32_C(2126008810), UINT32_C(2147483647), UINT32_C(2130303777), UINT32_C(2147483647), UINT32_C(2147483647),
        UINT32_C(2143188679), UINT32_C(2136746228), UINT32_C(2147483647), UINT32_C(2126008810), UINT32_C(2147483647),
        UINT32_C(2147483647)
    };
    static uint32_t lookup_table2[16] = {
        UINT32_C(4096000000), UINT32_C(2048000000), UINT32_C(1024000000), UINT32_C(512000000), UINT32_C(255744255),
        UINT32_C(127110228), UINT32_C(64000000), UINT32_C(32258064), UINT32_C(16016016), UINT32_C(8000000), UINT32_C(
            4000000), UINT32_C(2000000), UINT32_C(1000000), UINT32_C(500000), UINT32_C(250000), UINT32_C(125000)
    };

    /*lint -save -e704 */
    var1 = (int64_t)((1340 + (5 * (int64_t)_dev.calib.range_sw_err)) * ((int64_t)lookup_table1[gas_range])) >> 16;
    var2 = (((int64_t)((int64_t)gas_res_adc << 15) - (int64_t)(16777216UL)) + var1);
    var3 = (((int64_t)lookup_table2[gas_range] * (int64_t)var1) >> 9);
    calc_gas_res = (uint32_t)((var3 + ((int64_t)var2 >> 1)) / (int64_t)var2);

    /*lint -restore */
    return calc_gas_res;
}

/* This internal API is used to calculate the gas resistance */
uint32_t calc_gas_resistance_high(uint16_t gas_res_adc, uint8_t gas_range)
{
    uint32_t calc_gas_res;
    uint32_t var1 = UINT32_C(262144) >> gas_range;
    int32_t var2 = (int32_t)gas_res_adc - INT32_C(512);

    var2 *= INT32_C(3);
    var2 = INT32_C(4096) + var2;

    /* multiplying 10000 then dividing then multiplying by 100 instead of multiplying by 1000000 to prevent overflow */
    calc_gas_res = (UINT32_C(10000) * var1) / (uint32_t)var2;
    calc_gas_res = calc_gas_res * 100;

    return calc_gas_res;
}

/* This internal API is used to calculate the heater resistance value using float */
uint8_t calc_res_heat(uint16_t temp)
{
    static uint8_t heatr_res;
    static int32_t var1;
    static int32_t var2;
    static int32_t var3;
    static int32_t var4;
    static int32_t var5;
    static int32_t heatr_res_x100;

    if (temp > 400) /* Cap temperature */
    {
        temp = 400;
    }

    var1 = (((int32_t)_dev.amb_temp * _dev.calib.par_gh3) / 1000) * 256;
    var2 = (_dev.calib.par_gh1 + 784) * (((((_dev.calib.par_gh2 + 154009UL) * temp * 5) / 100) + 3276800UL) / 10);
    var3 = var1 + (var2 / 2);
    var4 = (var3 / (_dev.calib.res_heat_range + 4));
    var5 = (131 * _dev.calib.res_heat_val) + 65536UL;
    heatr_res_x100 = (int32_t)(((var4 / var5) - 250) * 34);
    heatr_res = (uint8_t)((heatr_res_x100 + 50) / 100);

    return heatr_res;
}

/* This internal API is used to calculate the gas wait */
uint8_t calc_gas_wait(uint16_t dur)
{
    uint8_t factor = 0;
    uint8_t durval;

    if (dur >= 0xfc0)
    {
        durval = 0xff; /* Max duration*/
    }
    else
    {
        while (dur > 0x3F)
        {
            dur = dur / 4;
            factor += 1;
        }

        durval = (uint8_t)(dur + (factor * 64));
    }

    return durval;
}

static uint8_t gas_range_l, gas_range_h;
static uint32_t adc_temp;
static uint32_t adc_pres;
static uint16_t adc_hum;
static uint16_t adc_gas_res_low, adc_gas_res_high;

/* This internal API is used to read a single data of the sensor */
void bme68x_get_data(void)
{
    bme68x_get_regs(BME68X_REG_FIELD0, int_buff, BME68X_LEN_FIELD);

    sensor_data.status = int_buff[0] & BME68X_NEW_DATA_MSK;
    sensor_data.gas_index = int_buff[0] & BME68X_GAS_INDEX_MSK;
    sensor_data.meas_index = int_buff[1];

    /* read the raw data from the sensor */
    adc_pres = (uint32_t)(((uint32_t)int_buff[2] * 4096) | ((uint32_t)int_buff[3] * 16) | ((uint32_t)int_buff[4] / 16));
    adc_temp = (uint32_t)(((uint32_t)int_buff[5] * 4096) | ((uint32_t)int_buff[6] * 16) | ((uint32_t)int_buff[7] / 16));
    adc_hum = (uint16_t)(((uint32_t)int_buff[8] * 256) | (uint32_t)int_buff[9]);
    adc_gas_res_low = (uint16_t)((uint32_t)int_buff[13] * 4 | (((uint32_t)int_buff[14]) / 64));
    adc_gas_res_high = (uint16_t)((uint32_t)int_buff[15] * 4 | (((uint32_t)int_buff[16]) / 64));
    gas_range_l = int_buff[14] & BME68X_GAS_RANGE_MSK;
    gas_range_h = int_buff[16] & BME68X_GAS_RANGE_MSK;
    if (_dev.variant_id == BME68X_VARIANT_GAS_HIGH)
    {
        sensor_data.status |= int_buff[16] & BME68X_GASM_VALID_MSK;
        sensor_data.status |= int_buff[16] & BME68X_HEAT_STAB_MSK;
    }
    else
    {
        sensor_data.status |= int_buff[14] & BME68X_GASM_VALID_MSK;
        sensor_data.status |= int_buff[14] & BME68X_HEAT_STAB_MSK;
    }
}

void bme68x_get_data_1(void)
{
    if (sensor_data.status & BME68X_NEW_DATA_MSK)
    {
        bme68x_get_regs(BME68X_REG_RES_HEAT0 + sensor_data.gas_index, &sensor_data.res_heat, 1);
        bme68x_get_regs(BME68X_REG_IDAC_HEAT0 + sensor_data.gas_index, &sensor_data.idac, 1);
        bme68x_get_regs(BME68X_REG_GAS_WAIT0 + sensor_data.gas_index, &sensor_data.gas_wait, 1);

        sensor_data.temperature = calc_temperature(adc_temp);
        sensor_data.pressure = calc_pressure(adc_pres);
        sensor_data.humidity = calc_humidity(adc_hum);
        if (_dev.variant_id == BME68X_VARIANT_GAS_HIGH)
        {
            sensor_data.gas_resistance = calc_gas_resistance_high(adc_gas_res_high, gas_range_h);
        }
        else
        {
            sensor_data.gas_resistance = calc_gas_resistance_low(adc_gas_res_low, gas_range_l);
        }
    }
}

/* This internal API is used to set heater configurations */
void set_conf(void)
{
    uint8_t rh_reg_addr[1] = { BME68X_REG_RES_HEAT0 };
    uint8_t rh_reg_data[1] = { calc_res_heat(_heatr_conf.heatr_temp) };
    bme68x_set_regs(rh_reg_addr, rh_reg_data, 1);
    uint8_t gw_reg_addr[1] = { BME68X_REG_GAS_WAIT0 };
    uint8_t gw_reg_data[1] = { calc_gas_wait(_heatr_conf.heatr_dur) };
    bme68x_set_regs(gw_reg_addr, gw_reg_data, 1);
}

/* This internal API is used to read the calibration coefficients */
void get_calib_data(void)
{
    bme68x_get_regs(BME68X_REG_COEFF1, int_buff, BME68X_LEN_COEFF1);
    bme68x_get_regs(BME68X_REG_COEFF2, &int_buff[BME68X_LEN_COEFF1], BME68X_LEN_COEFF2);
    bme68x_get_regs(BME68X_REG_COEFF3, &int_buff[BME68X_LEN_COEFF1 + BME68X_LEN_COEFF2], BME68X_LEN_COEFF3);
}

void get_calib_data_1(void)
{
    /* Temperature related coefficients */
    _dev.calib.par_t1 =
        (uint16_t)(BME68X_CONCAT_BYTES(int_buff[BME68X_IDX_T1_MSB], int_buff[BME68X_IDX_T1_LSB]));
    _dev.calib.par_t2 =
        (int16_t)(BME68X_CONCAT_BYTES(int_buff[BME68X_IDX_T2_MSB], int_buff[BME68X_IDX_T2_LSB]));
    _dev.calib.par_t3 = (int8_t)(int_buff[BME68X_IDX_T3]);

}

void get_calib_data_2(void)
{
    /* Pressure related coefficients */
    _dev.calib.par_p1 =
        (uint16_t)(BME68X_CONCAT_BYTES(int_buff[BME68X_IDX_P1_MSB], int_buff[BME68X_IDX_P1_LSB]));
    _dev.calib.par_p2 =
        (int16_t)(BME68X_CONCAT_BYTES(int_buff[BME68X_IDX_P2_MSB], int_buff[BME68X_IDX_P2_LSB]));
    _dev.calib.par_p3 = (int8_t)int_buff[BME68X_IDX_P3];
    _dev.calib.par_p4 =
        (int16_t)(BME68X_CONCAT_BYTES(int_buff[BME68X_IDX_P4_MSB], int_buff[BME68X_IDX_P4_LSB]));
    _dev.calib.par_p5 =
        (int16_t)(BME68X_CONCAT_BYTES(int_buff[BME68X_IDX_P5_MSB], int_buff[BME68X_IDX_P5_LSB]));
    _dev.calib.par_p6 = (int8_t)(int_buff[BME68X_IDX_P6]);
    _dev.calib.par_p7 = (int8_t)(int_buff[BME68X_IDX_P7]);
    _dev.calib.par_p8 =
        (int16_t)(BME68X_CONCAT_BYTES(int_buff[BME68X_IDX_P8_MSB], int_buff[BME68X_IDX_P8_LSB]));
    _dev.calib.par_p9 =
        (int16_t)(BME68X_CONCAT_BYTES(int_buff[BME68X_IDX_P9_MSB], int_buff[BME68X_IDX_P9_LSB]));
    _dev.calib.par_p10 = (uint8_t)(int_buff[BME68X_IDX_P10]);


}

void get_calib_data_3(void)
{
    /* Humidity related coefficients */
    _dev.calib.par_h1 =
        (uint16_t)(((uint16_t)int_buff[BME68X_IDX_H1_MSB] << 4) |
                   (int_buff[BME68X_IDX_H1_LSB] & BME68X_BIT_H1_DATA_MSK));
    _dev.calib.par_h2 =
        (uint16_t)(((uint16_t)int_buff[BME68X_IDX_H2_MSB] << 4) | ((int_buff[BME68X_IDX_H2_LSB]) >> 4));
    _dev.calib.par_h3 = (int8_t)int_buff[BME68X_IDX_H3];
    _dev.calib.par_h4 = (int8_t)int_buff[BME68X_IDX_H4];
    _dev.calib.par_h5 = (int8_t)int_buff[BME68X_IDX_H5];
    _dev.calib.par_h6 = (uint8_t)int_buff[BME68X_IDX_H6];
    _dev.calib.par_h7 = (int8_t)int_buff[BME68X_IDX_H7];
}

void get_calib_data_4(void)
{
    /* Gas heater related coefficients */
    _dev.calib.par_gh1 = (int8_t)int_buff[BME68X_IDX_GH1];
    _dev.calib.par_gh2 =
        (int16_t)(BME68X_CONCAT_BYTES(int_buff[BME68X_IDX_GH2_MSB], int_buff[BME68X_IDX_GH2_LSB]));
    _dev.calib.par_gh3 = (int8_t)int_buff[BME68X_IDX_GH3];

    /* Other coefficients */
    _dev.calib.res_heat_range = ((int_buff[BME68X_IDX_RES_HEAT_RANGE] & BME68X_RHRANGE_MSK) / 16);
    _dev.calib.res_heat_val = (int8_t)int_buff[BME68X_IDX_RES_HEAT_VAL];
    _dev.calib.range_sw_err = ((int8_t)(int_buff[BME68X_IDX_RANGE_SW_ERR] & BME68X_RSERROR_MSK)) / 16;
}

/* This internal API is used to read variant ID information from the register */
void read_variant_id(void)
{
    uint8_t reg_data = 0;
    bme68x_get_regs(BME68X_REG_VARIANT_ID, &reg_data, 1);
    _dev.variant_id = reg_data;
}
